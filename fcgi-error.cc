/*
 * Copyright (c) 2001 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "fastcgi.hh"

// Even pure virtual member functions must exist for the linker.

fcgi_error::~fcgi_error() throw()
    {
    }
