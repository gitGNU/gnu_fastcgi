/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#ifndef __FASTCGI_HPP__
#define __FASTCGI_HPP__

#include <cstdio>
#include <unistd.h>
#include <map>
#include <queue>
#include <string>
#include <stdexcept>
#include <new>

// Forward declarations.

class FCGIProtocolDriver;
class FCGIRequest;

// Exceptions we throw.

struct fcgi_error : public runtime_error
    {
    fcgi_error(const string& w) : runtime_error(w) { }
    virtual ~fcgi_error() = 0;
    };

struct unsupported_fcgi_version : public fcgi_error
    {
    unsupported_fcgi_version(const string& w) : fcgi_error(w) { }
    };

struct duplicate_begin_request : public fcgi_error
    {
    duplicate_begin_request(const string& w) : fcgi_error(w) { }
    };

struct unknown_fcgi_request : public fcgi_error
    {
    unknown_fcgi_request(const string& w) : fcgi_error(w) { }
    };

// The class representing a request.

class FCGIRequest
    {
  public:
    enum role_t
	{
	RESPONDER  = 1,
	AUTHORIZER = 2,
	FILTER     = 3
	};

    const u_int16_t id;
    const role_t role;
    const bool keep_connection;
    bool aborted;
    map<string,string> params;
    string stdin_stream, data_stream;
    bool stdin_eof, data_eof;

    FCGIRequest(FCGIProtocolDriver& driver_, u_int16_t id_, role_t role_, bool kc);
    ~FCGIRequest();

    enum ostream_type_t
	{
	STDOUT,
	STDERR
	};
    void write(const string& buf, ostream_type_t stream = STDOUT);
    void write(const void* buf, size_t cout, ostream_type_t stream = STDOUT);

    enum protocol_status_t
	{
	REQUEST_COMPLETE = 0,
	CANT_MPX_CONN    = 1,
	OVERLOADED       = 2,
	UNKNOWN_ROLE     = 3
	};
    void end_request(u_int32_t appStatus, protocol_status_t protStatus);

  private:
    FCGIProtocolDriver& driver;
    u_int8_t tmp_buf[64];
    };

//
// The FCGI protocol driver class.
//

class FCGIProtocolDriver
    {
  public:
    struct OutputCallback
	{
	virtual ~OutputCallback() = 0;
	virtual void operator() (const void*, size_t) = 0;
	};

  public:
    FCGIProtocolDriver(OutputCallback& cb);
    ~FCGIProtocolDriver();

    void process_input(const void* buf, size_t count);
    FCGIRequest* get_request(void);

  private:			// don't copy me
    FCGIProtocolDriver(const FCGIProtocolDriver&);
    FCGIProtocolDriver& operator= (const FCGIProtocolDriver&);

  protected:
    friend FCGIRequest;
    void terminate_request(u_int16_t id);
    OutputCallback& output_cb;

  private:
    typedef void (FCGIProtocolDriver::* proc_func_t)(u_int16_t, const u_int8_t*, u_int16_t);
    static const proc_func_t proc_funcs[];

    void process_begin_request(u_int16_t id, const u_int8_t* buf, u_int16_t len);
    void process_abort_request(u_int16_t id, const u_int8_t* buf, u_int16_t len);
    void process_params(u_int16_t id, const u_int8_t* buf, u_int16_t len);
    void process_stdin(u_int16_t id, const u_int8_t* buf, u_int16_t len);
    void process_unknown(u_int8_t type);

    typedef map<u_int16_t,FCGIRequest*> reqmap_t;
    reqmap_t reqmap;
    queue<u_int16_t> new_request_queue;

    basic_string<u_int8_t> InputBuffer;
    u_int8_t tmp_buf[64];
    };

#endif // !defined(__FASTCGI_HPP__)
