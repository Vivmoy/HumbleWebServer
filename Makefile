all: server

server: server.cpp
	g++ -W -Wall -o server server.cpp -lpthread

clean:
	rm server
