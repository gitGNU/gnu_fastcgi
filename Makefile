# Build the fastcgi library.

CXX		= g++
AR		= ar
RANLIB		= ranlib

CPPFLAGS	= -I../libscheduler -DDEBUG_FASTCGI
CXXFLAGS	= -O2 -Wall
LDFLAGS		=

OBJS		= fastcgi.o

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

all:		libfastcgi.a

libfastcgi.a:	$(OBJS)
	@rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

test:		test.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ test.o libfastcgi.a

echo:		echo.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ echo.o libfastcgi.a

clean::
	@rm -f libfastcgi.a $(OBJS)
	@rm -f test test.o echo echo.o
	@rm -f *.bak

# Dependencies

$(OBJS):	fastcgi.hpp
test.o:		fastcgi.hpp
echo.o:		fastcgi.hpp
