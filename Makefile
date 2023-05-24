server:
	g++ server.cpp lock.cpp threadpool.cpp http_conn.cpp -o server -lpthread
clean:
	rm server
