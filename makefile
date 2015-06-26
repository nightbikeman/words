.PHONY: clean
# This is a test

all: x_hash3.o libxhash3.so x_hash3 find_connection find_word

clean:
	rm -f *.so *.o x_hash2 x_hash3 out.txt

CPPFLAGS+=$(DEBUG) -Wall

x_hash: x_hash.o

x_hash2: LDFLAGS=-lhiredis
x_hash2: x_hash2.o

find_word find_connection x_hash3: LDFLAGS=-L. -lxhash3  -ldhash -fopenmp 
x_hash3: run_words.o 

words.o x_hash3.o:CPPFLAGS=-g -fPIC -fopenmp $(DEBUG) -Wall
x_hash3.o:x_hash3.c
words.o:words.c

find_connection.o:find_connection.c
find_connection:find_connection.o

find_word.o:find_word.c
find_word:find_word.o

run_words.o:CPPFLAGS=-g -fopenmp $(DEBUG) -Wall

libxhash3.so: LDFLAGS=-shared -ldhash 
libxhash3.so: OBJECTS=x_hash3.o words.o

libxhash3.so : x_hash3.o words.o
	    $(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) 

TEST_FILE=test_in.txt
test: all 
	LD_LIBRARY_PATH=.  ./find_connection test_in.txt google sky 
	if LD_LIBRARY_PATH=. ./find_connection test_in.txt google sky1  ; then  false ; fi
	LD_LIBRARY_PATH=.  ./find_word test_in.txt google
	if LD_LIBRARY_PATH=. ./find_word test_in.txt google1 ; then  false ; fi
	if LD_LIBRARY_PATH=. ./find_word test_in.txt  ; then  false ; fi
	if LD_LIBRARY_PATH=. ./find_word  ; then  false ; fi
	@echo PASS

confidence: all 
	LD_LIBRARY_PATH=. time ./x_hash3 in.txt 
	@echo ====================================
	@echo
	LD_LIBRARY_PATH=. time ./x_hash3 -b 100  4
	@echo ====================================
	@echo
	LD_LIBRARY_PATH=. time ./x_hash3 -c 100 test-txt
	@echo ====================================
	@echo
	LD_LIBRARY_PATH=. time ./x_hash3 -o $(TEST_FILE)
	@echo ====================================
	@echo
	LD_LIBRARY_PATH=. time ./x_hash3 -s 2 flower tree 
	@echo ====================================
	@echo
	LD_LIBRARY_PATH=. time ./x_hash3 -t doc_file
	@echo ====================================
	@echo

