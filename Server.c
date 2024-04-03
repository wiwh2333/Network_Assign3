/*
	C socket server example, handles multiple clients using threads
*/

#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<pthread.h> //for threading , link with lpthread
#include <dirent.h> //For directories
#include <errno.h> //For Errno (FILE)
#include <sys/stat.h>  //For stat()

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define DEBUG 1
//the thread function
void *connection_handler(void *);
int check_file_type(FILE *file, char* RequestURL);

int main(int argc , char *argv[])
{
	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	int portno; /* port to listen on */


	/* 
   * check command line arguments 
   */
  	if (argc != 2) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(1);
  	}
  	portno = atoi(argv[1]);
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
    //server.sin_addr.s_addr = inet_addr("10.224.76.120");
	server.sin_port = htons( portno );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("could not create thread");
			return 1;
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
        //Send some messages to the client
	    
	
	    // char *message = "Greetings! I am your connection handler\nNow type something and i shall repeat what you type \n";
	    // send(client_sock , message , strlen(message),0);
		puts("Handler assigned");
	}
	
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}
	
	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size, length, error =200, type = 7, isimage=0;
	char *message , client_message[2000], file_length[20];
    char *RequestMethod, *RequestURL;
	char ret [10000], FinalVersion[2000], FinalType[1000];
	size_t bytes_read;
	DIR *dir; FILE *file; FILE *out_bin; 
	struct stat sb;
    struct dirent *entry;
	char *RequestVersion; 
	char *ifClose;
	
	
	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{
		//Process Client Message
        char *token = strtok(client_message, " \t");
		ifClose = strstr(client_message, "Close");
		if (ifClose != NULL){
			puts("Client Closed Via Request\n");
			return 0; //EXIT
		}
        RequestMethod = token;
        token = strtok(NULL, " \t");
        RequestURL = token;
		for (int i = 0; RequestURL[i] != '\0'; i++){RequestURL[i] = RequestURL[i+1];}
        token = strtok(NULL, " \t");
        RequestVersion = token;
		//strcpy(RequestVersion,token);
		RequestVersion[8] = '\0';
        #if DEBUG
		printf ("WHAT:%s:%s:%s|",RequestMethod,RequestURL,RequestVersion);
		#endif
		//Method Check
		if (strcmp(RequestMethod, "GET") != 0){send(sock , "405 Method Not Allowed\n" , strlen("405 Method Not Allowed\n"),0);connection_handler(&sock);}
		//Version Check
		if ((strncmp(RequestVersion, "HTTP/1.1",sizeof("HTTP/1.1")) != 0) && (strncmp(RequestVersion, "HTTP/1.0",sizeof("HTTP/1.0")) != 0)){ send(sock , "505 HTTP Version Not Supported\n" , strlen("505 HTTP Version Not Supported\n"),0);connection_handler(&sock);}
		strcpy(FinalVersion, RequestVersion);
		
		//Stat
		#if DEBUG
		printf("STAT NOT OPEN%s\n", RequestVersion);
		#endif
		if (stat(RequestURL, &sb) == 0){
			if (S_ISDIR(sb.st_mode)){
				#if DEBUG
				printf("IS DIR\n");
				#endif
				//Open Dir
				dir = opendir(RequestURL);
        		if (dir == NULL) {send(sock , "400 Bad Request\n" , strlen("400 Bad Request\n"),0); perror ("Can't find directory"); connection_handler(&sock);} 
				strcat(RequestURL, "/index.html");
			}
			else{
				#if DEBUG
				printf("IS FILE\n");
				#endif
			}
		}
		

        
		//Determine File
        file = fopen(RequestURL, "rb");
		if (file == NULL){
			perror("Error opening file");
			switch(errno){
				case EACCES: //Insufficient Permissions
					send(sock , "403 Forbidden\n" , strlen("403 Forbidden\n"),0);
					connection_handler(&sock);
				break;
				case EISDIR://Is a directory SHOULD NEVER BE HERE
					strcat(RequestURL, "/index.html");
					file = fopen(RequestURL, "rb");
				break;
				case EINTR://No such File
					send(sock , "404 Not Found\n" , strlen("404 Not Found\n"),0);
					connection_handler(&sock);
				break;
				default:
					send(sock , "404 Not Found\n" , strlen("404 Not Found\n"),0);
					connection_handler(&sock);
				break;
			}
		}
		
        fseek(file, 0, SEEK_END); length = ftell(file); fseek(file, 0 , SEEK_SET);
		message = (char *)malloc(length+1);
        fread(message, length, 1, file);
        message[length] = '\0';
		fseek(file, 0 , SEEK_SET);
		//CALL check_type
		type = check_file_type(file, RequestURL);
		fseek(file, 0 , SEEK_SET);
		switch(type){
			case 0:
				strcpy(FinalType,"text/html");
				
			break;
			case 1:
				strcpy(FinalType,"image/png");
				isimage = 1;
				
			break;
			case 2:
				strcpy(FinalType,"image/gif");
				isimage = 1;
				
			break;
			case 3:
				strcpy(FinalType,"image/jpg");
				isimage = 1;
				
			break;
			case 4:
				strcpy(FinalType,"image/x-icon");
				isimage = 1;
				
			break;
			case 5:
				strcpy(FinalType,"text/css");
				
			break;
			case 6:
				strcpy(FinalType,"application/javascript");
				isimage = 1;
				
			break;
			default:
				strcpy(FinalType,"text/plain");
			break;
		}
        
		//Process Request
        printf("METHOD: %s, URL:%s, VERSION:%s TYPE:%s\n", RequestMethod, RequestURL, RequestVersion, FinalType);
        strcat(FinalVersion, " 200 OK\r\n");
        strcat(FinalVersion, "Content-Type: ");
		strcat(FinalVersion, FinalType);
		strcat(FinalVersion, " \r\n");
        sprintf(file_length, "%d", length);
        strcat(FinalVersion, "Content-Length: ");
        strcat(FinalVersion, file_length);
        strcat(FinalVersion, "\r\n\r\n");
		//Process File content
		if(isimage == 0){ 
			strcat(FinalVersion, message);
			printf("HERE:%s\n", FinalVersion);
        	send(sock , FinalVersion, strlen(FinalVersion),0);
			memset(RequestVersion, '\0', strlen(RequestVersion));
			memset(RequestURL, '\0', strlen(RequestURL));
			memset(RequestMethod, '\0', strlen(RequestMethod));
			memset(message, '\0', strlen(message));
			memset(FinalVersion, '\0', strlen(FinalVersion));
			
		}
		else{
			printf("HERE:%s\n", FinalVersion);
			send(sock , FinalVersion, strlen(FinalVersion),0);
			//while ((bytes_read = fgetc(file)) != EOF){fputc(bytes_read,socket_desc);}
			send(sock , message, length,0);
		}
        
        
		//memset(message, '\0', sizeof(message));
        
	}

	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}
		
	//Free the socket pointer and message
	free(socket_desc);
	// free(message);
	
	return 0;
}

int check_file_type(FILE *file, char* RequestURL){
	char buffer[32];
	const char *html_signature = "<!DOCTYPE html>";
	const char *png_signature = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"; // PNG file signature
	const char *gif_signature = "GIF87a"; // GIF file signature
	const char *gif_signature2 = "GIF89a"; // GIF file signature2
	const char *jpg_signature = "\xFF\xD8\xFF"; // JPEG file signature
	const char *ico_signature = "\x00\x00\x01\x00"; // ICO file signature
	const char *css_signature = "/*"; // CSS file signature
	const char *js_signature = "//"; // JavaScript file signature

	// size_t type_check = fread(buffer, MIN(sizeof(buffer),length), 1, file);
	size_t type_check = fread(buffer, 1,sizeof(buffer), file);

	

    if ( type_check >= strlen(html_signature) && memcmp(buffer, html_signature, strlen(html_signature)) == 0){
			return 0; //HTML
		}
	else if ( type_check >= strlen(png_signature) && memcmp(buffer, png_signature, strlen(png_signature)) == 0){
			return 1; //PNG
		}
	else if ( type_check >= strlen(gif_signature) && memcmp(buffer, gif_signature, strlen(gif_signature)) == 0){
			return 2; //GIF
		}
	else if ( type_check >= strlen(gif_signature2) && memcmp(buffer, gif_signature2, strlen(gif_signature2)) == 0){
			return 2; //GIF
		}
	else if ( type_check >= strlen(jpg_signature) && memcmp(buffer, jpg_signature, strlen(jpg_signature)) == 0){
			return 3; //JPG
		}
	else if ( type_check >= strlen(ico_signature) && memcmp(buffer, ico_signature, strlen(ico_signature)) == 0){
			if(RequestURL[strlen(RequestURL)-1] =='l'){
				return 0;
			}
			if(RequestURL[strlen(RequestURL)-1] =='t'){
				return 7;
			}
			if(RequestURL[strlen(RequestURL)-1] =='s'){
				if(RequestURL[strlen(RequestURL)-2] =='s'){
				return 5;
			}
			return 6;
			}
			return 4;
		}
	else if ( type_check >= strlen(css_signature) && memcmp(buffer, css_signature, strlen(css_signature)) == 0){
			return 5; //CSS
		}
	else if ( type_check >= strlen(js_signature) && memcmp(buffer, js_signature, strlen(js_signature)) == 0){
			return 6; //js
		}
		else{return 7;}
	return 7;
	
}