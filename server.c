#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define INPORT "31000"  /* the port users will be connecting to */
#define OUTPORT "32000"

#define BACKLOG 10     /* how many pending connections queue will hold */

int recvsockfd, sendsockfd, new_fd, send_fd; /* listen on sock_fd, new connection on new_fd */
int yes = 1;

char hostname[128];

/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void sigchld_handler(int s) {
	/* waitpid() might overwrite errno, so we save and restore it: */
	int saved_errno = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
	errno = saved_errno;
}

void *acceptSenders(){
	printf("Starting acceptSenders thread\n");
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; /* connector's address information */
	socklen_t sin_size;
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];
	int rv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; /* use my IP */

	gethostname(hostname, sizeof hostname);
	printf("my hostname:  %s\n ", hostname);
	getaddrinfo(hostname, INPORT, &hints, &servinfo);
	printf("Sender Port: %s\n ", INPORT);
	printf("Receiver Port: %s\n ", OUTPORT);
	if ((rv = getaddrinfo(NULL, INPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}
	/* loop through all the results and bind to the first we can */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sendsockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sendsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sendsockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sendsockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); /* all done with this structure */

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sendsockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	sa.sa_handler = sigchld_handler; /* reap all dead processes */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while (1) { /*  main accept() loop */
		sin_size = sizeof their_addr;
		new_fd = accept(sendsockfd, (struct sockaddr *) &their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);
		if (!fork()) { /* this is the child process */
			/* close(sockfd);  child doesn't need the listener*/
			/*	if (send(wz2, "Hello, world!", 13, 0) == -1)
			 perror("send");	*/
			return;
		}
		break;
	}
}

void *acceptReceivers(){
	int rv;
	struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
	socklen_t sin_size;
	struct sockaddr_storage their_addr; /* connector's address information */
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; /* use my IP */
	printf("Starting acceptReceivers thread\n");
	if ((rv = getaddrinfo(NULL, OUTPORT, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return;
		}
		/* loop through all the results and bind to the first we can */
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((recvsockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
					== -1) {
				perror("server: socket");
				continue;
			}

			if (setsockopt(recvsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
					== -1) {
				perror("setsockopt");
				exit(1);
			}

			if (bind(recvsockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(recvsockfd);
				perror("server: bind");
				continue;
			}

			break;
		}

		freeaddrinfo(servinfo); /* all done with this structure */

		if (p == NULL) {
			fprintf(stderr, "server: failed to bind\n");
			exit(1);
		}

		if (listen(recvsockfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}
		sa.sa_handler = sigchld_handler; /* reap all dead processes */
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}

		while (1) { /*  main accept() loop */
			sin_size = sizeof their_addr;
			send_fd = accept(recvsockfd, (struct sockaddr *) &their_addr, &sin_size);
			if (send_fd == -1) {
				perror("accept");
				continue;
			}
			inet_ntop(their_addr.ss_family,
					get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);
			if (!fork()) { /* this is the child process */
				/* close(sockfd);  child doesn't need the listener*/
				/*	if (send(wz2, "Hello, world!", 13, 0) == -1)
				 perror("send");	*/
				exit(0);
			}
			break;
		}
}

int main(void) {
	char *recvLine;
	struct sockaddr_storage addr;
	socklen_t addrlen;
	int buffer = 512;
	int port;
	char ipstr[INET6_ADDRSTRLEN];
	size_t num_bytes = 0;
	char *toSend = "";
	char portSeparator[] = ", ";
	char separator[] = ": ";
	char portString[5];

	pthread_t sendAccept;
	pthread_t receiveAccept;

	pthread_create(&sendAccept, NULL, acceptSenders, NULL);
	pthread_create(&receiveAccept, NULL, acceptReceivers, NULL);

	while (1) {
		recvLine = (char *) malloc(buffer + 1);
		while (num_bytes == 0 || num_bytes == -1) {
			num_bytes = recv(new_fd, recvLine, buffer, 0);
		}
		addrlen = sizeof(addr);
		getpeername(sendsockfd, (struct sockaddr*)&addr, &addrlen);
		if(addr.ss_family == AF_INET){
			struct sockaddr_in *s = (struct sockaddr_in *)&addr;
			port = ntohs(s->sin_port);
			inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
		} else{
			struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
			port = ntohs(s->sin6_port);
			inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
		}
		printf("Peer IP address: %s\n", ipstr);
		printf("Peer port: %d\n", port);
		atoi(port, portString, 10);
		toSend = (char *)malloc(buffer);
		strcpy(toSend, ipstr);
		strcat(toSend, portSeparator);
		strcat(toSend, portString);
		strcat(toSend, separator);
		strcat(toSend, recvLine);

		send(send_fd, toSend, buffer, 0);

		num_bytes = 0;
		free(recvLine);
	}
	return 0;
}
