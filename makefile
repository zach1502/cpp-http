CC = g++
CFLAGS = -g -std=c++17 -Wall -lboost_system -lboost_thread -lpthread -pipe -DBOOST_ALLOW_DEPRECATED_HEADERS
SOURCE = main.cpp http_server.cpp http_session.cpp


all: webserver

webserver: main.cpp http_server.cpp http_session.cpp
	$(CC) $(CFLAGS) -o webserver $(SOURCE)

clean:
	rm -f webserver
