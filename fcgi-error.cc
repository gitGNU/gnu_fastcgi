/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#include "fastcgi.hpp"

// Even pure virtual member functions must exist for the linker.

fcgi_error::~fcgi_error() throw()
    {
    }
