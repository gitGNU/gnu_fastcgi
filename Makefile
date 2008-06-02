# Build the fastcgi example programs.

CXX		= g++
CPPFLAGS	= -DDEBUG_FASTCGI
CXXFLAGS	= -O3 -Wall
LDFLAGS		=

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

all:		test.fcgi echo.fcgi

test.fcgi:	test.o fastcgi.o
	$(CXX) $(LDFLAGS) -o $@ test.o fastcgi.o

echo.fcgi:	echo.o fastcgi.o
	$(CXX) $(LDFLAGS) -o $@ echo.o fastcgi.o

clean::
	@rm -f fastcgi.o test.fcgi test.o echo.fcgi echo.o
	@rm -f *.bak

# Dependencies

fastcgi.o:	fastcgi.hpp
test.o:		fastcgi.hpp
echo.o:		fastcgi.hpp
