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

//
// Forward declarations.
//
class FCGIProtocolDriver;
class FCGIRequest;

//
// Exceptions the FCGI code throws.
//

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


//
// The FCGI request class.
//

class FCGIRequest
    {
  public:
    const u_int16_t role;
    const bool keep_connection;
    const bool have_all_params;
    const bool aborted;
    map<string,string> params;

    FCGIRequest(FCGIProtocolDriver& driver_, u_int16_t id_, u_int16_t role_, bool kc);
    ~FCGIRequest();
    void read(string& buf);
    void write(const string& buf);
    void end_request(u_int32_t appStatus, u_int8_t protStatus);

  protected:
    friend class FCGIProtocolDriver;
    string InputBuffer;

  private:
    FCGIProtocolDriver& driver;
    u_int16_t id;
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
	virtual ssize_t operator() (const void*, size_t) = 0;
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
    typedef map<u_int16_t,FCGIRequest*> reqmap_t;
    reqmap_t reqmap;
    queue<u_int16_t> new_request_queue;
    basic_string<u_int8_t> InputBuffer;

  public:
    struct Header
	{
	u_int8_t version;
	u_int8_t type;
	u_int8_t requestIdB1;
	u_int8_t requestIdB0;
	u_int8_t contentLengthB1;
	u_int8_t contentLengthB0;
	u_int8_t paddingLength;
	u_int8_t reserved;
	};
    static const u_int8_t TYPE_BEGIN_REQUEST     =  1;
    static const u_int8_t TYPE_ABORT_REQUEST     =  2;
    static const u_int8_t TYPE_END_REQUEST       =  3;
    static const u_int8_t TYPE_PARAMS            =  4;
    static const u_int8_t TYPE_STDIN             =  5;
    static const u_int8_t TYPE_STDOUT            =  6;
    static const u_int8_t TYPE_STDERR            =  7;
    static const u_int8_t TYPE_DATA              =  8;
    static const u_int8_t TYPE_GET_VALUES        =  9;
    static const u_int8_t TYPE_GET_VALUES_RESULT = 10;
    static const u_int8_t TYPE_UNKNOWN           = 11;

    struct BeginRequest
	{
	u_int8_t roleB1;
	u_int8_t roleB0;
	u_int8_t flags;
	u_int8_t reserved[5];
	};
    static const u_int8_t  FLAG_KEEP_CONN  = 1;
    static const u_int16_t ROLE_RESPONDER  = 1;
    static const u_int16_t ROLE_AUTHORIZER = 2;
    static const u_int16_t ROLE_FILTER     = 3;

    struct EndRequestMsg
	{
	Header   header;
	u_int8_t appStatusB3;
	u_int8_t appStatusB2;
	u_int8_t appStatusB1;
	u_int8_t appStatusB0;
	u_int8_t protocolStatus;
	u_int8_t reserved[3];
        };

    static const u_int8_t REQUEST_COMPLETE = 0;
    static const u_int8_t CANT_MPX_CONN    = 1;
    static const u_int8_t OVERLOADED       = 2;
    static const u_int8_t UNKNOWN_ROLE     = 3;
    };

#endif // !defined(__FASTCGI_HPP__)
