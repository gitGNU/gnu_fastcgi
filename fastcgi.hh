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

class FCGIProtocolDriver
    {
  public:
    struct OutputCallback
	{
	virtual ~OutputCallback() = 0;
	virtual ssize_t operator() (const void*, size_t) = 0;
	};

    FCGIProtocolDriver(OutputCallback& cb) : output_cb(cb)
	{
	}
    ~FCGIProtocolDriver()
	{
	}

    void process_input(const void* buf, size_t count)
	{
	cerr << __FUNCTION__ << ": got " << count << " byte input." << endl;

	// Copy data to our own puffer.

	InputBuffer.append(static_cast<const u_int8_t*>(buf), count);

	// If there is enough data in the input buffer to contain a
	// header, interpret it.

	while(InputBuffer.size() > sizeof(Header))
	    {
	    const Header* hp  = reinterpret_cast<const Header*>(InputBuffer.data());

	    // Check whether our peer speaks the correct protocol
	    // version.

	    if (hp->version != 1)
		{
		char buf[256];
		sprintf(buf, "FCGIProtocolDriver cannot handle protocol version %u.", hp->version);
		throw invalid_argument(buf);
		}

	    // Check whether we have the whole message that follows
	    // the headers in our buffer already. If not, we can't
	    // process it yet.

	    u_int16_t msg_len = (hp->contentLengthB1 << 8) + hp->contentLengthB0;
	    u_int16_t msg_id  = (hp->requestIdB1 << 8) + hp->requestIdB0;

	    if (InputBuffer.size() < sizeof(Header)+msg_len+hp->paddingLength)
		break;

	    // Process the message.

	    try {
	        reqmap_t::iterator req;

		switch(hp->type)
		    {
		    case TYPE_BEGIN_REQUEST:
			// Create a new request instance and put it
			// into the queue so that the user may get it.

			if (reqmap.find(msg_id) != reqmap.end())
			    {
			    char buf[256];
			    sprintf(buf, "FCGIProtocolDriver received BEGIN_REQUEST id %u, which " \
				    "exists already.", hp->type);
			    throw logic_error(buf);
			    }
			else
			    {
			    const BeginRequest* br =
				reinterpret_cast<const BeginRequest*>(InputBuffer.data()+sizeof(Header));
			    u_int16_t role  = (br->roleB1 << 8) + br->roleB0;

			    cerr << "TYPE_BEGIN_REQUEST: id = " << msg_id
				 << ", role = " << role
				 << ", flags = " << ((br->flags & FLAG_KEEP_CONN) == 1 ? "KEEP_CONN" : "none")
				 << endl;

			    reqmap[msg_id] = new FCGIRequest(*this, msg_id, role,
							     (br->flags & FLAG_KEEP_CONN) == 1);
			    new_request_queue.push(msg_id);
			    }
			break;

		    case TYPE_ABORT_REQUEST:
			// Find the request in the reqmap and set the
			// abort flag.
			cerr << "Received message type TYPE_ABORT_REQUEST, id " << msg_id << "." << endl;
			req = reqmap.find(msg_id);
			if (req != reqmap.end())
			    {
			    req->second->abort();
			    }
			else
			    cerr << "FCGIProtocolDriver received ABORT_REQUEST for id "
				 << msg_id << ", which does not exist. Ignoring."
				 << endl;
			break;

		    case TYPE_PARAMS:
			// Find the request in the reqmap and set the
			// environment.

			cerr << "Received message type TYPE_PARAMS, id " << msg_id << "." << endl;
			req = reqmap.find(msg_id);
			if (req != reqmap.end())
			    {
			    // Is this the last message to come? Then
			    // set the have_all_params flag in the
			    // request object.

			    if (msg_len == 0)
				{
				const_cast<bool&>(req->second->have_all_params) = true;
				break;
				}
			    // Process message.

			    const u_int8_t* p = InputBuffer.data()+sizeof(Header);
			    u_int32_t  name_len, data_len;
			    string     name, data;
			    if (*p >> 7 == 0)
				name_len = *(p++);
			    else
				{
				name_len = ((p[0] & 0x7F) << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
				p += 4;
				}
			    if (*p >> 7 == 0)
				data_len = *(p++);
			    else
				{
				data_len = ((p[0] & 0x7F) << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
				p += 4;
				}
			    name.assign(reinterpret_cast<const char*>(p), name_len);
			    data.assign(reinterpret_cast<const char*>(p)+name_len, data_len);
			    cerr << "Setting " << name << " = " << data << endl;
			    req->second->params[name] = data;
			    }
			else
			    cerr << "FCGIProtocolDriver received PARAMS for id "
				 << msg_id << ", which does not exist. Ignoring."
				 << endl;
			break;

		    case TYPE_STDIN:
			cerr << "Received message type TYPE_STDIN, len " << msg_len << "." << endl;
			break;
		    case TYPE_DATA:
			cerr << "Received message type TYPE_DATA, len " << msg_len << "." << endl;
			break;
		    case TYPE_GET_VALUES:
			cerr << "Received message type TYPE_GET_VALUES, len " << msg_len << "." << endl;
			break;
		    default:
			char buf[256];
			sprintf(buf, "FCGIProtocolDriver received unknown request type %u.", hp->type);
			throw invalid_argument(buf);
		    }

		InputBuffer.erase(0, sizeof(Header)+msg_len+hp->paddingLength);
		}
	    catch(...)
		{
		InputBuffer.erase(0, sizeof(Header)+msg_len+hp->paddingLength);
		throw;
		}
	    }
	}

    class FCGIRequest
	{
      public:
	const u_int16_t role;
	const bool      keep_connection;
	const bool      have_all_params;

	map<string,string> params;

      public:
	FCGIRequest(FCGIProtocolDriver& driver_, u_int16_t id_, u_int16_t role_, bool kc)
		: role(role_), keep_connection(kc), have_all_params(false),
	          driver(driver_), id(id_), aborted(false)
	    {
	    }
	~FCGIRequest()
	    {
	    }
	bool is_aborted(void)
	    {
	    return aborted;
	    }

      protected:
	friend class FCGIProtocolDriver;
	void abort(void) throw()
	    {
	    aborted = true;
	    }

      private:
	FCGIProtocolDriver& driver;
	u_int16_t id;
	bool aborted;
	};

    FCGIRequest* get_request(void)
	{
	if (new_request_queue.empty())
	    return 0;

	FCGIRequest* r = reqmap[new_request_queue.front()];
	new_request_queue.pop();
	return r;
	}


  private:			// don't copy me
    FCGIProtocolDriver(const FCGIProtocolDriver&);
    FCGIProtocolDriver& operator= (const FCGIProtocolDriver&);

  private:
    OutputCallback& output_cb;
    basic_string<u_int8_t> InputBuffer;

    typedef map<u_int16_t, FCGIRequest*> reqmap_t;
    reqmap_t reqmap;
    queue<u_int16_t> new_request_queue;

  private:
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
    static const unsigned int TYPE_BEGIN_REQUEST     =  1;
    static const unsigned int TYPE_ABORT_REQUEST     =  2;
    static const unsigned int TYPE_END_REQUEST       =  3;
    static const unsigned int TYPE_PARAMS            =  4;
    static const unsigned int TYPE_STDIN             =  5;
    static const unsigned int TYPE_STDOUT            =  6;
    static const unsigned int TYPE_STDERR            =  7;
    static const unsigned int TYPE_DATA              =  8;
    static const unsigned int TYPE_GET_VALUES        =  9;
    static const unsigned int TYPE_GET_VALUES_RESULT = 10;
    static const unsigned int TYPE_UNKNOWN           = 11;

    struct BeginRequest
	{
	u_int8_t roleB1;
	u_int8_t roleB0;
	u_int8_t flags;
	u_int8_t reserved[5];
	};
    static const unsigned int FLAG_KEEP_CONN  = 1;
    static const unsigned int ROLE_RESPONDER  = 1;
    static const unsigned int ROLE_AUTHORIZER = 2;
    static const unsigned int ROLE_FILTER     = 3;
    };

FCGIProtocolDriver::OutputCallback::~OutputCallback()
    {
    }


#endif // !defined(__FASTCGI_HPP__)
