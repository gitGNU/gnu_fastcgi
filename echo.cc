/*
 * Copyright (c) 2005 by Peter Simons <simons@cryp.to>.
 *
 * Copying and distribution of this file, with or without
 * modification, are permitted in any medium without royalty
 * provided the copyright notice and this notice are preserved.
 *
 * See <http://cryp.to/libfastcgi/> for the latest version.
 */

#include <iostream>             // ISO C++
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>       // POSIX.1
#include <unistd.h>
#include "infrastructure.hh"    // program internals

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
       << "<title>FastCGI Test Program</title>" << std::endl
       << "<h1 align=center>FastCGI Test Program</h1>" << std::endl
       << "<h3>FastCGI Status</h3>" << std::endl
       << "Test Program Compile Time = " << __DATE__ " " __TIME__ << "<br>" << std::endl
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
