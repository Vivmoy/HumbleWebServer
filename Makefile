server:
	g++ -W -Wall -o server server.cpp lock.cpp threadpool.h http_conn.cpp -lpthread
clean:
	rm server
