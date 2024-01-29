CC = g++
CFLAGS = -g -std=c++17 -Wall -lboost_system -lboost_thread -lpthread -pipe -DBOOST_ALLOW_DEPRECATED_HEADERS -O3
SOURCE = main.cpp http_server.cpp http_session.cpp globals.cpp


all: webserver

webserver: main.cpp http_server.cpp http_session.cpp globals.cpp http_server.hpp http_session.hpp globals.hpp
	$(CC) $(CFLAGS) -o webserver $(SOURCE)

clean:
	rm -f webserver
