CC=gcc
CFLAGS=-g -fPIC
LDFLAGS=-shared -o

WIN_BIN=lib_chron.dll
UNIX_BIN=libchron.so

OBJFILES=$(wildcard src/*.c deps/*.c)

all: unix

unix:
	$(CC) -I ./include -I ./deps $(CFLAGS) $(OBJFILES) $(LDFLAGS) $(UNIX_BIN) -lrt -lpthread

win:
	$(CC) $(CFLAGS) $(OBJFILES) $(LDFLAGS) $(WIN_BIN)

clean:
	rm $(TARGET)
