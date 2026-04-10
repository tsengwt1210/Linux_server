#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> // 這個標頭檔是為了轉換 IP 地址格式

int main() {
	int sock;
	struct sockaddr_in server;
	char filename[] = "homework.txt";
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket error");
		return 1;
	}

	//設定sever 的IP和port(輸入對方的號碼)
	server.sin_family = AF_INET;
	server.sin_port = htons(12345);
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr.s_addr); //設定 IP 為本機端 (127.0.0.1)，因為我們的 Server 也跑在同一台電腦上
	printf("connecting server");

	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		perror("connect error");
		return 1;
	}
	printf("connect successful");

	//傳送檔名
	write(sock, filename, sizeof(filename));

	close(sock);

	return 0;
}