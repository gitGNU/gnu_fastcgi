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

#ifndef FASTCGI_INTERNAL_HPP_INCLUDED
#define FASTCGI_INTERNAL_HPP_INCLUDED

#include "fastcgi.hpp"
#include <iostream>

enum message_type_t
  { TYPE_BEGIN_REQUEST     =  1
  , TYPE_ABORT_REQUEST     =  2
  , TYPE_END_REQUEST       =  3
  , TYPE_PARAMS            =  4
  , TYPE_STDIN             =  5
  , TYPE_STDOUT            =  6
  , TYPE_STDERR            =  7
  , TYPE_DATA              =  8
  , TYPE_GET_VALUES        =  9
  , TYPE_GET_VALUES_RESULT = 10
  , TYPE_UNKNOWN           = 11
  };

struct Header
{
  uint8_t version;
  uint8_t type;
  uint8_t requestIdB1;
  uint8_t requestIdB0;
  uint8_t contentLengthB1;
  uint8_t contentLengthB0;
  uint8_t paddingLength;
  uint8_t reserved;

  Header()
  {
    memset(this, 0, sizeof(*this));
  }

  Header(message_type_t t, uint16_t id, uint16_t len)
    : version(1), type(t)
    , requestIdB1(id >> 8)
    , requestIdB0(id & 0xff)
    , contentLengthB1(len >> 8)
    , contentLengthB0(len & 0xff)
    , paddingLength(0), reserved(0)
  {
  }
};

struct BeginRequest
{
  uint8_t roleB1;
  uint8_t roleB0;
  uint8_t flags;
  uint8_t reserved[5];
};

static const uint8_t FLAG_KEEP_CONN  = 1;

struct EndRequestMsg : public Header
{
  uint8_t appStatusB3;
  uint8_t appStatusB2;
  uint8_t appStatusB1;
  uint8_t appStatusB0;
  uint8_t protocolStatus;
  uint8_t reserved[3];

  EndRequestMsg()
  {
    memset(this, 0, sizeof(*this));
  }

  EndRequestMsg(uint16_t id, uint32_t appStatus, FCGIRequest::protocol_status_t protStatus)
    : Header(TYPE_END_REQUEST, id, sizeof(EndRequestMsg)-sizeof(Header))
    , appStatusB3((appStatus >> 24) & 0xff)
    , appStatusB2((appStatus >> 16) & 0xff)
    , appStatusB1((appStatus >>  8) & 0xff)
    , appStatusB0((appStatus >>  0) & 0xff)
    , protocolStatus(protStatus)
  {
    memset(this->reserved, 0, sizeof(this->reserved));
  }
};

struct UnknownTypeMsg : public Header
{
  uint8_t type;
  uint8_t reserved[7];

  UnknownTypeMsg()
  {
    memset(this, 0, sizeof(*this));
  }

  UnknownTypeMsg(uint8_t unknown_type)
    : Header(TYPE_UNKNOWN, 0, sizeof(UnknownTypeMsg) - sizeof(Header))
    , type(unknown_type)
  {
    memset(this->reserved, 0, sizeof(this->reserved));
  }
};

#endif // FASTCGI_INTERNAL_HPP_INCLUDED
