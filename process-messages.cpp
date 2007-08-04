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

  u_int8_t const * const  bufend(buf + len);
  u_int32_t               name_len;
  u_int32_t               data_len;
  while(buf != bufend)
  {
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
    assert(buf + name_len + data_len <= bufend);
    std::string const name(reinterpret_cast<const char*>(buf), name_len);
    buf += name_len;
    std::string const data(reinterpret_cast<const char*>(buf), data_len);
    buf += data_len;
#ifdef DEBUG_FASTCGI
    std::cerr << "request #" << id << ": FCGIProtocolDriver received PARAM '" << name << "' = '" << data << "'"
              << std::endl;
#endif
    req->second->params[name] = data;
  }
}
