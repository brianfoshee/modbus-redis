HEADER_PATHS = -I/usr/local/include -L/usr/local/lib
LIBS = -lmodbus -lhiredis
CFLAGS = -std=c99

all: sunsaver

sunsaver: connection collect
	cc ${CFLAGS} ${HEADER_PATHS} ${LIBS} src/main.c build/collect.o build/connection.o -o sunsaver

collect: connection
	cc ${CFLAGS} -c src/collect.c -o build/collect.o

connection:
	cc ${CFLAGS} -c src/connection.c -o build/connection.o

clean:
	rm -rf build/*o sunsaver
