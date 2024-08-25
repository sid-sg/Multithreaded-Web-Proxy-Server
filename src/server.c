#include "../include/parser.h"
#include "../include/server.h"
#include "../include/cache.h"

#define MAX_CLIENTS 50
#define MAX_BUFFER_BYTES 4096 

int portNo = 8080; //to-do: by default make the server assign a random port no. available on system
int proxySocketId;
pthread_t threadID[MAX_CLIENTS];
sem_t countingSemaphore;


int sendErrorMessage(int socket, int status_code)
{
	char str[1024];
	char currentTime[50];
	time_t now = time(0);

	struct tm data = *gmtime(&now);
	strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

	switch(status_code)
	{
		case 400: snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
				  printf("400 Bad Request\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 403: snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
				  printf("403 Forbidden\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 404: snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
				  printf("404 Not Found\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 500: snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
				  printf("500 Internal Server Error\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 501: snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
				  printf("501 Not Implemented\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 505: snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
				  printf("505 HTTP Version Not Supported\n");
				  send(socket, str, strlen(str), 0);
				  break;

		default:  return -1;

	}
	return 1;
}

int checkHTTPversion(char *msg)
{
	int version = -1;

	if(strncmp(msg, "HTTP/1.1", 8) == 0)
	{
		version = 1;
	}
	else if(strncmp(msg, "HTTP/1.0", 8) == 0)			
	{
		version = 1;										
	}
	else
		version = -1;

	return version;
}

int connectRemoteServer(char* hostAddress, int endServerPort){
    int remoteSocketID = socket(AF_INET, SOCK_STREAM, 0);
    if(remoteSocketID == -1){
        printf("socket creation failed\n");
        return -1;
    }   
    struct hostent* host = gethostbyname(hostAddress);
    if(host == NULL){
        printf("no such host exist\n");
        return -1;
    } 

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(endServerPort);

    memcpy( (char*)&serverAddress.sin_addr.s_addr, (char*)host->h_addr,host->h_length);
    
    if( connect(remoteSocketID, (struct sockaddr*)&serverAddress, (socklen_t)sizeof(serverAddress)) == -1 ){
        printf("error in connecting remote server\n");
        return -1;
    }

    return remoteSocketID;
}

int handleRequest(int clientSocketID, struct ParsedRequest *request, char* recievedBuffer ){
    char *buffer = (char*)malloc(MAX_BUFFER_BYTES*sizeof(char));
    strcpy(buffer, "GET ");
    strcat(buffer, request->path);
    strcat(buffer, " ");
    strcat(buffer, request->version);
    strcat(buffer, "\r\n");

    if( ParsedHeader_set(request, "Connection", "close") == -1){
        printf("set host header key is not working\n");
    }

    if( ParsedHeader_get(request, "Host") == NULL ){
        if( ParsedHeader_set(request, "Host", request->host) == -1){
            printf("set host header key is not working\n");
        }
    }

    int bufferLength = strlen(buffer);

    if( ParsedRequest_unparse_headers(request, buffer+bufferLength, (size_t)MAX_BUFFER_BYTES-bufferLength)== -1 ){
        printf("unparse failed");
    }

    int endServerPort = 80;
    if(request->port != NULL){
        endServerPort = atoi(request->port);
    }

    int remoteSocketID = connectRemoteServer(request->host, endServerPort);
    if(remoteSocketID == -1){
        return -1;
    }

    int bytesSend = send(remoteSocketID, buffer, strlen(buffer), 0);
    memset(buffer, 0, MAX_BUFFER_BYTES);

    bytesSend = recv(remoteSocketID, buffer, MAX_BUFFER_BYTES-1, 0);
    char *tempBuffer = (char*)malloc(sizeof(char)*MAX_BUFFER_BYTES);
    int tempIdx = 0;
    
    while(bytesSend>0){
        bytesSend = send(clientSocketID, buffer, bytesSend, 0);
        if(bytesSend==-1){
            printf("failed sending\n");
            break;
        }
        int size = bytesSend/sizeof(char);
        for(int i=0;i<size;i++){
            tempBuffer[tempIdx] = buffer[i];
            tempIdx++;
        }

        tempBuffer = (char*)realloc(tempBuffer, MAX_BUFFER_BYTES);
        memset(buffer, 0, MAX_BUFFER_BYTES);
        
        bytesSend = recv(remoteSocketID, buffer, MAX_BUFFER_BYTES-1, 0);
    }
    tempBuffer[tempIdx] = '\0';
    free(buffer);
    addCacheEle(tempBuffer, strlen(tempBuffer), recievedBuffer);
    free(tempBuffer);
    close(remoteSocketID);

    return 0;
}

void* threadFunction(void* newSocketID){
    sem_wait(&countingSemaphore);

    int semaphoreValue;
    sem_getvalue(&countingSemaphore, &semaphoreValue);
    printf("Semaphore value: %d\n",semaphoreValue);

    int* connectedClientIDSocketAddress = (int*) newSocketID;
    int connnectedClientSocketID = *connectedClientIDSocketAddress;

    char *buffer = (char*) calloc(MAX_BUFFER_BYTES, sizeof(char));  
    memset(buffer, 0, MAX_BUFFER_BYTES);

    int noOfBytesRecieved;
    noOfBytesRecieved = recv(connnectedClientSocketID, buffer, MAX_BUFFER_BYTES, 0); 

    // if(noOfBytesRecieved == -1){
    //     printf("failed recieving\n"); //todo: change pritf to something else remated to error handling
    //     exit(1);
    // }
    int lengthRecieved;

    while(noOfBytesRecieved>0){
        if( strstr(buffer, "\r\n\r\n") == NULL ){ 
            lengthRecieved = strlen(buffer);
            noOfBytesRecieved = recv(connnectedClientSocketID, buffer+lengthRecieved, MAX_BUFFER_BYTES-lengthRecieved, 0); 
            // if(noOfBytesRecieved == -1){
            //     printf("failed recieving\n"); //todo: change pritf to something else remated to error handling
            //     exit(1);
            // }
        }        
        else{
            break;
        }
    }

	printf("no of bytes recv: %d\n", noOfBytesRecieved);
	printf("buffer recieved: %s\n",buffer);

    char *recievedBuffer = (char*) malloc( strlen(buffer)*sizeof(char)+1 );
    for(int i=0;i<strlen(buffer);i++){
        recievedBuffer[i] = buffer[i];
    }
    recievedBuffer[strlen(buffer)-1] = '\0';

    struct cacheNode* cacheElement = findCacheEle(recievedBuffer);

    if(cacheElement != NULL){ //found in cache
	   printf("found in cache\n");	
       int dataSize = cacheElement->dataLength/sizeof(char);
       int pos = 0;
       char response[MAX_BUFFER_BYTES];
       while(pos<dataSize){
            memset(response, 0, MAX_BUFFER_BYTES);

            int bytesToSend = (dataSize-pos > MAX_BUFFER_BYTES)? MAX_BUFFER_BYTES:dataSize-pos;
            memcpy(response, cacheElement->data + pos, bytesToSend);
            pos += bytesToSend;

            if( send(connnectedClientSocketID, response, MAX_BUFFER_BYTES, 0) == -1 ){
                printf("sem init failed"); //todo: change pritf to something else remated to error handling
                // exit(1);
            }
       }

       printf("Data retrieved from cache\n");
       printf("%*s\n\n",MAX_BUFFER_BYTES,cacheElement->data); 
    }
    else if(noOfBytesRecieved>0){ //not found in cache
        printf("not found in cache\n");

        struct ParsedRequest* request = ParsedRequest_create();
        if( ParsedRequest_parse(request, buffer, strlen(buffer)) == -1 ){
            printf("parsing failed\n");
        }
        else{
            memset(buffer, 0, MAX_BUFFER_BYTES);

            if( strcmp(request->method,"GET") == 0 ){
                if( request->host && request->path && checkHTTPversion(request->version) == 1 ){
                    noOfBytesRecieved = handleRequest(connnectedClientSocketID, request, recievedBuffer);
                    if(noOfBytesRecieved == -1){
                        sendErrorMessage(connnectedClientSocketID, 500);
                    }
                }
            }
            else{
                printf("No other method supported other than GET\n");
            }
        }
        ParsedRequest_destroy(request);
    }
    else if(noOfBytesRecieved == 0){ //not recieved
        printf("Client is disconnected\n");
    }
    else{
        printf("failed recieving\n");
    }

    shutdown(connnectedClientSocketID, SHUT_RDWR);
    close(connnectedClientSocketID);
    free(buffer);
    free(recievedBuffer);

    sem_post(&countingSemaphore);
    sem_getvalue(&countingSemaphore, &semaphoreValue);
    printf("Semaphore post value: %d", semaphoreValue);

    return NULL;
}
 

int main(int argc, char* argv[]){

    int clientSocketID, clientAddressLength;
    struct sockaddr_in serverAddress, clientAddress;

    if( sem_init( &countingSemaphore, 0, MAX_CLIENTS) !=0 ){
        printf("sem init failed"); //todo: change pritf to something else remated to error handling
        exit(1);
    }      
    if( pthread_mutex_init(&mutex, NULL) ){
        printf("mutex init failed"); //todo: change pritf to something else remated to error handling
        exit(1);
    }

    if(argc == 2){
        portNo = atoi(argv[1]);
    }
    // else{
    //     printf("port no. not provided\n"); //todo: change pritf to something else remated to error handling
    //     exit(1);
    // } 

    printf("Starting Proxy Server at Port: %d\n", portNo);

    proxySocketId = socket(AF_INET, SOCK_STREAM, 0 );

    if(proxySocketId == -1){
        printf("socket creation failed\n"); //todo: change pritf to something else remated to error handling
        exit(1);
    }

    int reuse = 1;
    if( setsockopt(proxySocketId, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, (socklen_t)sizeof(reuse) ) == -1 ){
        printf("socket reuse failed\n"); //todo: change pritf to something else remated to error handling
        exit(1);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNo);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if( bind(proxySocketId, (const struct sockaddr*)&serverAddress, (socklen_t)sizeof(serverAddress)) == -1 ){
        printf("bind failed, port %d not available\n", portNo); //todo: change pritf to something else remated to error handling
        exit(1);
    }

    printf("Binding on port: %d\n",portNo);

    if( listen(proxySocketId, MAX_CLIENTS) == -1){
        printf("listen failed\n"); //todo: change pritf to something else remated to error handling
        exit(1);
    }

    int clientNumber = 0;
    int connectedSocketIDs[MAX_CLIENTS];

    while(1){
        clientAddressLength = sizeof(clientAddress);
        memset(&clientAddress, 0, clientAddressLength);
        clientSocketID = accept( proxySocketId, (struct sockaddr*)&clientAddress, (socklen_t*)&clientAddressLength);

        if(clientSocketID == -1){
            printf("connection failed\n"); //todo: change pritf to something else remated to error handling
            exit(1);
        }        
        else{
            connectedSocketIDs[clientNumber] = clientSocketID;
        }

        struct in_addr ipAddress = clientAddress.sin_addr;
        char IPstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipAddress, IPstr, INET_ADDRSTRLEN);
        
        printf("\nClient connected with Port Number: %d & IP Address: %s\n",ntohs(portNo), IPstr);

        pthread_create( &threadID[clientNumber], NULL, threadFunction, (void*)&connectedSocketIDs[clientNumber] );    
        clientNumber++;
    }

    close((proxySocketId));

    return 0;
}