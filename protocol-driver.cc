/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hpp"

const FCGIProtocolDriver::proc_func_t FCGIProtocolDriver::proc_funcs[] =
    {
    0,				                // unused
    &FCGIProtocolDriver::process_begin_request,	// TYPE_BEGIN_REQUEST
    &FCGIProtocolDriver::process_abort_request,	// TYPE_ABORT_REQUEST
    0,				                // TYPE_END_REQUEST
    &FCGIProtocolDriver::process_params,	// TYPE_PARAMS
    &FCGIProtocolDriver::process_stdin,		// TYPE_STDIN
    0,				                // TYPE_STDOUT
    0,				                // TYPE_STDERR
    0,				                // TYPE_DATA
    0,				                // TYPE_GET_VALUES
    0,				                // TYPE_GET_VALUES_RESULT
    0,				                // TYPE_UNKNOWN
    };

FCGIProtocolDriver::FCGIProtocolDriver(OutputCallback& cb) : output_cb(cb)
    {
    }

FCGIProtocolDriver::~FCGIProtocolDriver()
    {
    for(reqmap_t::iterator i = reqmap.begin(); i != reqmap.end(); ++i)
	{
	delete i->second;
	}
    }

void FCGIProtocolDriver::process_input(const void* buf, size_t count)
    {
    // Copy data to our own buffer.

    InputBuffer.append(static_cast<const u_int8_t*>(buf), count);

    // If there is enough data in the input buffer to contain a
    // header, interpret it.

    while(InputBuffer.size() >= sizeof(Header))
	{
	const Header* hp  = reinterpret_cast<const Header*>(InputBuffer.data());

	// Check whether our peer speaks the correct protocol version.

	if (hp->version != 1)
	    {
	    char buf[256];
	    sprintf(buf, "FCGIProtocolDriver cannot handle protocol version %u.", hp->version);
	    throw unsupported_fcgi_version(buf);
	    }

	// Check whether we have the whole message that follows the
	// headers in our buffer already. If not, we can't process it
	// yet.

	u_int16_t msg_len = (hp->contentLengthB1 << 8) + hp->contentLengthB0;
	u_int16_t msg_id  = (hp->requestIdB1 << 8) + hp->requestIdB0;

	if (InputBuffer.size() < sizeof(Header)+msg_len+hp->paddingLength)
	    return;

	// Process the message. That involves a lookup in the
	// proc_funcs array, using the message type as index. Then the
	// function is called. Use a sentry to make sure the message
	// is removed from the buffer in case of on exception.

	class sentry
	    {
	    basic_string<u_int8_t>& buf;
	    size_t count;
	  public:
	    sentry(basic_string<u_int8_t>& b, size_t c) : buf(b), count(c) { }
	    ~sentry() { buf.erase(0, count); }
	    }
	s(InputBuffer, sizeof(Header)+msg_len+hp->paddingLength);

	cerr << "Received message: id = " << msg_id << ", "
	     << "body len = " << msg_len << ", "
	     << "type = " << (int)hp->type << endl;

	if (hp->type > TYPE_UNKNOWN || proc_funcs[hp->type] == 0)
	    process_unknown(hp->type);
	else
	    (this->*proc_funcs[hp->type])(msg_id, InputBuffer.data()+sizeof(Header), msg_len);
	}
    }

FCGIRequest* FCGIProtocolDriver::get_request(void)
    {
    if (new_request_queue.empty())
	return 0;

    FCGIRequest* r = reqmap[new_request_queue.front()];
    new_request_queue.pop();
    return r;
    }

void FCGIProtocolDriver::terminate_request(u_int16_t id)
    {
    reqmap_t::iterator req;
    req = reqmap.find(id);
    if (req != reqmap.end())
	{
	delete req->second;
	reqmap.erase(req);
	}
    }

// Pure virtual member functions must exist nonetheless.

FCGIProtocolDriver::OutputCallback::~OutputCallback()
    {
    }
