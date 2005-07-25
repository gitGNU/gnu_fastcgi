/*
 * Copyright (c) 2005 by Peter Simons <simons@cryp.to>.
 *
 * Copying and distribution of this file, with or without
 * modification, are permitted in any medium without royalty
 * provided the copyright notice and this notice are preserved.
 *
 * See <http://cryp.to/libfastcgi/> for the latest version.
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include "fastcgi.hh"

class OutputCallback : public FCGIProtocolDriver::OutputCallback
{
public:
  OutputCallback(int fd_) : fd(fd_)
  {
  }
  virtual ~OutputCallback()
  {
    close(fd);
  }
  virtual void operator() (const void* buf, size_t count)
  {
    ssize_t rc;
    rc = write(fd, buf, count);
    if (rc < 0)
      throw std::runtime_error("write() failed");
  }

private:
  int fd;
};

int main(int, char** argv)
try
{
  for(size_t req_counter = 0;;)
  {
    // Accept a connection.

    int       socket;
    sockaddr  sa;
    socklen_t sa_len = sizeof(sa);

    socket = accept(0, &sa, &sa_len);
    if (socket < 0)
      throw std::runtime_error("accept() failed.");

    // Setup the FCGI protocol driver.

    OutputCallback op_cb(socket);
    FCGIProtocolDriver driver(op_cb);

    // Read input from socket and put it into the protocol driver
    // for processing.

    FCGIRequest* req = 0;
    do
    {
      char buf[4*1024];
      ssize_t rc = read(socket, buf, sizeof(buf));
      if (rc < 0)
        throw std::runtime_error("read() failed.");
      driver.process_input(buf, rc);
      if (req == 0)
        req = driver.get_request();
    }
    while (req == 0);

    // Make sure we don't get a keep-connection request.

    if (req->keep_connection)
    {
      std::cerr << "Test program can't handle KEEP_CONNECTION." << std::endl;
      req->end_request(1, FCGIRequest::CANT_MPX_CONN);
      continue;
    }

    // Make sure we are a responder.

    if (req->role != FCGIRequest::RESPONDER)
    {
      std::cerr << "Test program can't handle any role but RESPONDER." << std::endl;
      req->end_request(1, FCGIRequest::UNKNOWN_ROLE);
      continue;
    }

    // Print page with the environment details.

    std::cerr << argv[0] << "[" << getpid() << "]: Starting to handle request #" << req->id << "." << std::endl;
    ++req_counter;
    std::ostringstream os;
    os << "Content-type: text/html\r\n"
       << "\r\n"
       << "<title>FastCGI Test Program</title>" << std::endl
       << "<h1 align=center>FastCGI Test Program</h1>" << std::endl
       << "<h3>FastCGI Status</h3>" << std::endl
       << "Test Program Compile Time = " << __DATE__ " " __TIME__ << "<br>" << std::endl
       << "Process id                = " << getpid() << "<br>" << std::endl
       << "Request socket            = " << socket << "<br>" << std::endl
       << "Request id                = " << req->id << "<br>" << std::endl
       << "Request number            = " << req_counter << "<br>" << std::endl
       << "<h3>Request Environment</h3>" << std::endl;
    for (std::map<std::string,std::string>::const_iterator i = req->params.begin(); i != req->params.end(); ++i)
      os << i->first << "&nbsp;=&nbsp;" << i->second << "<br>" << std::endl;
    req->write(os.str().data(), os.str().size());

    // Make sure we read the entire standard input stream, then
    // echo it back.

    req->write("<h3>Input Stream</h3>\n" \
              "<pre>\n");
    while(req->stdin_eof == false)
    {
      char buf[4*1024];
      ssize_t rc = read(socket, buf, sizeof(buf));
      if (rc < 0)
        throw std::runtime_error("read() failed.");
      driver.process_input(buf, rc);
    }
    if (req->stdin_stream.empty() == false)
    {
      req->write(req->stdin_stream);
      req->stdin_stream.erase();
    }
    req->write("</pre>\n");

    // Terminate the request.

    std::cerr << argv[0] << "[" << getpid() << "]: Request #" << req->id << " handled successfully." << std::endl;
    req->end_request(0, FCGIRequest::REQUEST_COMPLETE);
  }

  // done

  std::cerr << "FastCGI test program terminating." << std::endl;
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
