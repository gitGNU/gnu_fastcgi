/*
 * Copyright (c) 2001-2007 Peter Simons <simons@cryp.to>
 *
 * This software is provided 'as-is', without any express or
 * implied warranty. In no event will the authors be held liable
 * for any damages arising from the use of this software.
 *
 * Copying and distribution of this file, with or without
 * modification, are permitted in any medium without royalty
 * provided the copyright notice and this notice are preserved.
 */

#include "fastcgi.hpp"
#include <iostream>             // ISO C++
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>       // POSIX.1
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "scheduler.hpp"        // http://cryp.to/libscheduler/

//
// Networking Code
//

template<class T>
class Listener : public scheduler::event_handler
{
public:
  Listener(scheduler& sched) : mysched(sched)
  {
    scheduler::handler_properties properties;
    properties.poll_events  = POLLIN;
    properties.read_timeout = 0;
    mysched.register_handler(0, *this, properties);
  }
  virtual ~Listener()
  {
    mysched.remove_handler(0);
  }

private:
  virtual void fd_is_readable(int fd)
  {
    int       socket;
    sockaddr  sa;
    socklen_t sa_len = sizeof(sa);

    socket = accept(fd, &sa, &sa_len);
    if (socket >= 0)
      new T(mysched, socket);
    else
      throw std::runtime_error(std::string("accept() failed: ") + strerror(errno));
  }
  virtual void fd_is_writable(int)
  {
    throw std::logic_error("This routine should not be called.");
  }
  virtual void read_timeout(int)
  {
    throw std::logic_error("This routine should not be called.");
  }
  virtual void write_timeout(int)
  {
    throw std::logic_error("This routine should not be called.");
  }
  virtual void error_condition(int)
  {
    throw std::logic_error("This routine should not be called.");
  }
  virtual void pollhup(int)
  {
    throw std::logic_error("This routine should not be called.");
  }

  scheduler& mysched;
};

template<class T>
class ConnectionHandler : public scheduler::event_handler,
			  public FCGIProtocolDriver::OutputCallback
{
public:
  ConnectionHandler(scheduler& sched, int sock)
    : mysched(sched), mysocket(sock), is_write_handler_registered(false),
      driver(*this), terminate(false)
  {
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
      throw std::runtime_error(std::string("Can set non-blocking mode: ") + strerror(errno));

    properties.poll_events  = POLLIN;
    properties.read_timeout = 0;
    mysched.register_handler(mysocket, *this, properties);
  }
  ~ConnectionHandler()
  {
    mysched.remove_handler(mysocket);
    close(mysocket);
  }

private:
  virtual void operator() (void const * buf, size_t count)
  {
    if (write_buffer.empty())
    {
      int rc = write(mysocket, buf, count);
      if (rc >= 0)
        write_buffer.append(static_cast<char const *>(buf)+rc, count-rc);
      else if (errno != EINTR && errno != EAGAIN)
      {
        char tmp[1024];
        snprintf(tmp, sizeof(tmp), "An error occured while writing to fd %d: %s",
                mysocket, strerror(errno));
        throw fcgi_io_callback_error(tmp);
      }
      else
        write_buffer.append(static_cast<char const *>(buf), count);
    }
    else
      write_buffer.append(static_cast<char const *>(buf), count);

    if (!write_buffer.empty() && is_write_handler_registered == false)
    {
      properties.poll_events   = POLLIN | POLLOUT;
      properties.write_timeout = 0;
      mysched.register_handler(mysocket, *this, properties);
      is_write_handler_registered = true;
    }
  }
  virtual void fd_is_readable(int fd)
  {
    char tmp[1024*10];
    int rc = read(fd, tmp, sizeof(tmp));
    if (rc > 0)
    {
      try
      {
        driver.process_input(tmp, rc);
        FCGIRequest* req = driver.get_request();
        if (req)
        {
          if (req->keep_connection == false)
            terminate = true;
          req->handler_cb = new T;
          req->handler_cb->operator()(req);
        }
      }
      catch(std::exception const & e)
      {
        std::cerr << "Caught exception thrown in FCGIProtocolDriver: " << e.what() << std::endl
                  << "Terminating connection " << mysocket << "." << std::endl;
        delete this;
        return;
      }
      catch(...)
      {
        std::cerr << "Caught unknown exception in FCGIProtocolDriver; terminating connection "
                  << mysocket << "." << std::endl;
        delete this;
        return;
      }
    }
    else if (rc <= 0 && errno != EINTR && errno != EAGAIN)
    {
      std::cerr << "An error occured while reading from fd " << mysocket << ": " << strerror(errno) << std::endl;
      delete this;
      return;
    }
    terminate_if_we_shall();
  }
  virtual void fd_is_writable(int fd)
  {
    if (write_buffer.empty())
    {
      properties.poll_events = POLLIN;
      mysched.register_handler(mysocket, *this, properties);
      is_write_handler_registered = false;
    }
    else
    {
      int rc = write(fd, write_buffer.data(), write_buffer.length());
      if (rc > 0)
        write_buffer.erase(0, rc);
      else if (rc < 0 && errno != EINTR && errno != EAGAIN)
      {
        std::cerr << "An error occured while writing to fd " << mysocket << ": " << strerror(errno) << std::endl;
        delete this;
        return;
      }
    }
    terminate_if_we_shall();
  }
  virtual void read_timeout(int)
  {
    throw std::logic_error("Not implemented yet.");
  }
  virtual void write_timeout(int)
  {
    throw std::logic_error("Not implemented yet.");
  }
  virtual void error_condition(int)
  {
    throw std::logic_error("Not implemented yet.");
  }
  virtual void pollhup(int)
  {
    throw std::logic_error("Not implemented yet.");
  }
  void terminate_if_we_shall()
  {
    if (terminate && driver.have_active_requests() == false && write_buffer.empty())
      delete this;
  }

  scheduler& mysched;
  scheduler::handler_properties properties;
  int mysocket;
  bool is_write_handler_registered;
  FCGIProtocolDriver driver;
  std::string write_buffer;
  bool terminate;
};

//
// FastCGI Handler
//

class RequestHandler : public FCGIRequest::handler
{
private:
  virtual void operator()(FCGIRequest* req)
  {
    if (!req->stdin_eof)
      return;

    // Make sure we are a responder.

    if (req->role != FCGIRequest::RESPONDER)
    {
      req->write("We can't handle any role but RESPONDER.", 39, FCGIRequest::STDERR);
      req->end_request(1, FCGIRequest::UNKNOWN_ROLE);
      return;
    }

    // Print page with the environment details.

    std::ostringstream os;
    os << "Content-type: text/html\r\n"
       << "\r\n"
       << "<title>FastCGI Echo Program</title>" << std::endl
       << "<h1 align=center>FastCGI Echo Program</h1>" << std::endl
       << "<h3>FastCGI Status</h3>" << std::endl
       << "Echo Program Compile Time = " << __DATE__ " " __TIME__ << "<br>" << std::endl
       << "Process id                = " << getpid() << "<br>" << std::endl
       << "Request id                = " << req->id << "<br>" << std::endl
       << "<h3>Request Environment</h3>" << std::endl;
    for (std::map<std::string,std::string>::const_iterator i = req->params.begin(); i != req->params.end(); ++i)
      os << i->first << "&nbsp;=&nbsp;" << i->second << "<br>" << std::endl;
    os << "<h3>Input Stream</h3>\n"
       << "<pre>\n";
    req->write(os.str().c_str(), os.str().size());

    for (size_t i = 0; i < req->stdin_stream.size(); )
    {
      size_t count = ((req->stdin_stream.size() - i) > 0xffff) ? 0xffff : req->stdin_stream.size()-i;
      req->write(req->stdin_stream.data()+i, count);
      i += count;
    }

    req->write("</pre>\n");
    req->write("<h1 align=center>End of Request</h1>\n");
    std::cerr << "Request #" << req->id << " handled successfully." << std::endl;
    req->end_request(0, FCGIRequest::REQUEST_COMPLETE);
  }
};

//
// The main program.
//

int main(int, char**)
try
{
  // Set a limit on the number of open files.

  rlimit rlim;
  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim.rlim_cur = 8;
  rlim.rlim_max = 8;
  if (setrlimit(RLIMIT_NOFILE, &rlim) != 0)
    throw std::runtime_error("setrlimit() failed.");

  // Let's go.

  scheduler sched;
  Listener< ConnectionHandler<RequestHandler> > listener(sched);
  while(!sched.empty())
  {
    sched.schedule();
    //sched.dump(std::cerr);
  }
  return 0;
}
catch(const std::exception &e)
{
  std::cerr << "Caught exception: " << e.what() << std::endl;
  return 1;
}
catch(...)
{
  std::cerr << "Caught unknown exception." << std::endl;
  return 1;
}
