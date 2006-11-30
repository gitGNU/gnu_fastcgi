/*
 * Copyright (c) 2005 by Peter Simons <simons@cryp.to>.
 *
 * Copying and distribution of this file, with or without
 * modification, are permitted in any medium without royalty
 * provided the copyright notice and this notice are preserved.
 *
 * See <http://cryp.to/libfastcgi/> for the latest version.
 */

#ifndef INFRASTRUCTURE_HH
#define INFRASTRUCTURE_HH

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "scheduler.hh"         // http://cryp.to/libscheduler/
#include "fastcgi.hh"

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
  virtual void operator() (const void* buf, size_t count)
  {
    if (write_buffer.empty())
    {
      int rc = write(mysocket, buf, count);
      if (rc >= 0)
        write_buffer.append(static_cast<const char*>(buf)+rc, count-rc);
      else if (errno != EINTR && errno != EAGAIN)
      {
        char tmp[1024];
        snprintf(tmp, sizeof(tmp), "An error occured while writing to fd %d: %s",
                mysocket, strerror(errno));
        throw fcgi_io_callback_error(tmp);
      }
      else
        write_buffer.append(static_cast<const char*>(buf), count);
    }
    else
      write_buffer.append(static_cast<const char*>(buf), count);

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
      catch(const std::exception& e)
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

#endif // !defined(INFRASTRUCTURE_HH)
