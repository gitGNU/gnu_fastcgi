/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hpp"

void FCGIProtocolDriver::process_stdin(u_int16_t id, const u_int8_t* buf, u_int16_t len)
    {
    // Find request instance for this id. Ignore message if non
    // exists.

    reqmap_t::iterator req = reqmap.find(id);
    if (req == reqmap.end())
	{
	cerr << "FCGIProtocolDriver received STDIN for non-existing id " << id << ". Ignoring."
	     << endl;
	return;
	}

    // Is this the last message to come? Then set the eof flag.

    if (len == 0)
	{
	req->second->stdin_eof = true;
	return;
	}

    // Add data to stream.

    req->second->stdin_stream.append((const char*)buf, len);
    }
