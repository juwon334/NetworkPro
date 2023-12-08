#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void* request_handler(void* arg);
void send_data(int clnt_sock,char* ct, char* file_name);
char* content_type(char* file);
void send_error(int clnt_sock);
void error_handling(char* message);

int main(int argc,char* argv[]){
	int serv_sock,clnt_sock;
	struct sockaddr_in serv_adr,clnt_adr;
	int clnt_adr_size;
	char buf[BUF_SIZE];
	pthread_t t_id;
	
	serv_sock = socket(PF_INET,SOCK_STREAM,0);
	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if(bind(serv_sock,(struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock,20) == -1)
		error_handling("listen() error");
	while(1){
		clnt_adr_size = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_adr,&clnt_adr_size);
		printf("Connect Request : %s:%d\n", inet_ntoa(clnt_adr.sin_addr),ntohs(clnt_adr.sin_port));
		pthread_create(&t_id,NULL,request_handler, &clnt_sock);
		pthread_detach(t_id);
	}
	close(serv_sock);
	return 0;
}

void* request_handler(void *arg){
	int clnt_sock = *((int*)arg);
	char req_line[SMALL_BUF];
	int rcv_len = 0;

	char method[10];
	char ct[15];
	char file_name[30];

	rcv_len = read(clnt_sock, req_line, SMALL_BUF);

	if(rcv_len == 0){
		close(clnt_sock);
	}
	if(strstr(req_line,"HTTP/") == NULL)
	{
		send_error(clnt_sock);
		close(clnt_sock);
	}

	strcpy(method, strtok(req_line," /"));
	strcpy(file_name, strtok(NULL," /"));
	strcpy(ct,content_type(file_name));

	if(strcmp(method,"GET")!=0){
		send_error(clnt_sock);
		close(clnt_sock);
	}

	send_data(clnt_sock,ct,file_name);
	close(clnt_sock);
}

void send_data(int clnt_sock, char* ct, char* file_name){
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char server[] = "Server:Linux Web Server \r\n";
	char clnt_len[] = "Content-Length:2048\r\n";
	char cnt_type[SMALL_BUF];
	char buf[BUF_SIZE];
	FILE* send_file;
	int sent_len;

	sprintf(cnt_type,"Content-Type:%s\r\n\r\n", ct);
	send_file = fopen(file_name,"r");
	if(send_file == NULL){
		send_error(clnt_sock);
		close(clnt_sock);
		return;
	}
	write(clnt_sock,protocol,strlen(protocol));
	write(clnt_sock,server,strlen(server));
	write(clnt_sock,clnt_len,strlen(clnt_len));
	write(clnt_sock,cnt_type,strlen(cnt_type));

	while(fgets(buf,BUF_SIZE,send_file)!=NULL){
		sent_len = write(clnt_sock, buf, strlen(buf));
	}
	shutdown(clnt_sock,SHUT_WR);
}

char* content_type(char* file){
	char extension[SMALL_BUF];
	char file_name[SMALL_BUF];
	strcpy(file_name,file);
	strtok(file_name,".");
	strcpy(extension,strtok(NULL,"."));

	if(!strcmp(extension,"html")||!strcmp(extension,"htm"))
		return "text/html";
	else
		return "text/plain";
}

void send_error(int clnt_sock){
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char server[] = "Server:Linux Web Server \r\n";
	char clnt_len[] = "Content-Length:2048\r\n";
	char cnt_type[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><meta charset=\"UTF-8\"><title>NETWORK</title></head>"
		"<body><font size=+5><br>오류발생! 요청 파일명 및 요청 방식 확인!"
		"</font></body></html>";

	write(clnt_sock,protocol,strlen(protocol));
	write(clnt_sock,server,strlen(server));
	write(clnt_sock,clnt_len,strlen(clnt_len));
	write(clnt_sock,cnt_type,strlen(cnt_type));
	write(clnt_sock,content,strlen(content));
	shutdown(clnt_sock,SHUT_WR);

}

void error_handling(char* message)
{
	printf("error\n");
    exit(0);
}

