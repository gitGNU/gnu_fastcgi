# Build the fastcgi library.

CXX		= g++
AR		= ar
RANLIB		= ranlib

CPPFLAGS	= -I../libscheduler -DDEBUG_FASTCGI -I$${BOOST_ROOT}
CXXFLAGS	= -O3 -Wall -pipe
LDFLAGS		=

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

asio-test:	asio-test.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ asio-test.o libfastcgi.a

echo:		echo.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ echo.o libfastcgi.a

clean::
	@rm -f libfastcgi.a $(OBJS)
	@rm -f test test.o echo echo.o
	@rm -f *.bak

index.html:	README
	@lhs2html $<
	@sed -e 's%<p>\(.*Homepage\]</a></p>\)%<p class="title">\1%' <$<.html >$@
	@rm -f $<.html

# Dependencies

process-admin-messages.o:	internal.hh fastcgi.hh
process-messages.o:		internal.hh fastcgi.hh
process-stream-messages.o:	internal.hh fastcgi.hh
protocol-driver.o:		internal.hh fastcgi.hh
request.o:			internal.hh fastcgi.hh
test.o:				fastcgi.hh
echo.o:				infrastructure.hh
