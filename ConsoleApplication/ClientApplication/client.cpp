#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> // 這個標頭檔是為了轉換 IP 地址格式
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
	printf("connecting server");

	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		perror("connect error");
		return 1;
	}
	printf("connect successful");

	//傳送檔名
	if (write(sock, filename, sizeof(filename)) < 0){
		perror("Client: 傳送檔名失敗");
		close(sock);
		return 1;
	}

	//取得目前傳送進度
	long long offset = 0;
	if (read(sock, &offset, sizeof(offset)) < 0) {
		perror("Client: 傳送檔名失敗");
		close(sock);
		return 1;
	}
	printf("Server 已經有 %lld bytes，準備進行快轉...\n", offset);

	//打開本地檔案
	int file_fd = open(filename, O_RDONLY);
	if (file_fd < 0) {
		perror("Client: 打開本地檔案失敗 (請確認檔案是否存在)");
		close(sock);
		return 1;
	}
	//快轉檔案指標
	if (lseek(file_fd, offset, SEEK_SET) < 0) {
		perror("Client: 檔案指標快轉失敗");
		close(sock);
		close(file_fd);
	}

	//傳送回圈
	char buffer[1024];
	int bytes_read;
	printf("開始傳送檔案內容");
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
		if (write(sock, buffer, bytes_read) < 0) {
			perror("Client: 網路傳送失敗 (可能斷線)");
			break;
		}
	}
	if (bytes_read < 0) {
		perror("Client: 讀取本地檔案發生錯誤");
	}
	else {
		printf("\n檔案傳送完畢！\n");
	}

	close(sock);
	close(file_fd);

	return 0;
}