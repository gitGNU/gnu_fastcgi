# Build the fastcgi example program.

CXX		= g++
INSTALL		= install
APACHECTL	= /usr/local/apache/bin/apachectl

CPPFLAGS	=
CXXFLAGS	= -pipe -Wall -pedantic -O3
LDFLAGS		= -s

OBJS		= echo.o				\
		  libfastcgi/fcgi-error.o		\
		  libfastcgi/process-admin-messages.o	\
		  libfastcgi/process-messages.o 	\
		  libfastcgi/process-stream-messages.o  \
		  libfastcgi/protocol-driver.o 		\
		  libfastcgi/request.o

.PHONY:		all echo clean install
.SUFFIXES:
.SUFFIXES:	.o .cc

.cc.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

all:		echo

echo:		$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

install:	echo
	$(INSTALL) -c echo /usr/local/apache/fcgi-bin/
	su -c "$(APACHECTL) graceful"

clean:
	@rm -f echo $(OBJS)

# Dependencies

echo.o: infrastructure.hh libscheduler/scheduler.hh
echo.o: libscheduler/pollvector.hh libfastcgi/fastcgi.hh
libfastcgi/fcgi-error.o: libfastcgi/fastcgi.hh
libfastcgi/process-admin-messages.o: libfastcgi/internal.hh
libfastcgi/process-admin-messages.o: libfastcgi/fastcgi.hh
libfastcgi/process-messages.o: libfastcgi/internal.hh libfastcgi/fastcgi.hh
libfastcgi/process-stream-messages.o: libfastcgi/internal.hh
libfastcgi/process-stream-messages.o: libfastcgi/fastcgi.hh
libfastcgi/protocol-driver.o: libfastcgi/internal.hh libfastcgi/fastcgi.hh
libfastcgi/request.o: libfastcgi/internal.hh libfastcgi/fastcgi.hh
