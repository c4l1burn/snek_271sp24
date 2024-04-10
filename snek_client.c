
// check in 1: send information from the client to the server
// client.c - sends the first command line argument to the server

#include <sys/socket.h> // socket
#include <sys/types.h>  // socket
#include <netinet/in.h> // htons
#include <arpa/inet.h>  // inet_pton
#include <stdio.h>      // fprintf
#include <stdlib.h>     // exit
#include <string.h>     // strlen
#include <unistd.h>     // write

#define DOMAIN AF_INET6
#define PORT   htons(0xC271)

int main(int argc, char *argv[]) // cmd line args
{
	int sock = socket(DOMAIN, SOCK_STREAM, 0) ;
	struct sockaddr_in6 addr ;

	addr.sin6_family = DOMAIN ;
	addr.sin6_port   = PORT   ;
	inet_pton(DOMAIN, "::1", &addr.sin6_addr) ; // localhost

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
	{
		perror("Client - conect failed.") ;
		exit(-1) ;
	}

    write(sock, argv[1], strlen(argv[1])) ;

	return 0 ;
}
