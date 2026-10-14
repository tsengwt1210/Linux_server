#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include "../io_helpers.h"

int main() {
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    int sock0 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock0 < 0) { perror("socket"); return 1; }
    int reuse = 1;
    setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sock0, (struct sockaddr*)&addr, sizeof(addr)) != 0 || listen(sock0, 5) != 0) {
        perror("bind/listen"); close(sock0); return 1;
    }
    printf("Server is running on 127.0.0.1:12345\n");
    while (1) {
        struct sockaddr_in client = {};
        socklen_t len = sizeof(client);
        int sock = accept(sock0, (struct sockaddr*)&client, &len);
        if (sock < 0) {
            if (errno != EINTR) perror("accept");
            continue;
        }
        // 每條連線由一個子行程接收；父行程繼續 accept。
        pid_t pid = fork();
        if (pid == 0) {
            close(sock0);
            char filename[256] = {};
            if (!read_exact(sock, filename, sizeof(filename)) ||
                memchr(filename, '\0', sizeof(filename)) == NULL || filename[0] == '\0' ||
                strspn(filename, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != strlen(filename)) {
                fprintf(stderr, "Invalid or incomplete filename\n");
                close(sock); exit(1);
            }
            // 保留課堂版的 PID 命名方式：不同連線會產生不同檔案。
            char save_filename[512];
            snprintf(save_filename, sizeof(save_filename), "Client_%d_%s", getpid(), filename);
            int file_fd = open(save_filename, O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW, 0644);
            struct stat st = {};
            if (file_fd < 0 || fstat(file_fd, &st) != 0 || !S_ISREG(st.st_mode)) {
                perror("open output");
                if (file_fd >= 0) close(file_fd);
                close(sock); exit(1);
            }
            long long current_size = lseek(file_fd, 0, SEEK_END);
            if (current_size < 0 || !write_all(sock, &current_size, sizeof(current_size))) {
                perror("send offset"); close(file_fd); close(sock); exit(1);
            }
            char buffer[1024];
            bool ok = true;
            while (1) {
                ssize_t n = read(sock, buffer, sizeof(buffer));
                if (n < 0 && errno == EINTR) continue;
                if (n < 0) { perror("receive"); ok = false; break; }
                if (n == 0) break;
                if (!write_all(file_fd, buffer, static_cast<size_t>(n))) {
                    perror("write output"); ok = false; break;
                }
            }
            close(file_fd);
            close(sock);
            if (ok) printf("Saved %s\n", save_filename);
            exit(ok ? 0 : 1);
        }
        if (pid < 0) perror("fork");
        close(sock);
    }
}
