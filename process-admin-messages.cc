/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "internal.hh"

void FCGIProtocolDriver::process_unknown(u_int8_t type)
     {
     new(tmp_buf) UnknownTypeMsg(type);
     output_cb(tmp_buf, sizeof(UnknownTypeMsg));
     }
