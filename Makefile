all: sunsaver

sunsaver: connection send collect
	cc -std=c99 -I/usr/local/include -L/usr/local/lib -lmodbus -lcurl -lhiredis -ljson-c -lpthread src/main.c build/send.o build/collect.o build/connection.o -o sunsaver

send: connection
	cc -c src/send.c -o build/send.o

collect: connection
	cc -c src/collect.c -o build/collect.o

connection:
	cc -c src/connection.c -o build/connection.o

clean:
	rm -rf build/*o sunsaver
