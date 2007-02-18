/*
 * Copyright (c) 2001-2006 by Peter Simons <simons@cryp.to>
 *
 * Copying and distribution of this file, with or without
 * modification, are permitted in any medium without royalty
 * provided the copyright notice and this notice are preserved.
 *
 * See <http://cryp.to/libfastcgi/> for the latest version.
 */

#include "internal.hh"

void FCGIProtocolDriver::process_unknown(u_int8_t type)
{
  new(tmp_buf) UnknownTypeMsg(type);
  output_cb(tmp_buf, sizeof(UnknownTypeMsg));
}
