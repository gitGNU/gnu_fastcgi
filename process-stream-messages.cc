/*
 * Copyright (c) 2001 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hh"

void FCGIProtocolDriver::process_stdin(u_int16_t id, const u_int8_t* buf, u_int16_t len)
    {
    // Find request instance for this id. Ignore message if non
    // exists.

    reqmap_t::iterator req = reqmap.find(id);
    if (req == reqmap.end())
	{
	std::cerr << "FCGIProtocolDriver received STDIN for non-existing id " << id << ". Ignoring."
		  << std::endl;
	return;
	}

    // Is this the last message to come? Then set the eof flag.
    // Otherwise, add the data to the buffer in the request structure.

    if (len == 0)
	req->second->stdin_eof = true;
    else
	req->second->stdin_stream.append((const char*)buf, len);

    // Notify the handler associated with this request.

    if (req->second->handler_cb)
	(*req->second->handler_cb)(req->second);
    }
