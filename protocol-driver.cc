/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hpp"

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

    while(InputBuffer.size() > sizeof(Header))
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

	// Process the message.

	try {
	    if (msg_id == 0)	// Admin message.
		{
		switch(hp->type)
		    {
		    case TYPE_GET_VALUES:
			cerr << "Received message type TYPE_GET_VALUES, len " << msg_len << "." << endl;
			break;
		    default:
			process_unknown(hp->type);
		    }
		}
	    else		// Normal requests.
		{
		switch(hp->type)
		    {
		    case TYPE_BEGIN_REQUEST:
			process_begin_request(msg_id, InputBuffer.data()+sizeof(Header));
			break;
		    case TYPE_ABORT_REQUEST:
			process_abort_request(msg_id);
			break;
		    case TYPE_PARAMS:
			process_params(msg_id, InputBuffer.data()+sizeof(Header), msg_len);
			break;
		    case TYPE_STDIN:
			cerr << "Received message type TYPE_STDIN, len " << msg_len << "." << endl;
			break;
		    case TYPE_DATA:
			cerr << "Received message type TYPE_DATA, len " << msg_len << "." << endl;
			break;
		    default:
			char buf[256];
			sprintf(buf, "FCGIProtocolDriver received unknown request type %u.", hp->type);
			throw unknown_fcgi_request(buf);
		    }
		}

	    // Remove message from input buffer.

	    InputBuffer.erase(0, sizeof(Header)+msg_len+hp->paddingLength);
	    }
	catch(...)
	    {
	    InputBuffer.erase(0, sizeof(Header)+msg_len+hp->paddingLength);
	    throw;
	    }
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
