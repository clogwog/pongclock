CXXFLAGS=-Wall -O3 -g -fno-strict-aliasing
BINARIES=pongclock 

# Where our library resides. It is split between includes and the binary
# library in lib
RGB_INCDIR=include
RGB_LIBDIR=lib
LDFLAGS+=-L$(RGB_LIBDIR) -lrgbmatrix -lrt -lm -lpthread

all : $(BINARIES)


pongclock : pongclock.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) pongclock.o -o $@ $(LDFLAGS)


%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -DADAFRUIT_RGBMATRIX_HAT -c -o $@ $<

clean:
	rm -f *.o $(OBJECTS) $(BINARIES)

