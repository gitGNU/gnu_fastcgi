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
	: id(id_), role(role_), keep_connection(kc), have_all_params(false),
          aborted(false), driver(driver_)
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
    write(buf.data(), buf.size());
    }

void FCGIRequest::write(const void* buf, size_t count)
    {
    if (count > 0xffff)
	throw out_of_range("Can't send messages of that size.");
    else if (count == 0)
	return;

    // Construct message.

    FCGIProtocolDriver::Header h =
	{
	1,
	FCGIProtocolDriver::TYPE_STDOUT,
	id >> 8, id & 0xff,
	count >> 8, count & 0xff,
	0,
	0
	};
    driver.output_cb(&h, sizeof(h));
    driver.output_cb(buf, count);
    }

void FCGIRequest::end_request(u_int32_t appStatus, u_int8_t protStatus)
    {
    // Terminate the stdout stream.

    FCGIProtocolDriver::Header h1 =
	{
	1,
	FCGIProtocolDriver::TYPE_STDOUT,
	id >> 8, id & 0xff,
	0, 0,
	0,
	0
	};
    driver.output_cb(&h1, sizeof(h1));

    // Terminate the stderr stream.

    FCGIProtocolDriver::Header h2 =
	{
	1,
	FCGIProtocolDriver::TYPE_STDERR,
	id >> 8, id & 0xff,
	0, 0,
	0,
	0
	};
    driver.output_cb(&h2, sizeof(h2));

    // Send the end-request message.

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
    driver.terminate_request(id);
    }
