CC = g++
CFLAGS = -g -std=c++17 -Wall -lboost_system -lboost_thread -lpthread -pipe -DBOOST_ALLOW_DEPRECATED_HEADERS

all: webserver

webserver: my_server.cpp
	$(CC) $(CFLAGS) -o webserver my_server.cpp

clean:
	rm -f webserver
