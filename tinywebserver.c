#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "acess.h"
#include "acces-network.h"
#define PORT 80 // The port users will be connecting to
#define WEBROOT "./webroot" // The webserver's root directory
void handle_connection(int, struct sockaddr_in *); 
int get_file_size(int);
int main(void) {
int sockfd, new_sockfd, yes=1;
struct sockaddr_in host_addr, client_addr; 
socklen_t sin_size;
printf("Accepting web requests on port %d\n", PORT);
if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
fatal("in socket");
if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
fatal("setting socket option SO_REUSEADDR");
host_addr.sin_family = AF_INET; // Host byte order
host_addr.sin_port = htons(PORT); // Short, network byte order
host_addr.sin_addr.s_addr = INADDR_ANY; // Automatically fill with my IP.
memset(&(host_addr.sin_zero), '\0', 8); // Zero the rest of the struct.
if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
fatal("binding to socket");
if (listen(sockfd, 20) == -1)
fatal("listening on socket");
while(1) { // Accept loop.
sin_size = sizeof(struct sockaddr_in);
new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
if(new_sockfd == -1)
fatal("accepting connection");
handle_connection(new_sockfd, &client_addr);
}
return 0;
}

void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr) {
unsigned char *ptr, request[500], resource[500];
int fd, length;
length = recv_line(sockfd, request);
printf("Got request from %s:%d \"%s\"\n", inet_ntoa(client_addr_ptr->sin_addr),
ntohs(client_addr_ptr->sin_port), request);
ptr = strstr(request, " HTTP/"); 
if(ptr == NULL) { 
printf(" NOT HTTP!\n");
} else {
*ptr = 0; 
ptr = NULL;
if(strncmp(request, "GET ", 4) == 0) 
ptr = request+4; 
if(strncmp(request, "HEAD ", 5) == 0)
ptr = request+5; 
if(ptr == NULL) { 
printf("\tUNKNOWN REQUEST!\n");
} else { 
if (ptr[strlen(ptr) - 1] == '/') 
strcat(ptr, "index.html"); 
strcpy(resource, WEBROOT); 
strcat(resource, ptr);
fd = open(resource, O_RDONLY, 0); 
printf("\tOpening \'%s\'\t", resource);
if(fd == -1) {
printf(" 404 Not Found\n");
send_string(sockfd, "HTTP/1.0 404 NOT FOUND\r\n");
send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
send_string(sockfd, "<html><head><title>404 Not Found</title></head>");
send_string(sockfd, "<body><h1>URL not found</h1></body></html>\r\n");
} else {
printf(" 200 OK\n");
send_string(sockfd, "HTTP/1.0 200 OK\r\n");
send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
if(ptr == request + 4) { 
if( (length = get_file_size(fd)) == -1)
fatal("getting resource file size");
if( (ptr = (unsigned char *) malloc(length)) == NULL)
fatal("allocating memory for reading resource");
read(fd, ptr, length); 
send(sockfd, ptr, length, 0); 
free(ptr);
}
close(fd);
} 
} 
} 
shutdown(sockfd, SHUT_RDWR); 
}

int get_file_size(int fd) {
struct stat stat_struct;
if(fstat(fd, &stat_struct) == -1)
return -1;
return (int) stat_struct.st_size;
}