/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include <iostream>
#include <stdexcept>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include "fastcgi.hpp"

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
    virtual ssize_t operator() (const void* buf, size_t count)
	{
	ssize_t rc;
	rc = write(fd, buf, count);
	if (rc < 0)
	    throw runtime_error("write() failed");
	return rc;
	}

  private:
    int fd;
    };


int main()
try {
    for(;;)
	{
	// Accept a connection.

	int       socket;
	sockaddr  sa;
	socklen_t sa_len = sizeof(sa);

	socket = accept(0, &sa, &sa_len);
	if (socket < 0)
	    throw runtime_error("accept() failed.");

	// Setup the FCGI protocol driver.

	OutputCallback op_cb(socket);
	FCGIProtocolDriver driver(op_cb);

	// Read input from socket and put it into the protocol driver
	// for processing.

	sleep(1);
	char buf[4*1024];
	ssize_t rc = read(socket, buf, sizeof(buf));
	if (rc < 0)
	    throw runtime_error("read() failed.");
	driver.process_input(buf, rc);
	FCGIRequest* req = driver.get_request();
	if (req->keep_connection)
	    throw runtime_error("Test program can't handle KEEP_CONNECTION.");
	if (req->role != FCGIProtocolDriver::ROLE_RESPONDER)
	    throw runtime_error("Test program can't handle any role but RESPONDER.");

	req->write("Content-type: text/html\r\n" \
		   "\r\n"
		   "<title>FastCGI echo</title>"
		   "<h1>FastCGI echo</h1>\n");
	req->end_request(0, FCGIProtocolDriver::REQUEST_COMPLETE);
	}

    // done

    return 0;
    }
catch(const exception &e)
    {
    cerr << "Caught exception: " << e.what() << endl;
    return 1;
    }
catch(...)
    {
    cerr << "Caught unknown exception." << endl;
    return 1;
    }
