#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    time_t t = time(NULL);

    // 標頭強制指定 UTF-8
    printf("HTTP/1.1 200 OK\r\n");
    printf("Content-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n");

    printf("<html><head><meta charset='UTF-8'></head><body>");

    printf("<h1>CGI Dynamic Page</h1>");
    printf("<p>Linux network programming exercise</p>");

    printf("<hr>");
    printf("<p>Server Time: %s</p>", ctime(&t));

    char* remote_ip = getenv("REMOTE_ADDR");
    char* remote_port = getenv("REMOTE_PORT");

    if (remote_ip) {
        printf("<p style='color:blue;'>Your IP: %s</p>", remote_ip);
    }

    if (remote_port) printf("<p>Your port: %s</p>", remote_port);

    printf("<br><a href='/index.html'>Back to Home</a>");
    printf("</body></html>");

    return 0;
}