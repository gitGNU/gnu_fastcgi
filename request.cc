/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "fastcgi.hpp"

FCGIRequest::FCGIRequest(FCGIProtocolDriver& driver_, u_int16_t id_, u_int16_t role_, bool kc)
	: role(role_), keep_connection(kc), have_all_params(false), aborted(false),
	  driver(driver_), id(id_)
    {
    }

FCGIRequest::~FCGIRequest()
    {
    }

void FCGIRequest::read(string& buf)
    {
    buf = InputBuffer;
    InputBuffer.erase();
    }

void FCGIRequest::write(const string& buf)
    {
    if (buf.size() > 0xffff)
	throw out_of_range("Can't send messages of that size.");

    // Construct message.

    FCGIProtocolDriver::Header h =
	{
	1,
	FCGIProtocolDriver::TYPE_STDOUT,
	id >> 8, id & 0xff,
	buf.size() >> 8, buf.size() & 0xff,
	0,
	0
	};
    driver.output_cb(&h, sizeof(h));
    driver.output_cb(buf.data(), buf.size());
    }

void FCGIRequest::end_request(u_int32_t appStatus, u_int8_t protStatus)
    {
    FCGIProtocolDriver::EndRequestMsg msg =
	{
	    {
	    1,
	    FCGIProtocolDriver::TYPE_END_REQUEST,
	    id >> 8, id & 0xff,
	    sizeof(msg)-sizeof(FCGIProtocolDriver::Header) >> 8,
	    sizeof(msg)-sizeof(FCGIProtocolDriver::Header) & 0xff,
	    0,
	    0
	    },
	(appStatus >> 24) & 0xff,
	(appStatus >> 16) & 0xff,
	(appStatus >>  8) & 0xff,
	(appStatus >>  0) & 0xff,
	protStatus,
	{ 0, 0, 0 }
	};
    driver.output_cb(&msg, sizeof(msg));
    }
