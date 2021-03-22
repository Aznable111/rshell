#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#define DEBUG 1

int main(int argc, char *argv[]){
  struct addrinfo hints, *addr;
  int bytes, sockfd, status;
  int rflag=0;
  char *file, *ptr, sendbuf[255];
  char receive[100];
  char userin[2047],tokuserin[2047];
  FILE *fp;

  if(argc!=3) {
	printf("Usage: %s [IP] [Port]\n", argv[0]);
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
	strcpy(tokuserin, userin);
	//printf("you inputted: %s", userin);
    //CLIENT SIDE COMMAND CHECK IF ELSE
	if(strncmp(userin, "exit", 4)==0) break;
    
    //HELP
    
    else if(strncmp(userin, "help", 4)==0){
		printf("download [Absolute file path] #Command handled on server side\n");
		printf("upload [Local file path] [Destination file path\n");
		printf("exit: disconnects from rshellS\n");
		printf("help: prints this statement\n");
	}
	//DOWNLOAD
	
	else if(strncmp(strtok(tokuserin," "), "download", 8)==0){
		printf("Setting up Download with %s\n", userin);
		file = strrchr(strtok(NULL," "), '/');
		ptr=strchr(file,'\n');
		*ptr='\0';
		printf("Creating Local File %s\n",file+1);
		if (file != NULL){
			fp = fopen(file+1,"w");
		}
		else {
			printf("Issue with filename\n");
			break;
		}
		printf("File created, reaching out to download\n");
		if(send(sockfd, userin, strlen(userin),0)==-1) {
        		perror("send");
        }		
		while(1){
 			if((bytes = recv(sockfd, receive, 99, 0)) == -1){
				close(sockfd);
                perror("Recv Error");
                return -1;
			}
            receive[bytes]='\0';
            if(strncmp(receive, "000xxx000", 9)==0){
                printf("RECIEVED FINAL TERMINATOR\n");
				break;
            }
			fputs(receive,fp);
 		}
 		printf("Complete\n");
		fclose(fp);
    }
	
    else if(strncmp(tokuserin, "upload", 8)==0){
		printf("Setting up Upload with %s\n", tokuserin);
		char *filename;
		filename=strtok(NULL, " ");
		printf("Attempting to open:%s\n",filename);
		fp = fopen(filename,"r");
		if(fp != NULL){
			printf("Success! Sending file\n");
			if(send(sockfd, userin, strlen(userin),0)==-1) {
        		perror("send");
			}
			while (fgets(sendbuf, 255, fp) != NULL){
				if(send(sockfd, sendbuf, strlen(sendbuf), 0)<0){
						perror("send");
				}	
			}
			sleep(1);
			if(send(sockfd, "000xxx000", 9, 0)<0){
				perror("send");
			}
			fclose(fp); 
		}
		else{
			printf("Unable to open file errno: %d\n", errno);
		}
	}
    
	//Just for testing
	
	//IF NO LOCAL OPTION SEND AS REMOTE SHELL COMMAND
    else{
		printf("Sending Command\n");
		char * token;
		while(token!=NULL){
			token = strtok(NULL," ");
		}
		if(send(sockfd, userin, strlen(userin),0)==-1) {
			perror("send");
		}
	//RECIEVE output of command
		while(1){
			if((bytes = recv(sockfd, receive, 99, 0)) == -1){
				close(sockfd);
				perror("Recv Error");
				return -1;
			}
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

