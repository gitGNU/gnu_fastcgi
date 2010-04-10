# Build the fastcgi library.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AM_CPPFLAGS	= -DDEBUG_FASTCGI
AM_CXXFLAGS	= -O3 -Wall

EXTRA_DIST = fastcgi.hpp libscheduler

bin_PROGRAMS = test.fcgi echo.fcgi

test_fcgi_SOURCES = test.cpp fastcgi.cpp

echo_fcgi_SOURCES = echo.cpp fastcgi.cpp

