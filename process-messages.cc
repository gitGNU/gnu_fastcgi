/*
 * Copyright (c) 2001 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hh"

void FCGIProtocolDriver::process_begin_request(u_int16_t id, const u_int8_t* buf, u_int16_t)
    {
    // Check whether we have an open request with that id already and
    // if, throw an exception.

    if (reqmap.find(id) != reqmap.end())
	{
	char tmp[256];
	sprintf(tmp, "FCGIProtocolDriver received duplicate BEGIN_REQUEST id %u.", id);
	throw duplicate_begin_request(tmp);
	}

    // Create a new request instance and store it away. The user may
    // get it after we've read all parameters.

    const BeginRequest* br = reinterpret_cast<const BeginRequest*>(buf);
    reqmap[id] = new FCGIRequest(*this, id,
				 FCGIRequest::role_t((br->roleB1 << 8) + br->roleB0),
				 (br->flags & FLAG_KEEP_CONN) == 1);
    }

void FCGIProtocolDriver::process_abort_request(u_int16_t id, const u_int8_t*, u_int16_t)
    {
    // Find request instance for this id. Ignore message if non
    // exists, set ignore flag otherwise.

    reqmap_t::iterator req = reqmap.find(id);
    if (req == reqmap.end())
	std::cerr << "FCGIProtocolDriver received ABORT_REQUEST for non-existing id " << id << ". Ignoring."
		  << std::endl;
    else
	{
	req->second->aborted = true;
	if (req->second->handler_cb) // Notify the handler associated with this request.
	    (*req->second->handler_cb)(req->second);
	}
    }

void FCGIProtocolDriver::process_params(u_int16_t id, const u_int8_t* buf, u_int16_t len)
    {
    // Find request instance for this id. Ignore message if non
    // exists.

    reqmap_t::iterator req = reqmap.find(id);
    if (req == reqmap.end())
	{
	std::cerr << "FCGIProtocolDriver received PARAMS for non-existing id " << id << ". Ignoring."
		  << std::endl;
	return;
	}

    // Is this the last message to come? Then queue the request for
    // the user.

    if (len == 0)
	{
	new_request_queue.push(id);
	return;
	}

    // Process message.

    u_int32_t   name_len, data_len;
    std::string name, data;
    if (*buf >> 7 == 0)
	name_len = *(buf++);
    else
	{
	name_len = ((buf[0] & 0x7F) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
	buf += 4;
	}
    if (*buf >> 7 == 0)
	data_len = *(buf++);
    else
	{
	data_len = ((buf[0] & 0x7F) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
	buf += 4;
	}
    name.assign(reinterpret_cast<const char*>(buf), name_len);
    data.assign(reinterpret_cast<const char*>(buf)+name_len, data_len);
    req->second->params[name] = data;
    }
