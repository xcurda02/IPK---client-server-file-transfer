CFLAGS = -std=c++11 -pedantic -Wall

PROGS = ipk-client ipk-server

all: $(PROGS)

ipk-client: ipk-client.o msg.o
	g++ $(CFLAGS) -o $@ $^

ipk-client.o: ipk-client.cpp msg.h
	g++ $(CFLAGS) -c $^

ipk-server: ipk-server.o msg.o
	g++ $(CFLAGS) -o $@ $^

ipk-server.o: ipk-server.cpp msg.h
	g++ $(CFLAGS) -c $^

msg.o: msg.cpp msg.h
	g++ $(CFLAGS) -c $^
clean:
	rm -rf *.o $(PROGS)
zip:
	zip xcurda02.zip ipk-server.cpp ipk-client.cpp msg.h msg.cpp readme dokumentace.pdf Makefile
