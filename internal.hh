/*
 * $Source$
 * $Revision$
 * $Date$
 *
 * Copyright (c) 2000 by Peter Simons <simons@ieee.org>.
 * All rights reserved.
 */

#ifndef __INTERNAL_HPP__
#define __INTERNAL_HPP__

#include "fastcgi.hh"

namespace
    {
    enum message_type_t
	{
	TYPE_BEGIN_REQUEST     =  1,
	TYPE_ABORT_REQUEST     =  2,
	TYPE_END_REQUEST       =  3,
	TYPE_PARAMS            =  4,
	TYPE_STDIN             =  5,
	TYPE_STDOUT            =  6,
	TYPE_STDERR            =  7,
	TYPE_DATA              =  8,
	TYPE_GET_VALUES        =  9,
	TYPE_GET_VALUES_RESULT = 10,
	TYPE_UNKNOWN           = 11
	};

    struct Header
	{
	u_int8_t version;
	u_int8_t type;
	u_int8_t requestIdB1;
	u_int8_t requestIdB0;
	u_int8_t contentLengthB1;
	u_int8_t contentLengthB0;
	u_int8_t paddingLength;
	u_int8_t reserved;

	Header()
	    {
	    memset(this, 0, sizeof(*this));
	    }
	Header(message_type_t t, u_int16_t id, u_int16_t len)
		: version(1), type(t),
	          requestIdB1(id >> 8), requestIdB0(id & 0xff),
	          contentLengthB1(len >> 8), contentLengthB0(len & 0xff),
		  paddingLength(0), reserved(0)
	    {
	    }
	};

    struct BeginRequest
	{
	u_int8_t roleB1;
	u_int8_t roleB0;
	u_int8_t flags;
	u_int8_t reserved[5];
	};
    static const u_int8_t  FLAG_KEEP_CONN  = 1;

    struct EndRequestMsg : public Header
	{
	u_int8_t appStatusB3;
	u_int8_t appStatusB2;
	u_int8_t appStatusB1;
	u_int8_t appStatusB0;
	u_int8_t protocolStatus;
	u_int8_t reserved[3];

	EndRequestMsg()
	    {
	    memset(this, 0, sizeof(*this));
	    }
	EndRequestMsg(u_int16_t id, u_int32_t appStatus, FCGIRequest::protocol_status_t protStatus)
		: Header(TYPE_END_REQUEST, id, sizeof(EndRequestMsg)-sizeof(Header)),
		  appStatusB3((appStatus >> 24) & 0xff),
		  appStatusB2((appStatus >> 16) & 0xff),
		  appStatusB1((appStatus >>  8) & 0xff),
		  appStatusB0((appStatus >>  0) & 0xff),
		  protocolStatus(protStatus)
	    {
	    memset(this->reserved, 0, sizeof(this->reserved));
	    }
        };

    struct UnknownTypeMsg : public Header
	{
	u_int8_t type;
	u_int8_t reserved[7];

	UnknownTypeMsg()
	    {
	    memset(this, 0, sizeof(*this));
	    }
	UnknownTypeMsg(u_int8_t unknown_type)
		: Header(TYPE_UNKNOWN, 0, sizeof(UnknownTypeMsg)-sizeof(Header)),
		  type(unknown_type)
	    {
	    memset(this->reserved, 0, sizeof(this->reserved));
	    }
        };

    } // end of anonymous namespace

#endif // !defined(__INTERNAL_HPP__)
