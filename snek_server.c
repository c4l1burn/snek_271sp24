
// server.c - prints first message from client then exits
// entirely stolen

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
#define SIZE   1024

int main(int argc, char *argv[])
{
	int sock = socket(DOMAIN, SOCK_STREAM, 0), opt = 1, conx ;
	socklen_t s = sizeof(struct sockaddr_in6) ;
	struct sockaddr_in6 addr ;
	char buff[SIZE] ;

	addr.sin6_family = DOMAIN ;
	addr.sin6_port   = PORT   ;
	addr.sin6_addr   = in6addr_any ;
	memset(buff, 0, SIZE) ;

	// make testing easier
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	{
		perror("Server - setsockopt addr failed.\n") ;
		exit(-1) ;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("Server - setsockopt port failed.\n") ;
		exit(-1) ;
	}
	// end testing ease

	if (bind(sock, (struct sockaddr *)&addr, s))
	{
		perror("Server - bindin failed.\n") ;
		exit(-1) ;
	}
	if (listen(sock, 1))
	{
		perror("Server - listen failed.\n") ;
		exit(-1) ;
	}
	conx = accept(sock, (struct sockaddr *)&addr, &s) ;
	if (conx == -1)
	{
		perror("Server - accept failed.\n") ;
		exit(-1) ;
	}

	read(conx, buff, SIZE) ;

	printf("%s\n", buff) ;

	return 0 ;
}
