all: server.c sender.c receiver.c
	gcc -Wall -pedantic -o server server.c 
	gcc -Wall -pedantic -o sender sender.c
	gcc -Wall -pedantic -o receiver receiver.c
