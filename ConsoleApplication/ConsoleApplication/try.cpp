#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>

int main() {
	int sock0,sock;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	socklen_t len;
	sock0 = socket(AF_INET, SOCK_STREAM, 0); //IPv4 TCP
	if (sock0 < 0) {
		printf("erroe \n");
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = INADDR_ANY;//接受任何IP連線
	if (bind(sock0, (struct sockaddr*)&addr, sizeof(addr)) != 0) {        //sizeof()單向告知
		perror("bind error");
		return 1;
	}

	if (listen(sock0, 5) != 0) {
		perror("listen error");
		return 1;
	} //排隊

	printf("Server is running on Port 12345\n");

	//等人連線進來
	len = sizeof(client);
	printf("Waiting for client to connect\n");
	sock = accept(sock0, (struct sockaddr*)&client, &len);// 程式執行到這會卡住直到有Client連進來為止  &len是因為須回報實際大小
	if (sock < 0) {
		perror("accept error");
		return 1;
	}
	printf("Client connected\n");
	
	//聽取 Client 傳過來的檔名
	char filename[256] = { 0 };
	if (read(sock, filename, sizeof(filename)) > 0) {
		printf("client wants to upload: %s\n",filename);
	}

	close(sock0);

	return 0;
}