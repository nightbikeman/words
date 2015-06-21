.PHONY: clean

all: x_hash3.o x_hash2.o libxhash3.so libxhash2.so x_hash3 x_hash2

clean:
	rm -f *.so *.o x_hash2 x_hash3 out.txt

x_hash: x_hash.o

x_hash2: LDFLAGS=-L. -lhiredis -lxhash2 
x_hash2: run_words.o

x_hash3: LDFLAGS=-L. -lxhash3  -ldhash -fopenmp
x_hash3: run_words.o 

x_hash3.o:CPPFLAGS=-g -fPIC -fopenmp
x_hash3.o:x_hash3.c

run_words.o:CPPFLAGS=-g -fopenmp

libxhash3.so: LDFLAGS=-shared  
libxhash3.so: OBJECTS=x_hash3.o

libxhash3.so libxhash2.so: $(OBJECTS)
	    $(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

libxhash3.so : $(OBJECTS)
	    $(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) 

test: x_hash3
	@echo HASH version 2
	time LD_LIBRARY_PATH=. ./x_hash2 in.txt  > /dev/null
	@echo HASH version 3
	time LD_LIBRARY_PATH=. ./x_hash3 in.txt  > /dev/null
