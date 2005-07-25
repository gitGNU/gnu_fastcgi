# Build the fastcgi library.

CXX		= g++
AR		= ar
RANLIB		= ranlib

CPPFLAGS	=
CXXFLAGS	= -O3 -Wall -pipe
LDFLAGS		= -s

OBJS		= process-admin-messages.o process-messages.o \
		  process-stream-messages.o protocol-driver.o request.o

.cc.o:
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

# Dependencies

process-admin-messages.o: internal.hh fastcgi.hh
process-messages.o: internal.hh fastcgi.hh
process-stream-messages.o: internal.hh fastcgi.hh
protocol-driver.o: internal.hh fastcgi.hh
request.o: internal.hh fastcgi.hh
test.o: fastcgi.hh
