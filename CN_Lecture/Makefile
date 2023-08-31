CC = gcc
CFLAGS = -Wall -O
CLIBS =
#
OS := $(shell uname -s)
ifeq ($(OS), Linux)
#	CLIBS += -lbsd
endif
ifeq ($(OS), Darwin)
#	CLIBS +=
endif
#
all: server.o
	$(CC) -o server server.o $(CLIBS)

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -rf *.o
	rm -rf tsh
