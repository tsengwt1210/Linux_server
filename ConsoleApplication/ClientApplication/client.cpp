#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include "../io_helpers.h"

int main() {
    signal(SIGPIPE, SIG_IGN);
    // 檔名固定 256 bytes，其後回傳 native long long offset。
    char filename[256] = "homework.txt";
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) { perror("open homework.txt"); return 1; }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); close(file_fd); return 1; }
    struct sockaddr_in server = {};
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        perror("connect"); close(sock); close(file_fd); return 1;
    }
    long long offset = 0;
    struct stat st = {};
    if (!write_all(sock, filename, sizeof(filename)) ||
        !read_exact(sock, &offset, sizeof(offset)) ||
        fstat(file_fd, &st) != 0 || offset < 0 || offset > st.st_size ||
        lseek(file_fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "Failed handshake or invalid file offset\n");
        close(sock); close(file_fd); return 1;
    }
    printf("Server offset: %lld bytes\n", offset);
    bool ok = true;
    char buffer[1024];
    while (1) {
        ssize_t n = read(file_fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { perror("read file"); ok = false; break; }
        if (n == 0) break;
        if (!write_all(sock, buffer, static_cast<size_t>(n))) {
            perror("send file"); ok = false; break;
        }
    }
    close(sock);
    close(file_fd);
    if (ok) printf("File contents sent. Compare the saved file to verify completion.\n");
    return ok ? 0 : 1;
}
