/*
 * $Source: /home/cvs/fastcgi-example/echo.cpp,v $
 * $Revision: 1.1 $
 * $Date: 2001/03/20 17:38:49 $
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

// ISO C++ header files.
#include <iostream>
#include <strstream>
#include <stdexcept>
#include <string>

// POSIX.1 header files.
#include <sys/resource.h>
#include <unistd.h>

// Programm-internal stuff.
#include "infrastructure.hpp"

class RequestHandler : public FCGIRequest::handler
    {
  private:
    virtual void operator()(FCGIRequest* req)
	{
	cerr << "Handling request #" << req->id << "." << endl;

	// Make sure we are a responder.

	if (req->role != FCGIRequest::RESPONDER)
	    {
	    cerr << "Test program can't handle any role but RESPONDER." << endl;
	    req->end_request(1, FCGIRequest::UNKNOWN_ROLE);
	    return;
	    }

	// Handle KEEP_CONNECTION.
	//
	//if (req->keep_connection)
	//terminate_connection = false;

	// Do we have everything we expected from STDIN?

	if (!req->stdin_eof)
	    return;

	// Print page with the environment details.

	ostrstream os;
	os << "Content-type: text/html\r\n"
	   << "\r\n"
	   << "<title>FastCGI Test Program</title>" << endl
	   << "<h1 align=center>FastCGI Test Program</h1>" << endl
	   << "<h3>FastCGI Status</h3>" << endl
	   << "Test Program Compile Time = " << __DATE__ " " __TIME__ << "<br>" << endl
	   << "Process id                = " << getpid() << "<br>" << endl
	   << "Request id                = " << req->id << "<br>" << endl
	   << "<h3>Request Environment</h3>" << endl;
	for (map<string,string>::const_iterator i = req->params.begin(); i != req->params.end(); ++i)
	    os << i->first << "&nbsp;=&nbsp;" << i->second << "<br>" << endl;
	req->write(os.str(), os.pcount());
	os.freeze(0);

	// Make sure we read the entire standard input stream, then
	// echo it back.

	req->write("<h3>Input Stream</h3>\n" \
		   "<pre>\n");
	req->write(req->stdin_stream);
	req->stdin_stream.erase();
	req->write("</pre>\n");

	// Terminate the request.

	cerr << "Request #" << req->id << " handled successfully." << endl;
	req->end_request(0, FCGIRequest::REQUEST_COMPLETE);
	}
    };

int main(int, char** argv)
try
    {
    // Set a limit on the number of open files.

    rlimit rlim;
    getrlimit(RLIMIT_NOFILE, &rlim);
    rlim.rlim_cur = 8;
    rlim.rlim_max = 8;
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0)
	throw runtime_error("setrlimit() failed.");

    // Let's go.

    scheduler sched;
    Listener< ConnectionHandler<RequestHandler> > listener(sched);
    while(!sched.empty())
	{
	sched.schedule();
	//sched.dump(cerr);
	}
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
