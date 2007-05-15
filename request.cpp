/*
 * Copyright (c) 2001-2007 Peter Simons <simons@cryp.to>
 *
 * This software is provided 'as-is', without any express or
 * implied warranty. In no event will the authors be held liable
 * for any damages arising from the use of this software.
 *
 * Copying and distribution of this file, with or without
 * modification, are permitted in any medium without royalty
 * provided the copyright notice and this notice are preserved.
 */

#include "internal.hpp"

FCGIRequest::FCGIRequest(FCGIProtocolDriver& driver_, u_int16_t id_, role_t role_, bool kc)
  : id(id_), role(role_), keep_connection(kc), aborted(false), stdin_eof(false)
  , data_eof(false), handler_cb(0), driver(driver_)
{
}

FCGIRequest::~FCGIRequest()
{
  if (handler_cb)
    delete handler_cb;
}

void FCGIRequest::write(const std::string& buf, ostream_type_t stream)
{
  write(buf.data(), buf.size(), stream);
}

void FCGIRequest::write(const char* buf, size_t count, ostream_type_t stream)
{
  if (count > 0xffff)
    throw std::out_of_range("Can't send messages of that size.");
  else if (count == 0)
    return;

  // Construct message.

  if (stream == STDOUT)
    new(tmp_buf) Header(TYPE_STDOUT, id, count);
  else
    new(tmp_buf) Header(TYPE_STDERR, id, count);
  driver.output_cb(tmp_buf, sizeof(Header));
  driver.output_cb(buf, count);
}

void FCGIRequest::end_request(u_int32_t appStatus, FCGIRequest::protocol_status_t protStatus)
{
  // Terminate the stdout and stderr stream, and send the
  // end-request message.

  u_int8_t* p = tmp_buf;
  new(p) Header(TYPE_STDOUT, id, 0);
  p += sizeof(Header);
  new(p) Header(TYPE_STDERR, id, 0);
  p += sizeof(Header);
  new(p) EndRequestMsg(id, appStatus, protStatus);
  p += sizeof(EndRequestMsg);
  driver.output_cb(tmp_buf, p-tmp_buf);
  driver.terminate_request(id);
}
