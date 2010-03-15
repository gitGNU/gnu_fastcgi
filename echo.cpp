/*
 * Copyright (c) 2001-2010 Peter Simons <simons@cryp.to>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "fastcgi.hpp"
#include "ioxx/dispatch.hpp"
#include <boost/shared_ptr.hpp>
#include <iostream>             // ISO C++
#include <sstream>
#include <stdexcept>

template<class IOCore, class RequestHandler>
class ConnectionHandler : public FCGIProtocolDriver::OutputCallback
{
public:
  typedef IOCore                        io_core;
  typedef typename io_core::socket      socket;
  typedef typename socket::address      address;
  typedef typename socket::event_set    event_set;
  typedef typename socket::native_t     native_socket_t;

  static void accept(io_core & io, socket & listen_socket)
  {
    boost::shared_ptr<ConnectionHandler> p;
    {
      native_socket_t s;
      address addr;
      listen_socket.accept(s, addr);
      p.reset(new ConnectionHandler(io, s));
    }
    p->_sock.modify(boost::bind(&ConnectionHandler::run, p, _1), socket::readable);
    p->_sock.set_nonblocking();
    p->_sock.set_linger_timeout(0);
  }

private:
  socket                _sock;
  FCGIProtocolDriver    _fcgi_driver;
  bool                  _terminate;
  std::vector<char>     _write_buffer;

  ConnectionHandler(io_core & io, native_socket_t sock)
  : _sock(io, sock), _fcgi_driver(*this), _terminate(false)
  {
  }

  void shutdown()
  {
    _sock.modify(typename io_core::dispatch::handler(), socket::no_events);
  }

  void run(event_set ev)
  {
    try
    {
      if (ev & socket::readable)
      {
        char tmp[1024*10];
        char * data_end( _sock.read(tmp, tmp + sizeof(tmp)) );
        if (!data_end) return shutdown();
        _fcgi_driver.process_input(tmp, data_end - tmp);
        FCGIRequest* req( _fcgi_driver.get_request() );
        if (req)
        {
          _terminate = (req->keep_connection == false);
          req->handler_cb = new RequestHandler;
          req->handler_cb->operator()(req);
        }
      }
      if (ev & socket::writable)
      {
        if (!_write_buffer.empty())
        {
          char const * p( _sock.write(&_write_buffer[0], &_write_buffer[_write_buffer.size()]) );
          if (!p) throw ioxx::system_error(errno, "cannot write to socket");
          _write_buffer.erase(_write_buffer.begin(), _write_buffer.begin() + (p - &_write_buffer[0]));
        }
        if (_write_buffer.empty())
          _sock.request(socket::readable);
      }
      if (_terminate && !_fcgi_driver.have_active_requests() && _write_buffer.empty())
        shutdown();
    }
    catch(std::exception const & e)
    {
      std::cerr << "Caught exception in FCGIProtocolDriver: " << e.what() << std::endl
                << "Terminating connection " << _sock << "." << std::endl;
      return shutdown();
    }
    catch(...)
    {
      std::cerr << "Caught unknown exception in FCGIProtocolDriver; terminating connection "
                << _sock << "." << std::endl;
      return shutdown();
    }
  }

  virtual void operator() (void const * buf, size_t count)
  {
    bool const start_writer( _write_buffer.empty() );
    _write_buffer.insert(_write_buffer.end(), static_cast<char const *>(buf), static_cast<char const *>(buf) + count);
    if (start_writer)
      _sock.request(socket::readable | socket::writable);
  }
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

///// main program ////////////////////////////////////////////////////////////

int main(int, char**)
try
{
  typedef ioxx::dispatch<>                              io_core;
  typedef io_core::socket                               socket;
  typedef ConnectionHandler<io_core, RequestHandler>    fcgi_handler;
  using boost::ref;

  // The main i/o event dispatcher.
  io_core io;

  // Accept FastCGI connections on stdin.
  io_core::socket acceptor( io, STDIN_FILENO );
  acceptor.close_on_destruction(false);
  acceptor.modify(boost::bind(fcgi_handler::accept, ref(io), ref(acceptor)), socket::readable);

  // The main i/o loop.
  for (;;)
  {
    io.run();
    io.wait(io.max_timeout());
  }
  return 0;
}
catch(std::exception const & e)
{
  std::cerr << "*** error: " << e.what() << std::endl;
  return 1;
}
catch(...)
{
  std::cerr << "*** unknown error; terminating" << std::endl;
  return 2;
}
