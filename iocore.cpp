/*
 * Copyright (c) 2005 by Peter Simons <simons@cryp.to>.
 * All rights reserved.
 */

#include <string>
#include <iostream>
#include <deque>
#include <functional>
using namespace std;

#include <boost/array.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
using namespace boost;

#include <sys/poll.h>

class IOCore : noncopyable
{
  enum { max_threads = 512 };
  typedef shared_ptr<void> thread_ptr ;
  array<pollfd, max_threads> poll_array ;
  int fd;

public:
  IOCore() { }
  ~IOCore() { }
};


struct SP
{
  /// Process an input block. Will be called with \c begin and \c end
  /// being equal to signal the end of the input stream.
  ///
  /// \param begin start of buffer
  /// \param end   end of buffer
  /// \return Number of bytes consumed from the buffer; may be 0 to
  ///         signal blocking (not enough data to do anything).

  size_t operator() (const char* begin, const char* end)
  {
    BOOST_ASSERT(begin <= end);
    size_t len = end - begin;
    if (len > 0)
      cout.write(begin, len);
    return len;
  }
};

template <typename T>
void test(T)
{
  cerr << __PRETTY_FUNCTION__ << endl;
}

template <typename spT>
void drive(istream& hIn, spT sp = spT())
{
  char buf[20];
  size_t len = 0;

  while(hIn.good())
  {
    size_t n;
    hIn.read(&buf[len], sizeof(buf) - len);
    n    = hIn.gcount();
    len += n;
    BOOST_ASSERT(len <= sizeof(buf));
    n    = sp(&buf[0], &buf[len]);
    BOOST_ASSERT(n <= len);
    if (n > 0)
      copy(&buf[n], &buf[len], &buf[0]);
    len -= n;
  }
  sp(buf, buf);
}

int main(int, char**)
{
  SP sp;

  test(&SP::operator());

  drive(cin, sp);

  return 0;
}
