HEADER_PATHS = -I/usr/local/include -L/usr/local/lib
LIBS = -lmodbus -lcurl -lhiredis -ljson-c -lpthread
CFLAGS = -std=c99

all: sunsaver

sunsaver: connection send collect
	cc ${CFLAGS} ${HEADER_PATHS} ${LIBS} src/main.c build/send.o build/collect.o build/connection.o -o sunsaver

send: connection
	cc ${CFLAGS} -c src/send.c -o build/send.o

collect: connection
	cc ${CFLAGS} -c src/collect.c -o build/collect.o

connection:
	cc ${CFLAGS} -c src/connection.c -o build/connection.o

clean:
	rm -rf build/*o sunsaver
