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
#include <strstream>

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

	FCGIRequest* req = 0;
	do
	    {
	    char buf[4*1024];
	    ssize_t rc = read(socket, buf, sizeof(buf));
	    if (rc < 0)
		throw runtime_error("read() failed.");
	    driver.process_input(buf, rc);
	    if (req == 0)
		req = driver.get_request();
	    }
	while (req == 0 || req->have_all_params == false);

	// Make sure we don't get a keep-connection request.

	if (req->keep_connection)
	    {
	    cerr << "Test program can't handle KEEP_CONNECTION." << endl;
	    req->end_request(1, FCGIProtocolDriver::CANT_MPX_CONN);
	    continue;
	    }

	// Make sure we are a responder.

	if (req->role != FCGIProtocolDriver::ROLE_RESPONDER)
	    {
	    cerr << "Test program can't handle any role but RESPONDER." << endl;
	    req->end_request(1, FCGIProtocolDriver::UNKNOWN_ROLE);
	    continue;
	    }

	// Handle request.

	cerr << "Starting to handle request #" << req->id << endl;
	ostrstream os;
	os << "Content-type: text/html\r\n"
	   << "\r\n"
	   << "<title>FastCGI Test Program</title>" << endl
	   << "<h1 align=center>FastCGI Test Program</h1>" << endl
	   << "<h3>FastCGI Status</h3>" << endl
	   << "Process id = " << getpid() << "<br>" << endl
	   << "Request id = " << req->id << endl
	   << "<h3>Request Environment</h3>" << endl;

	for (map<string,string>::const_iterator i = req->params.begin(); i != req->params.end(); ++i)
	    os << i->first << "&nbsp;=&nbsp;" << i->second << "<br>" << endl;

	req->write(os.str(), os.pcount());
	os.freeze(0);
	cerr << "Request #" << req->id << " handled successfully." << endl;
	req->end_request(0, FCGIProtocolDriver::REQUEST_COMPLETE);
	}

    // done

    cerr << "FastCGI test program terminating." << endl;
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
