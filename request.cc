/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hpp"

FCGIRequest::FCGIRequest(FCGIProtocolDriver& driver_, u_int16_t id_, role_t role_, bool kc)
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

    Header h(TYPE_STDOUT, id, count);
    driver.output_cb(&h, sizeof(h));
    driver.output_cb(buf, count);
    }

void FCGIRequest::end_request(u_int32_t appStatus, FCGIRequest::protocol_status_t protStatus)
    {
    // Terminate the stdout stream.

    Header h1(TYPE_STDOUT, id, 0);
    driver.output_cb(&h1, sizeof(h1));

    // Terminate the stderr stream.

    Header h2(TYPE_STDERR, id, 0);
    driver.output_cb(&h2, sizeof(h2));

    // Send the end-request message.

    EndRequestMsg msg(id, appStatus, protStatus);
    driver.output_cb(&msg, sizeof(msg));
    driver.terminate_request(id);
    }
