/*
 * $Source: /home/cvs/fastcgi-example/infrastructure.hpp,v $
 * $Revision: 1.1 $
 * $Date: 2001/03/20 17:38:49 $
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#ifndef __INFRASTRUCTURE_HPP__
#define __INFRASTRUCTURE_HPP__

#include <sys/types.h>
#include <sys/socket.h>
#include "libscheduler/scheduler.hpp"
#include "libfastcgi/fastcgi.hpp"

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
	    throw runtime_error(string("accept() failed: ") + strerror(errno));
	}
    virtual void fd_is_writable(int)
	{
	throw logic_error("This routine should not be called.");
	}
    virtual void read_timeout(int)
	{
	throw logic_error("This routine should not be called.");
	}
    virtual void write_timeout(int)
	{
	throw logic_error("This routine should not be called.");
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
	cerr << "Creating new ConnectionHandler instance for socket " << mysocket << "." << endl;
	properties.poll_events  = POLLIN;
	properties.read_timeout = 0;
	mysched.register_handler(mysocket, *this, properties);
	}
    ~ConnectionHandler()
	{
	cerr << "Destroying ConnectionHandler instance for socket " << mysocket << "." << endl;
	mysched.remove_handler(mysocket);
	close(mysocket);
	}

  private:
    virtual void operator() (const void* buf, size_t count)
	{
	int rc = write(mysocket, buf, count);
	if (rc < 0)
	    {
	    char tmp[128];
	    snprintf(tmp, sizeof(tmp), "An error occured while writing to fd %d.", mysocket);
	    throw fcgi_io_callback_error(tmp);
	    }
	else if (static_cast<size_t>(rc) < count)
	    {
	    write_buffer.append(static_cast<char*>(buf)+rc, count-rc);
	    if (is_write_handler_registered == false)
		{
		properties.poll_events  = POLLIN | POLLOUT;
		properties.write_timeout = 0;
		mysched.register_handler(mysocket, *this, properties);
		is_write_handler_registered = true;
		}
	    }
	}
    virtual void fd_is_readable(int fd)
	{
	char tmp[1024];
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
	    catch(const exception& e)
		{
		cerr << "Caught exception thrown in FCGIProtocolDriver: " << e.what() << endl;
		cerr << "Terminating connection " << mysocket << "." << endl;
		delete this;
		}
	    catch(...)
		{
		cerr << "Caught unknown exception in FCGIProtocolDriver; terminating connection "
		     << mysocket << "." << endl;
		delete this;
		}
	    }
	else if (rc <= 0)
	    {
	    cerr << "An error occured while reading from fd " << mysocket << "." << endl;
	    mysched.remove_handler(mysocket);
	    delete this;
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
	    else if (rc < 0)
		{
		cerr << "An error occured while writing to fd " << mysocket << "." << endl;
		mysched.remove_handler(mysocket);
		delete this;
		}
	    }
	terminate_if_we_shall();
	}
    virtual void read_timeout(int)
	{
	throw logic_error("Not implemented yet.");
	}
    virtual void write_timeout(int)
	{
	throw logic_error("Not implemented yet.");
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
    string write_buffer;
    bool terminate;
    };

#endif // !defined( __INFRASTRUCTURE_HPP__)
