.PHONY: clean

all: x_hash3.o libxhash3.so x_hash3 

clean:
	rm -f *.so *.o x_hash2 x_hash3 out.txt

x_hash: x_hash.o

x_hash2: LDFLAGS=-lhiredis
x_hash2: x_hash2.o

x_hash3: LDFLAGS=-L. -lxhash3  -ldhash
x_hash3: run_words.o 

x_hash3.o:CPPFLAGS=-g -fPIC 
x_hash3.o:x_hash3.c


libxhash3.so: LDFLAGS=-shared -ldhash 
libxhash3.so: OBJECTS=x_hash3.o

libxhash3.so : $(OBJECTS)
	    $(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

test: x_hash3
	time LD_LIBRARY_PATH=. ./x_hash3 in.txt  > /dev/null
