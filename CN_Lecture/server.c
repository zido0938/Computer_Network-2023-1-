#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// Function to get the MIME type of a file based on its extension
const char *get_mime_type(const char *file_path) {
    const char *extension = strrchr(file_path, '.');
    if (extension == NULL) {
        return "application/octet-stream";
    }
    if (strcmp(extension, ".html") == 0) {
        return "text/html";
    }
    if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
        return "image/jpeg";
    }
    if (strcmp(extension, ".gif") == 0) {
        return "image/gif";
    }
    if (strcmp(extension, ".png") == 0) {
        return "image/png";
    }
    if (strcmp(extension, ".css") == 0) {
        return "text/css";
    }
    return "application/octet-stream";
}

// Function to send an HTTP response to the client
void send_response(int client_fd, const char* request_buffer) {
    // Extract the filename from the request buffer
    char file_path[BUFFER_SIZE];
    if (sscanf(request_buffer, "GET /%s HTTP/1.1", file_path) != 1) {
        perror("sscanf");
        return;
    }

    // Construct the absolute path of the file
    char absolute_path[BUFFER_SIZE];
    snprintf(absolute_path, BUFFER_SIZE, "%s/%s", getcwd(NULL, 0), file_path);

    // Open the file for reading
    int file_fd = open(absolute_path, O_RDONLY);
    if (file_fd == -1) {
        perror("open");
        char *not_found_msg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Not Found</h1>";
        send(client_fd, not_found_msg, strlen(not_found_msg), 0);
        return;
    }

    // Get the file size
    struct stat st;
    if (fstat(file_fd, &st) == -1) {
        perror("fstat");
        close(file_fd);
        return;
    }
    off_t file_size = st.st_size;

    // Send HTTP response header
    char header_buffer[BUFFER_SIZE];
    int header_size = snprintf(header_buffer, BUFFER_SIZE,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        get_mime_type(absolute_path),
        (long)file_size);
    send(client_fd, header_buffer, header_size, 0);

    // Send file contents
    ssize_t bytes_sent = 0;
    while (bytes_sent < file_size) {
        ssize_t bytes_read = sendfile(client_fd, file_fd, NULL, file_size - bytes_sent);
        if (bytes_read == -1) {
            if (errno == EINTR) {
                // sendfile() was interrupted by a signal, so we try again
                continue;
            }
            perror("sendfile");
            close(file_fd);
            return;
        }
        bytes_sent += bytes_read;
    }

    // Close the file
    close(file_fd);
}

int main(int argc, char *argv[])
{
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        return 1;
    }

    int port_number = atoi(argv[1]);
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Hello from server";

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 포트와 IP 주소 설정
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_number);

    // 소켓에 바인드
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 클라이언트의 연결 요청 대기
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 클라이언트의 연결 수락
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // 클라이언트로부터 데이터 수신 및 전송
    valread = read(new_socket, buffer, BUFFER_SIZE);
    printf("%s\n", buffer);

    // HTTP response message 생성 및 전송
    send_response(new_socket, buffer);

    return 0;
}

	

