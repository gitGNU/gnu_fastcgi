# Build the fastcgi example programs.

CXX		= g++

CPPFLAGS	=
CXXFLAGS	= -O -finline-functions -Wall -pedantic -pipe
LDFLAGS		= -s

OBJS		= echo.o libscheduler/libscheduler.a libfastcgi/libfastcgi.a

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

echo:		$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

libscheduler/libscheduler.a:
	(cd libscheduler && $(MAKE) libscheduler.a)

libfastcgi/libfastcgi.a:
	(cd libfastcgi && $(MAKE) libfastcgi.a)

clean::
	(cd libscheduler && $(MAKE) $@)
	(cd libfastcgi && $(MAKE) $@)
	rm -f echo $(OBJS)

install:	echo
	su -c "install -s -c -o bin -g bin -m 755 echo /usr/local/apache/fcgi-bin/echo.fcgi"
	su -c "/usr/local/apache/bin/apachectl graceful"

# Dependencies

echo.o: infrastructure.hpp libscheduler/scheduler.hpp
echo.o: libscheduler/pollvector.hpp libfastcgi/fastcgi.hpp
