# Build the fastcgi library.

CXX		= g++
AR		= ar
RANLIB		= ranlib

CPPFLAGS	= -Iioxx/include -DDEBUG_FASTCGI
CXXFLAGS	= -O3 -Wall
LDFLAGS		=

OBJS		= fastcgi.o

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

all:		test.fcgi echo.fcgi

libfastcgi.a:	$(OBJS)
	@rm -f $@
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

test.fcgi:	test.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ test.o libfastcgi.a

echo.fcgi:	echo.o libfastcgi.a
	$(CXX) $(LDFLAGS) -o $@ echo.o libfastcgi.a

clean::
	@rm -f libfastcgi.a $(OBJS)
	@rm -f test.fcgi test.o echo.fcgi echo.o
	@rm -f *.bak

# Dependencies

$(OBJS):	fastcgi.hpp
test.o:		fastcgi.hpp
echo.o:		fastcgi.hpp
