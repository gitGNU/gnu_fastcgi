# Build the fastcgi library.

CXX		= g++
AR		= ar
RANLIB		= ranlib

CPPFLAGS	=
CXXFLAGS	= -Wall -pipe
LDFLAGS		=

OBJS		= protocol_driver.o process_messages.o process_admin_messages.o \
		  process_stream_messages.o request.o fcgi_error.o

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

all:		libfastcgi.a

libfastcgi.a:	$(OBJS)
	@rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

test:		test.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ test.o libfastcgi.a

clean::
	rm -f libfastcgi.a $(OBJS)
	rm -f test test.o

install:	test
	install -s -c -o bin -g bin -m 755 test /usr/local/apache/fcgi-bin/test.fcgi

# Dependencies

fcgi_error.o: fastcgi.hpp
process_admin_messages.o: internal.hpp fastcgi.hpp
process_messages.o: internal.hpp fastcgi.hpp
protocol_driver.o: internal.hpp fastcgi.hpp
request.o: internal.hpp fastcgi.hpp
test.o: fastcgi.hpp
