/*
 * $Source: /home/cvs/fastcgi-example/echo.cpp,v $
 * $Revision: 1.2 $
 * $Date: 2001/03/20 17:42:34 $
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
	if (!req->stdin_eof)
	    return;

	// Make sure we are a responder.

	if (req->role != FCGIRequest::RESPONDER)
	    {
	    req->write("We can't handle any role but RESPONDER.", FCGIRequest::STDERR);
	    req->end_request(1, FCGIRequest::UNKNOWN_ROLE);
	    return;
	    }

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
	os << "<h3>Input Stream</h3>\n"
	   << "<pre>\n";
	req->write(os.str(), os.pcount());
	os.freeze(0);

	for (size_t i = 0; i < req->stdin_stream.size(); )
	    {
	    size_t count = ((req->stdin_stream.size() - i) > 0xffff) ? 0xffff : req->stdin_stream.size()-i;
	    req->write(req->stdin_stream.data()+i, count);
	    i += count;
	    }

	req->write("</pre>\n");
	req->write("<h1 align=center>End of Request</h1>\n");
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
