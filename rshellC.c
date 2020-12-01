#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char *argv[]){
  struct addrinfo hints, *addr;
  int bytes, sockfd, status;
  int rflag=0;
  char receive[100];
  char userin[2047];

  if(argc!=3) {
	printf("Usage: ./rshellC [IP] [Port]\n");
	return -1;
  }
  memset(&hints,0,sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if((status=getaddrinfo(argv[1], argv[2], &hints, &addr)) != 0){
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return -1;
  }

  if((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1){
    perror("Socket Error");
    return -1;
  }

  if(connect(sockfd, addr->ai_addr, addr->ai_addrlen) == -1){
    perror("Connect Error");
    close(sockfd);
    return -1;
  }
  
  if((bytes = recv(sockfd, receive, 99, 0)) == -1){
	close(sockfd);
	perror("Recv Error");
	return -1;
  }

  receive[bytes]='\0';
  
  printf("---------------  rshellS  ---------------\n");
  printf("Connect Successful\nrshellS Running as: %sType \"exit\" to exit or \"help\" for a local command listing\n", receive);
  
  if(strncmp(receive, "root", 4)==0) rflag=1;
  while (1){
	printf("--shell%s ", (rflag==1) ? "#" : "$");
	fgets(userin, 2047, stdin);
	//printf("you inputted: %s", userin);
    //SEND shell command
    if(strncmp(userin, "exit", 4)==0) break;
    
    if(strncmp(userin, "help", 4)==0){
		printf("exit: disconnects from rshellS\n");
		printf("help: prints this statement\n");
	}
	else{
		if(send(sockfd, userin, strlen(userin),0)==-1) {
				perror("send");
		}
			
	//RECIEVE output of command
		while(1){
			if((bytes = recv(sockfd, receive, 99, 0)) == -1){
				close(sockfd);
				perror("Recv Error");
				return -1;}
			receive[bytes]='\0';
			if(strncmp(receive, "000xxx000", 9)==0){
				//printf("RECIEVED FINAL TERMINATOR\n");
				break;
			}
			printf("%s", receive);
		}
	}
  }
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
  return 0;
}

