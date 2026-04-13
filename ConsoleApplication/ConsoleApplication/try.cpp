#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include <fcntl.h> //file
#include <signal.h> // 處理殭屍行程
#include <stdlib.h> // fork(), exit() 需要

int main() {
	int sock0,sock;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	socklen_t len;
	signal(SIGCHLD, SIG_IGN); //防止分身變殭屍

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
	//加入無窮迴圈讓老闆永遠不關門
	while (1) {
		len = sizeof(client);
		printf("Waiting for client to connect\n");
		sock = accept(sock0, (struct sockaddr*)&client, &len);// 程式執行到這會卡住直到有Client連進來為止  &len是因為須回報實際大小
		if (sock < 0) {
			perror("accept error");
			continue; // 如果這個客人失敗，不要 return 1 關門，繼續接下一個
		}
		printf("Client connected\n");

		//呼叫分身
		pid_t pid = fork();
		if (pid == 0) {
			//被生出來的分身
			close(sock0); //分身不用顧大門

			//聽取 Client 傳過來的檔名
			char filename[256] = { 0 };
			if (read(sock, filename, sizeof(filename)) > 0) {
				printf("client wants to upload: %s\n", filename);
			}
			else {
				perror("Server: failed to read file name");
				close(sock);
				exit(1); // 分身任務失敗，直接登出
			}

			//替檔案加上分身編號，避免多人上傳同檔名撞車
			char save_filename[512];
			sprintf(save_filename, "Client_%d_%s", getpid(), filename);
			printf("[Clone Number %d] responsible for receiving the file, which are archived at %s\n", getpid(), save_filename);

			//建立新檔案準備寫入
			// O_APPEND: 確保新資料會接在檔案的最尾巴
			int file_fd = open(save_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (file_fd < 0) {
				perror("file creation failed");
				close(sock);
				exit(1);
			}

			//告知目前有多少檔案
			long long current_size = lseek(file_fd, 0, SEEK_END); //傳送大檔案
			if (current_size < 0)current_size = 0;
			else {
				printf("the fil currently contains  %lld bytes, notify Client to continue transformitting from here...\n", current_size);
				write(sock, &current_size, sizeof(current_size));
			}

			//接收迴圈
			char buffer[1024];
			int bytes_received;
			while ((bytes_received = read(sock, buffer, sizeof(buffer))) > 0) {
				// 將收到的包裹寫進硬碟檔案中
				if (write(file_fd, buffer, bytes_received) < 0) {
					perror("Server: failed to write to hard drive");
					break;
				}
			}
			if (bytes_received < 0) {
				perror("Server: an error occurred while receiving data; the connection may have been lost");
			}
			else {
				printf("\nfiles received and stored!\n");
			}
			close(file_fd);
			close(sock);
			exit(0); //分身做完事必須呼叫 exit(0) 消失，不然它會跑去搶接客
		}
		else if (pid > 0) {
			close(sock); //老闆的程式碼就到這裡，他會立刻順著 while(1) 跑回最上面，繼續卡在 accept() 等待下一個客人進門
		}
		else {
			perror("Server: calling the cline fork failed");
			close(sock);
		}
	}
	close(sock0);

	return 0;
}