
# https://www3.ntu.edu.sg/home/ehchua/programming/cpp/gcc_make.html
# g++ -o blokker blokker.cpp -I/usr/local/include -L/usr/local/lib -lwiringPi

# $@ for the pattern-matched target
# $< for the pattern-matched dependency
# $^ expands to the rule's dependencies, in this case the three files

CC=g++
CFLAGS=-c -Wall

all: rf-trans

# link
rf-trans: rf-trans.o
	$(CC) rf-trans.o OokTransmitter.o -lboost_program_options -lwiringPi -o rf-trans

# build rf-trans object
rf-trans.o: OokTransmitter.o rf-trans.cpp
	$(CC) $(CFLAGS) rf-trans.cpp

# build OokTransmitter object
OokTransmitter.o: OokTransmitter.cpp OokTransmitter.h
	$(CC) $(CFLAGS) OokTransmitter.cpp

clean:
	rm *.o
