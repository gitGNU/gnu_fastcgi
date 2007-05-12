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

#include "scheduler.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

class my_handler : public scheduler::event_handler
{
public:
  my_handler(scheduler& sched) : mysched(sched)
  {
  }
  ~my_handler()
  {
  }
  scheduler::handler_properties prop;
private:
  virtual void fd_is_readable(int fd)
  {
    int rc = read(fd, &tmp, sizeof(tmp));
    if (rc == 0)
      mysched.remove_handler(0);
    else if (rc > 0)
    {
      buffer.append(tmp, rc);
      prop.poll_events = POLLOUT;
      mysched.register_handler(1, *this, prop);
    }
  }
  virtual void fd_is_writable(int fd)
  {
    if (buffer.empty())
      mysched.remove_handler(1);
    int rc = write(fd, buffer.data(), buffer.length());
    if (rc > 0)
      buffer.erase(0, rc);
  }
  virtual void read_timeout(int fd)
  {
    cerr << "fd " << fd << " had a read timeout." << endl;
  }
  virtual void write_timeout(int fd)
  {
    cerr << "fd " << fd << " had a write timeout." << endl;
  }
  virtual void error_condition(int fd)
  {
    cerr << "fd " << fd << " had an error condition." << endl;
  }
  virtual void pollhup(int fd)
  {
    cerr << "fd " << fd << " has hung up." << endl;
  }
  char tmp[1024];
  string buffer;
  scheduler& mysched;
};

int main(int, char**)
{
  scheduler sched;
  my_handler my_handler(sched);
  my_handler.prop.poll_events   = POLLIN;
  my_handler.prop.read_timeout  = 5;
  my_handler.prop.write_timeout = 3;
  sched.register_handler(0, my_handler, my_handler.prop);
  while (!sched.empty())
  {
    sched.schedule();
    //sched.dump(cerr);
  }

  // done
  return 0;
}
