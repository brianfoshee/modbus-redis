all: sunsaver

sunsaver: send.o collect.o main.o
	cc send.o collect.o main.o -o sunsaver

main.o: main.c
	cc main.c

send.o: send.c
	cc -std=c99 -I/usr/local/include -L/usr/local/lib -lcurl -lhiredis -ljson-c -lpthread send.c

collect.o: collect.c
	cc -I/usr/local/include/modbus -I/usr/local/include/hiredis -L/usr/local/lib -lmodbus -lhiredis  mod-red.c

clean:
	rm -rf *o sunsaver
