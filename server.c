


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

#define INPORT "31000"  /* the port users will be connecting to */
#define OUTPORT "32000"

#define BACKLOG 10     /* how many pending connections queue will hold */

void sigchld_handler(int s)
{
	    /* waitpid() might overwrite errno, so we save and restore it: */
	    int saved_errno = errno;
	    
	    while(waitpid(-1, NULL, WNOHANG) > 0);
	    	errno = saved_errno;
	    }
	    
	    
	    /* get sockaddr, IPv4 or IPv6: */
	    void *get_in_addr(struct sockaddr *sa)
	    {
	    	if (sa->sa_family == AF_INET) {
	    		return &(((struct sockaddr_in*)sa)->sin_addr);
	    	}

	    	return &(((struct sockaddr_in6*)sa)->sin6_addr);
	    }
	    
	    int main(void)
	    {

		char hostname[128];
		char recvLine[128];
		size_t buffer= 128;
		size_t num_bytes=0;

	    	int sockfd, new_fd, send_fd;  /* listen on sock_fd, new connection on new_fd */
	        struct addrinfo hints, *servinfo, *p;
	        struct sockaddr_storage their_addr; /* connector's address information */
	        socklen_t sin_size;
	        struct sigaction sa;
	        int yes=1;
	        char s[INET6_ADDRSTRLEN];
	        int rv;
	        memset(&hints, 0, sizeof hints);
	        hints.ai_family = AF_UNSPEC;
	        hints.ai_socktype = SOCK_STREAM;  
		hints.ai_flags = AI_PASSIVE; /* use my IP */

		
		gethostname(hostname, sizeof hostname);
		printf("my hostname:  %s\n ", hostname);
		getaddrinfo(hostname, INPORT, &hints, &servinfo);
		printf("my port: %s\n ", INPORT);

	        if ((rv = getaddrinfo(NULL, INPORT, &hints, &servinfo)) != 0) {
	        	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	                return 1;
		}
		/* loop through all the results and bind to the first we can */
	        for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}

			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {	   
				perror("setsockopt");	    
				exit(1);	    
			}	    
	   
			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {	   
				close(sockfd);	    
				perror("server: bind");	    
				continue;	    
			}
	    	    
			break;	    
		}

		freeaddrinfo(servinfo); /* all done with this structure */

		if (p == NULL)  {
			fprintf(stderr, "server: failed to bind\n");
			exit(1);
		}

		if (listen(sockfd, BACKLOG) == -1) {
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

		while(1) { /*  main accept() loop */
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				perror("accept");
				continue;
			}
			inet_ntop(their_addr.ss_family,
						get_in_addr((struct sockaddr *)&their_addr),
						s, sizeof s);
				printf("server: got connection from %s\n", s);
			if (!fork()) { /* this is the child process */
				/* close(sockfd);  child doesn't need the listener*/
				/*	if (send(wz2, "Hello, world!", 13, 0) == -1)
					perror("send");	*/
				exit(0);
			}
			break;
		}
	    
		printf("server: waiting for connections...\n");

		if ((rv = getaddrinfo(NULL, OUTPORT, &hints, &servinfo)) != 0) {
		        	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		                return 1;
			}
			/* loop through all the results and bind to the first we can */
		        for(p = servinfo; p != NULL; p = p->ai_next) {
				if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
					perror("server: socket");
					continue;
				}

				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
					perror("setsockopt");
					exit(1);
				}

				if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
					close(sockfd);
					perror("server: bind");
					continue;
				}

				break;
			}

			freeaddrinfo(servinfo); /* all done with this structure */

			if (p == NULL)  {
				fprintf(stderr, "server: failed to bind\n");
				exit(1);
			}

			if (listen(sockfd, BACKLOG) == -1) {
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
			
			while(1) { /*  main accept() loop */
				sin_size = sizeof their_addr;
				send_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
				if (send_fd == -1) {
					perror("accept");
					continue;
				}
				inet_ntop(their_addr.ss_family,
							get_in_addr((struct sockaddr *)&their_addr),
							s, sizeof s);
					printf("server: got connection from %s\n", s);
				if (!fork()) { /* this is the child process */
					/* close(sockfd);  child doesn't need the listener*/
					/*	if (send(wz2, "Hello, world!", 13, 0) == -1)
						perror("send");	*/
					exit(0);
				}
				break;
			}

		
		

		while(1){
			while(num_bytes == 0 || num_bytes == -1 ) {
				num_bytes = recv(new_fd, recvLine, sizeof recvLine, 0);
			}

			printf("server received: %s\n", recvLine);

			send(send_fd, recvLine, sizeof recvLine, 0);

			num_bytes = 0;
		}
		return 0;	    
	    }





