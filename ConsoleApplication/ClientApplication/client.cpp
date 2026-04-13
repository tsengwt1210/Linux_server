#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> // 轉換 IP 地址格式
#include <fcntl.h> //file

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
	printf("connecting server\n");

	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		perror("connect error");
		return 1;
	}
	printf("connect successful\n");

	//傳送檔名
	if (write(sock, filename, sizeof(filename)) < 0){
		perror("Client: filed to transfer file name");
		close(sock);
		return 1;
	}

	//取得目前傳送進度
	long long offset = 0;
	if (read(sock, &offset, sizeof(offset)) < 0) {
		perror("Client: filed to transfer file name");
		close(sock);
		return 1;
	}
	printf("Server already has %lld bytes and is preparing for fast transfer...\n", offset);

	//打開本地檔案
	int file_fd = open(filename, O_RDONLY);
	if (file_fd < 0) {
		perror("Client: failed to open local file. Please check if the file exists.");
		close(sock);
		return 1;
	}
	//快轉檔案指標
	if (lseek(file_fd, offset, SEEK_SET) < 0) {
		perror("Client: file indicators failed to transfer quickly");
		close(sock);
		close(file_fd);
	}

	//傳送回圈
	char buffer[1024];
	int bytes_read;
	printf("start ttansferring file contents\n");
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
		if (write(sock, buffer, bytes_read) < 0) {
			perror("Client: network transmission failed; connection may be lost");
			break;
		}
	}
	if (bytes_read < 0) {
		perror("Client: an error occurred while reading a local file");
	}
	else {
		printf("\nfile transfer complete!\n");
	}

	close(sock);
	close(file_fd);

	return 0;
}