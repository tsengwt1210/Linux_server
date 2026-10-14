// 課程練習：fork 處理連線，提供靜態檔案及固定的 /cgi 入口。
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "../io_helpers.h"

const int port = 9998;
const size_t Maxbuff = 1024;

const char* get_respmessage(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return NULL;
    if (!strcasecmp(ext, ".html") || !strcasecmp(ext, ".htm")) return "text/html; charset=utf-8";
    if (!strcasecmp(ext, ".jpg") || !strcasecmp(ext, ".jpeg")) return "image/jpeg";
    if (!strcasecmp(ext, ".png")) return "image/png";
    if (!strcasecmp(ext, ".gif")) return "image/gif";
    return NULL;
}

void reply_error(int fd, const char* status) {
    char reply[256];
    int n = snprintf(reply, sizeof(reply),
        "HTTP/1.1 %s\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n%s\n", status, status);
    if (n > 0 && n < static_cast<int>(sizeof(reply))) write_all(fd, reply, static_cast<size_t>(n));
}

void sig_handler(int signo) {
    if (signo == SIGCHLD) {
        int saved = errno;
        while (waitpid(-1, NULL, WNOHANG) > 0) {}
        errno = saved;
    }
}

void serve_client(int fd, const sockaddr_in& client) {
    // 只處理一個短 GET 請求；讀完標頭，避免把 TCP 分段當成完整請求。
    timeval timeout = {5, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    char request[Maxbuff] = {};
    size_t used = 0;
    while (strstr(request, "\r\n\r\n") == NULL) {
        if (used == sizeof(request) - 1) { reply_error(fd, "431 Request Header Fields Too Large"); return; }
        ssize_t n = read(fd, request + used, sizeof(request) - 1 - used);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { reply_error(fd, "400 Bad Request"); return; }
        if (memchr(request + used, '\0', static_cast<size_t>(n))) {
            reply_error(fd, "400 Bad Request"); return;
        }
        used += static_cast<size_t>(n);
        request[used] = '\0';
    }
    char method[16] = {}, target[512] = {}, version[16] = {};
    if (sscanf(request, "%15s %511s %15s", method, target, version) != 3 ||
        (strcmp(version, "HTTP/1.1") && strcmp(version, "HTTP/1.0"))) {
        reply_error(fd, "400 Bad Request"); return;
    }
    if (strcmp(method, "GET")) { reply_error(fd, "405 Method Not Allowed"); return; }
    if (!strcmp(target, "/")) strcpy(target, "/index.html");
    // 本作業只服務目前目錄中的單一檔名；不接受子目錄、編碼路徑或查詢參數。
    if (target[0] != '/' || target[1] == '\0' ||
        strspn(target + 1, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != strlen(target + 1) ||
        strstr(target, "..")) {
        reply_error(fd, "403 Forbidden"); return;
    }

    if (!strcmp(target, "/cgi")) {
        if (access("./cgi", X_OK) != 0) { reply_error(fd, "500 Internal Server Error"); return; }
        setenv("REMOTE_ADDR", inet_ntoa(client.sin_addr), 1);
        char p_str[10];
        snprintf(p_str, sizeof(p_str), "%u", ntohs(client.sin_port));
        setenv("REMOTE_PORT", p_str, 1);
        if (dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) return;
        if (fd > STDERR_FILENO) close(fd);
        execl("./cgi", "./cgi", static_cast<char*>(NULL));
        reply_error(STDOUT_FILENO, "500 Internal Server Error");
        return;
    }

    const char* content_type = get_respmessage(target);
    if (!content_type) { reply_error(fd, "415 Unsupported Media Type"); return; }
    int file = open(target + 1, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    struct stat st = {};
    if (file < 0) { reply_error(fd, "404 Not Found"); return; }
    if (fstat(file, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(file); reply_error(fd, "403 Forbidden"); return;
    }
    char header[256];
    int length = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n",
        content_type, static_cast<long long>(st.st_size));
    bool ok = length > 0 && length < static_cast<int>(sizeof(header)) && write_all(fd, header, static_cast<size_t>(length));
    char buffer[Maxbuff];
    while (ok) {
        ssize_t n = read(file, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        ok = write_all(fd, buffer, static_cast<size_t>(n));
    }
    close(file);
}

int main() {
    signal(SIGCHLD, sig_handler);
    signal(SIGPIPE, SIG_IGN);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 || listen(sockfd, 5) != 0) {
        perror("bind/listen"); close(sockfd); return 1;
    }
    printf("Server starts at http://127.0.0.1:%d/\n", port);
    while (1) {
        sockaddr_in client = {};
        socklen_t length = sizeof(client);
        int fd = accept(sockfd, (struct sockaddr*)&client, &length);
        if (fd < 0) { if (errno != EINTR) perror("accept"); continue; }
        pid_t pid = fork();
        if (pid == 0) {
            close(sockfd);
            serve_client(fd, client);
            close(fd);
            exit(0);
        }
        if (pid < 0) perror("fork");
        close(fd);
    }
}
