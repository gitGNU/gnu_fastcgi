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
