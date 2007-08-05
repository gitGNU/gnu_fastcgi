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

void FCGIProtocolDriver::process_unknown(uint8_t type)
{
  new(tmp_buf) UnknownTypeMsg(type);
  output_cb(tmp_buf, sizeof(UnknownTypeMsg));
}
