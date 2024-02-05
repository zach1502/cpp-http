CC = g++
CFLAGS = -g -std=c++17 -Wall -lboost_system -lboost_thread -lpthread -pipe -DBOOST_ALLOW_DEPRECATED_HEADERS -O3
SOURCE = main.cpp http_server.cpp http_session.cpp globals.cpp router.cpp logger.cpp
HEADERS =         http_server.hpp http_session.hpp globals.hpp router.hpp thread_safe_queue.hpp logger.hpp


all: webserver

webserver: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -o webserver $(SOURCE)

profile: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -pg -o webserver $(SOURCE)

clean:
	rm -f webserver
