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

const FCGIProtocolDriver::proc_func_t FCGIProtocolDriver::proc_funcs[] =
  { 0                                           // unused
  , &FCGIProtocolDriver::process_begin_request  // TYPE_BEGIN_REQUEST
  , &FCGIProtocolDriver::process_abort_request  // TYPE_ABORT_REQUEST
  , 0                                           // TYPE_END_REQUEST
  , &FCGIProtocolDriver::process_params         // TYPE_PARAMS
  , &FCGIProtocolDriver::process_stdin          // TYPE_STDIN
  , 0                                           // TYPE_STDOUT
  , 0                                           // TYPE_STDERR
  , 0                                           // TYPE_DATA
  , 0                                           // TYPE_GET_VALUES
  , 0                                           // TYPE_GET_VALUES_RESULT
  , 0                                           // TYPE_UNKNOWN
  };

FCGIProtocolDriver::FCGIProtocolDriver(OutputCallback& cb) : output_cb(cb)
{
}

FCGIProtocolDriver::~FCGIProtocolDriver()
{
  for(reqmap_t::iterator i = reqmap.begin(); i != reqmap.end(); ++i)
  {
    delete i->second;
  }
}

void FCGIProtocolDriver::process_input(const void* buf, size_t count)
{
  // Copy data to our own buffer.

  InputBuffer.append(static_cast<const u_int8_t*>(buf), count);

  // If there is enough data in the input buffer to contain a
  // header, interpret it.

  while(InputBuffer.size() >= sizeof(Header))
  {
    const Header* hp  = reinterpret_cast<const Header*>(InputBuffer.data());

    // Check whether our peer speaks the correct protocol version.

    if (hp->version != 1)
    {
      char buf[256];
      sprintf(buf, "FCGIProtocolDriver cannot handle protocol version %u.", hp->version);
      throw unsupported_fcgi_version(buf);
    }

    // Check whether we have the whole message that follows the
    // headers in our buffer already. If not, we can't process it
    // yet.

    u_int16_t msg_len = (hp->contentLengthB1 << 8) + hp->contentLengthB0;
    u_int16_t msg_id  = (hp->requestIdB1 << 8) + hp->requestIdB0;

    if (InputBuffer.size() < sizeof(Header)+msg_len+hp->paddingLength)
      return;

    // Process the message. In case an exception arrives here,
    // terminate the request.

    try
    {
#ifdef DEBUG_FASTCGI
      cerr << "Received message: id = " << msg_id << ", "
           << "body len = " << msg_len << ", "
           << "type = " << (int)hp->type << endl;
#endif
      if (hp->type > TYPE_UNKNOWN || proc_funcs[hp->type] == 0)
        process_unknown(hp->type);
      else
        (this->*proc_funcs[hp->type])(msg_id, InputBuffer.data()+sizeof(Header), msg_len);
    }
    catch(const fcgi_io_callback_error&)
    {
      throw;
    }
    catch(const std::exception& e)
    {
      std::cerr << "Caught exception while processing request #" << msg_id << ": " << e.what() << std::endl;
      terminate_request(msg_id);
    }
    catch(...)
    {
      std::cerr << "Caught unknown exception while processing request #" << msg_id << "." << std::endl;
      terminate_request(msg_id);
    }

    // Remove the message from our buffer and contine processing
    // if there if something left.

    InputBuffer.erase(0, sizeof(Header)+msg_len+hp->paddingLength);
  }
}

FCGIRequest* FCGIProtocolDriver::get_request()
{
  if (new_request_queue.empty())
    return 0;

  FCGIRequest* r = reqmap[new_request_queue.front()];
  new_request_queue.pop();
  return r;
}

bool FCGIProtocolDriver::have_active_requests()
{
  if (new_request_queue.empty() && reqmap.empty())
    return false;
  else
    return true;
}

void FCGIProtocolDriver::terminate_request(u_int16_t id)
{
  reqmap_t::iterator req;
  req = reqmap.find(id);
  if (req != reqmap.end())
  {
    delete req->second;
    reqmap.erase(req);
  }
}

// Pure virtual destructors must also exist somewhere.

FCGIProtocolDriver::OutputCallback::~OutputCallback()
{
}
