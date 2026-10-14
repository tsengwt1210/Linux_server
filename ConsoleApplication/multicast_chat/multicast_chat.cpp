#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define GROUP_IP "239.0.0.1"  // Multicast 專用的群組 IP 範圍
#define PORT 8888             // 聊天室使用的 Port
#define BUF_SIZE 1024         // 訊息緩衝區大小

int main() {
    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in dest_addr;
    struct ip_mreq mreq;

    // 步驟 1：建立 UDP Socket (是 SOCK_DGRAM)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket 建立失敗");
        exit(1); // 發生錯誤就結束程式
    }

    // 步驟 2：設定 Port 可以重複使用 (SO_REUSEADDR)
    // 這樣才能在同一台電腦開三個終端機自己測試
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
        perror("設定 SO_REUSEADDR 失敗");
        exit(1);
    }

    // 步驟 3：設定本地接收地址並綁定 (Bind)
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 接收來自所有網卡的封包

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("Bind 綁定失敗");
        exit(1);
    }

    // 步驟 4：加入 Multicast 群組 (核心步驟)
    mreq.imr_multiaddr.s_addr = inet_addr(GROUP_IP); // 設定群組 IP
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);   // 從預設網卡加入

    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("加入 Multicast 群組失敗");
        exit(1);
    }

    // 步驟 5：設定發送目標的地址結構 (往群組 IP 丟)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    dest_addr.sin_addr.s_addr = inet_addr(GROUP_IP);

    printf("==== 成功加入多播聊天室 (IP: %s) ====\n", GROUP_IP);
    printf("現在可以開始打字聊天了...\n");

    // 步驟 6：用 fork 處理「同時收發」的問題
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork 失敗");
        exit(1);
    }

    if (pid == 0) {
        // 子行程負責接收訊息
        char recv_buf[BUF_SIZE];
        while (1) {
            // 從 Socket 讀取別人傳來的訊息
            int n = recvfrom(sockfd, recv_buf, BUF_SIZE - 1, 0, NULL, 0);
            if (n < 0) {
                perror("接收訊息失敗");
                continue; // 失敗了不要退出，繼續等下一個
            }
            recv_buf[n] = '\0'; // 補上字串結尾符號，避免印出亂碼

            // 印出收到的訊息 (加個前綴區分)
            printf("[群組訊息]: %s", recv_buf);
        }
    }
    else {
        // 父行程負責發送訊息
        char send_buf[BUF_SIZE];
        while (1) {
            // 從鍵盤讀取使用者輸入的文字
            if (fgets(send_buf, BUF_SIZE, stdin) != NULL) {
                // 將文字發送到 Multicast 群組
                int n = sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                if (n < 0) {
                    perror("發送訊息失敗");
                }
            }
        }
    }

    // 關閉 Socket
    close(sockfd);
    return 0;
}