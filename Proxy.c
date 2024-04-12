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
#include <netdb.h> //For gethostbyname()
#include <openssl/evp.h> //For MD5()
#include <time.h> //For cache timing

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define DEBUG 1
//the thread function
void *connection_handler(void *);
void connect_server(char *client_message,struct hostent *host_info, void *proxy_sock,int port,char *RequestURL);
int check_file_type(FILE *file, char* RequestURL);
int is_cached(char* RequestURL, char* digest);

typedef struct {
	char url[256];
	char content[20000];
	time_t timestamp;
}CachedPage;


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
int is_cached(char *RequestURL, char *digest){
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	int md_len;
	
	// Initialize the MD context
    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        // Handle error
        return -1;
    }
    // Set the MD algorithm (MD5 in this case)
    md = EVP_md5();
    if (md == NULL) {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    // Initialize the MD context for the chosen algorithm
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    // Update the digest with the input data
    if (EVP_DigestUpdate(mdctx, RequestURL, strlen(RequestURL)) != 1) {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    // Finalize the digest and store it in the output buffer
    if (EVP_DigestFinal_ex(mdctx, digest, &md_len) != 1) {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
	
}

void connect_server(char *client_message,struct hostent *host_info, void *proxy_sock, int port, char *RequestURL){
	int socket_desc;
	int sock = *(int*)proxy_sock;
	struct sockaddr_in server;
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	char* URL;
	printf("AHHH%s\n",RequestURL);
	strcpy(URL, RequestURL);
	printf("SENDING:\n%s\n",client_message);//Print Message recieved by HTML site
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket for Server created");
	server.sin_family = AF_INET;
	//memcpy(&server.sin_addr.s_addr, host_info->h_addr, host_info->h_length);
	//char IP[] = inet_ntoa(*(struct in_addr*)host_info->h_addr);
	//printf("IP:%s",IP);
	server.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)host_info->h_addr));
	server.sin_port = htons( port );
    

//Connect with Client
	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Error connecting to server");
        return;
    }
//Forward Request to Client
	if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
        perror("Error sending request");
        return;
    }
	memset(client_message, 0 ,20000);
	int bytes_received;
	printf("AHHH%s\n",URL);
	FILE *file = fopen(URL, "ab");
	if (file == NULL){printf("ERROR w/ FILE");}
	while((bytes_received = recv(socket_desc, client_message, 1000, 0))>0){
    	if (bytes_received < 0){
        	perror("ERROR reading from server socket");
		}
		printf("Responce:%d|%s\n",bytes_received,client_message);
		if(fwrite(client_message, 1, bytes_received, file) != bytes_received){printf("ERROR WRITING TO OUT");fclose(file);}
	//Forawrd Response to Client
		printf("Here");
		if(send(sock, client_message, bytes_received, 0) < 0){
    		perror("ERROR writing to client socket");
		}
	}
	fclose(file);
	close(socket_desc);//Close Output socket
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size, length, error =200, type = 7, isimage=0, port;
	char *message , client_message[20000], file_length[20];
    char *RequestMethod, *RequestURL, *RequestHost;
	char CompareMethod[256];
	char FinalVersion[2000], FinalType[1000], FinalURL[256];
	char host[256];
	size_t bytes_read;
	DIR *dir; FILE *file; FILE *out_bin; 
	struct stat sb;
    struct dirent *entry;
	char *RequestVersion; 
	char *ifClose;
	struct hostent *host_info;
	FILE *block;
	int blocked = 0;
	char line[256];
	char digest[256];
	
	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{
	//Extract Host, Request Method, Request Version and Request URL
		char *hostPtr = strstr(client_message, "Host:");
		if (hostPtr != NULL){
			hostPtr += strlen("Host: ");
			int i = 0;
			while(hostPtr[i] != ' ' && hostPtr[i] != '\r' && hostPtr[i] != '\n' && hostPtr[i] != '\0' && hostPtr[i] != ':'){
				host[i] = hostPtr[i];
				i++;
			}
			host[i] = '\0';
			char *portPtr = strchr(host, ':');
			if (portPtr != NULL){
				port = atoi(portPtr + 1);
			}
			
			
		}
		else{
			printf("Host Not Found");
		}
		strcpy(FinalVersion, client_message);
        char *token = strtok(client_message, " \t");
        RequestMethod = token;
        token = strtok(NULL, " \t");
        RequestURL = token;
        token = strtok(NULL, " \t");
        RequestVersion = token;
		RequestVersion[8] = '\0';

		

        //RequestHost = token;
        
		strcpy(CompareMethod, RequestMethod);
		//printf ("WHAT:%s:%s:%s:%s|",RequestMethod,RequestURL,RequestVersion,RequestHost);
	//Method Check
		if (strcmp(CompareMethod, "GET") != 0){
			
			//send(sock , "405 Method Not Allowed\n" , strlen("405 Method Not Allowed\n"),0);
			//connection_handler(&sock);
		}

	//Version Check
		if ((strncmp(RequestVersion, "HTTP/1.1",sizeof("HTTP/1.1")) != 0) && (strncmp(RequestVersion, "HTTP/1.0",sizeof("HTTP/1.0")) != 0)){ send(sock , "505 HTTP Version Not Supported\n" , strlen("505 HTTP Version Not Supported\n"),0);connection_handler(&sock);}
		
	//UHostCheck
		if (strcmp(host, "detectportal.firefox.com") == 0 || strcmp(host, "detectportal.firefox") == 0 || strcmp(CompareMethod, "GET") != 0){
			//printf ("OH MY %s\n",host);
			//recv(sock,client_message,2000,0);
			//DO NOTHING
		}
	//Send request to Server
		else{
			
			printf ("WHAT:%s:%s:%s:%s|\n",RequestMethod,RequestURL,RequestVersion,host);
			//Determine Valid URL
			host_info = gethostbyname(host);
			if (host_info == NULL){
				printf("HOST NAME IS INVALID\n");
			}
			else{
				printf("HOST VERIFIED: %s\n", host_info->h_name);
				struct in_addr **addr_list;
    			addr_list = (struct in_addr **)host_info->h_addr_list;
    			if (addr_list[0] != NULL) {
        		//printf("%s\n", inet_ntoa(*addr_list[0]));
    				}
				block = fopen("blocklist", "r");
				while(fgets(line, 256, block) != NULL){
					 // Remove the newline character if present
        			if (line[strlen(line) - 1] == '\n') {
         			   line[strlen(line) - 1] = '\0';
        			}

        			// Compare the line with the string to match
        			if ((strcmp(line, host_info->h_name) == 0) || (strcmp(line, inet_ntoa(*addr_list[0])) == 0)) {
         			   blocked = 1;
					   printf("BLOCKED\n\n\n\n");
        			}
				}
				
				
			}
			//Connect to HTTP
			
			// strcat(FinalVersion, RequestMethod); strcat(FinalVersion, " ");
			// strcat(FinalVersion, RequestURL); strcat(FinalVersion, " ");
			// strcat(FinalVersion, RequestVersion); strcat(FinalVersion, " ");
			int *new_sock;
			new_sock = malloc(1);
			*new_sock = sock;
			is_cached(RequestURL,digest);
			snprintf(FinalURL, sizeof(FinalURL), "%s.bin", digest);
			if(!blocked){
				connect_server(FinalVersion,host_info,(void*)new_sock,80,FinalURL);
			}
			else{
				send(sock , "403 Forbidden\n" , strlen("403 Forbidden\n"),0);
			}
		}
        printf("END OF LOOP\n");
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