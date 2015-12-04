all: server.c client.c
	gcc -Wall -pedantic -o server server.c 
	gcc -Wall -pedantic -o sender sender.c


