// HTTP 1.1 WWW server
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define DEBUG
#define filecount 12      // 支援的檔案類型數量
#define port 9998         // 伺服器執行的埠號 (Port)
#define Maxbuff 1024      // 緩衝區大小
#define workplace /home/www  // 伺服器工作目錄

// 定義支援的副檔名清單
#define sup_filetypelist \
{".html",".HTML",".htm",".HTM",".jpg",".JPG",\
".jpeg",".JPEG",".png",".PNG",".gif",".GIF"}

// 定義對應副檔名的 Content-Type (HTTP 標頭內容類型)
#define sup_respondmessagelist \
{"text/html", "text/html", "text/html", "text.html", "image/jpg", "image/jpg", \
"image/jpeg", "image/jpeg", "image/png", "image/png", "image/gif", "image/gif"}

char curpath[Maxbuff];

// 加上 const，告訴編譯器這些是不可修改的常數標籤
const char* filetypelist[] = sup_filetypelist;
const char* respondmessagelist[] = sup_respondmessagelist;

// 根據副檔名尋找對應的 HTTP Content-Type
// 1. 回傳型態要加上 const
const char* get_respmessage(char* filetype)
{
	// 2. 裡面的 ptr 也要加上 const
	const char* ptr = NULL;
	int i;
	for (i = 0; i < filecount; i++)
	{
		if (!strcmp(filetype, filetypelist[i]))
			ptr = respondmessagelist[i];
	}
	return ptr;
}

int main()
{
	getcwd(curpath, sizeof(curpath)); // 取得程式目前的執行路徑

	int sockfd;                     // 伺服器監聽用的 Socket
	struct sockaddr_in dest;        // 伺服器本身的地址資訊
	int s_addrlen = sizeof(dest);
	int clientfd;                   // 與客戶端連線後產生的 Socket
	struct sockaddr_in client_addr; // 客戶端的地址資訊
	int c_addrlen = sizeof(client_addr);
	socklen_t temp = c_addrlen;

	// 1. 建立 Socket (買電話)
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Server socket error"); // 錯誤處理
		exit(-1);
	}

#if defined (DEBUG)
	printf("Server socket() is OK...\n");
#endif

	// 初始化地址結構
	bzero(&dest, s_addrlen);
	dest.sin_family = PF_INET;           // 使用 IPv4
	dest.sin_port = htons(port);         // 設定埠號 (將主機字節序轉為網路字節序)
	dest.sin_addr.s_addr = htonl(INADDR_ANY); // 接受任何來源的連線

	// 2. 設定 Socket 選項：允許地址重複使用 (避免連線結束後 Port 鎖死)
	int sock_opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_opt, sizeof(sock_opt)) < 0)
	{
		perror("Server setsockopt() - SO_REUSERADDR error!");
		exit(-1);
	}

#if defined (DEBUG)
	printf("Server setsockopt() is OK...\n");
#endif

	// 3. 綁定地址與埠號 (掛門牌)
	if (bind(sockfd, (struct sockaddr*)&dest, s_addrlen) < 0)
	{
		perror("Server bind() error!");
		exit(-1);
	}

#if defined (DEBUG)
	printf("Server bind() is OK...\n");
#endif

	// 4. 開始監聽連線 (等待電話鈴響)
	if (listen(sockfd, 5) < 0)
	{
		perror("Server listen() error!");
		exit(-1);
	}

#if defined (DEBUG)
	printf("Server listen is OK...\n");
#endif

	// 伺服器主迴圈
	while (1)
	{
		// 5. 接受客戶端連線 (拿起電話)
		if ((clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &temp)) < 0)
		{
			perror("Server accept error!");
			continue; // 如果失敗則繼續等待下一個
		}

#if defined (DEBUG)
		printf("Server accept() is OK...\n");
#endif

		char recvbuff[Maxbuff];
		int len;
		// 6. 接收來自瀏覽器的 HTTP Request (聽客人說話)
		if ((len = recv(clientfd, recvbuff, Maxbuff, 0)) > 0)
		{
			// 將 Socket 轉換為文件指標，方便進行讀寫
			FILE* clientfile = fdopen(clientfd, "wb");
			if (clientfile == NULL)
				printf("Server writing error!!");
			else
			{
				char reqst[Maxbuff];
				// 解析請求行，例如: "GET /index.html HTTP/1.1"
				sscanf(recvbuff, "GET %s HTTP", reqst);
				if (strcmp(reqst, "/") == 0) // 如果只請求根目錄，預設給 index.html
					sprintf(reqst, "%s", "/index.html");

				char notuse[Maxbuff];
				char filetype[Maxbuff]; // 存放檔案類型 (副檔名)
				char realpath[Maxbuff]; // 存放檔案在電腦裡的實際路徑

				// 分離檔名與副檔名
				sscanf(reqst, "%[^.]%s", notuse, filetype);
				// 限制寫入的大小，不讓它擠爆空間
				snprintf(realpath, sizeof(realpath), "%s%s", curpath, reqst); // 組合成絕對路徑

				printf("Real Path: %s\n", realpath);
				printf("File Type: %s\n", filetype);

				// 7. 嘗試開啟請求的檔案
				FILE* ser_reqstfile;
				if ((ser_reqstfile = fopen(realpath, "rb")) != NULL)
				{
					char ackmessage[Maxbuff];
					memset(ackmessage, 0, Maxbuff);

					// 製作 HTTP Response 標頭 (狀態碼 200 OK)
					// 在 Content-Type 後面加上 charset=utf-8
					sprintf(ackmessage, "HTTP/1.1 200 OK\r\nContent-Type:%s; charset=utf-8\r\n\r\n", get_respmessage(filetype));
					fwrite(ackmessage, sizeof(char), strlen(ackmessage), clientfile);

					char ser_remessage[Maxbuff];
					long i;

					// 將檔案內容讀取並傳送給客戶端
					while ((i = fread(ser_remessage, sizeof(char), Maxbuff, ser_reqstfile)) > 0)
					{
						fwrite(ser_remessage, sizeof(char), i, clientfile);
						memset(ser_remessage, 0, Maxbuff);
					}
					fclose(ser_reqstfile);
				}
				else
				{
					printf("Open File Error.\n"); // 檔案不存在的處理
				}
				fclose(clientfile);
			}
		}
		close(clientfd); // 結束本次連線
	}
	close(sockfd);
	return 0;
}