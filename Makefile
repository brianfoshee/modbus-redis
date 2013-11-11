all: sunsaver

sunsaver: send collect main
	cc build/send.o build/collect.o build/main.o -o sunsaver

main: send collect
	cc -std=c99 -I/usr/local/include -L/usr/local/lib -lmodbus -lcurl -lhiredis -ljson-c -lpthread src/main.c -o main.o

send: connection
	cc -c -std=c99 src/send.c -o build/send.o

collect: connection
	cc -c -std=c99 src/collect.c -o build/collect.o

connection:
	cc -c -std=c99 src/connection.h -o build/connection.o

clean:
	rm -rf build/*o sunsaver
