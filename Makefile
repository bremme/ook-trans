
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
	$(CC) rf-trans.o RemoteTransmitter.o -lboost_program_options -lwiringPi -o rf-trans

# build rf-trans object
rf-trans.o: RemoteTransmitter.o rf-trans.cpp
	$(CC) $(CFLAGS) rf-trans.cpp

# build RemoteTransmitter object
RemoteTransmitter.o: RemoteTransmitter.cpp RemoteTransmitter.h
	$(CC) $(CFLAGS) RemoteTransmitter.cpp

clean:
	rm *.o
