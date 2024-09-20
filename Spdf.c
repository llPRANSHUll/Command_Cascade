#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#define PORT 6978 // set the default port 6978

// we have created function to create the tar file on receiving signal from main server 
void tarCreation(const char *tar_name, const char *filetype, const char *root_dir) {
    char command[512];
    snprintf(command, sizeof(command), "find %s -type f -name '*%s' -print0 | xargs -0 tar -cvf %s", root_dir, filetype, tar_name);
    system(command);
}

// this function create the directory spdf on reciveing signal 
void handleFileTransfer(int new_socket, char *filename, char *dest_path) {
    // Print extracted paths for debugging
    printf("Filename: %s\n", filename);
    printf("Destination path: %s\n", dest_path);

    // Check if the destination directory exists; if not, create it
    struct stat st = {0};
    if (stat(dest_path, &st) == -1) {
        // Create the destination directory if it doesn't exist, including intermediate directories
        char tmp[1024];
        snprintf(tmp, sizeof(tmp), "%s", dest_path);

        for (char *p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                if (mkdir(tmp, 0777) == -1 && errno != EEXIST) {
                    perror("Failed to create intermediate directory");
                    close(new_socket);
                    return;
                }
                *p = '/';
            }
        }

        if (mkdir(dest_path, 0777) == -1 && errno != EEXIST) {
            perror("Failed to create destination directory");
            close(new_socket);
            return;
        }
    } else {
        printf("Destination directory already exists.\n");
    }

    // Construct the full path for the file in the destination directory
    char new_file_path[1024];
    snprintf(new_file_path, sizeof(new_file_path), "%s/%s", dest_path, filename);

    // Open the destination file for writing
    int dest_fd = open(new_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (dest_fd < 0) {
        perror("Failed to open destination file for writing");
        close(new_socket);
        return;
    }

    // Receive the file size
    size_t file_size;
    ssize_t size_read = read(new_socket, &file_size, sizeof(size_t));
    if (size_read != sizeof(size_t)) {
        perror("Failed to receive file size");
        close(dest_fd);
        close(new_socket);
        return;
    }
    printf("Expected file size: %zu bytes\n", file_size);

    // Receive and write the file content
    size_t bytes_received = 0;
    char buffer[1024];
    ssize_t bytes_read;
    while (bytes_received < file_size) {
        bytes_read = read(new_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            perror("Error receiving file content");
            break;
        }
        if (write(dest_fd, buffer, bytes_read) < 0) {
            perror("Error writing file content to disk");
            break;
        }
        bytes_received += bytes_read;
    }

    if (bytes_received == file_size) {
        printf("File saved to: %s\n", new_file_path);
        send(new_socket, "File received and saved", strlen("File received and saved"), 0);
    } else {
        printf("Failed to receive complete file.\n");
        send(new_socket, "Failed to receive complete file", strlen("Failed to receive complete file"), 0);
    }

    close(dest_fd);
    close(new_socket);
}

// function create then file at directory created and then send the content of the file 
void handleFileRequest(int new_socket, char *filepath) {
    // Open the requested file
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0) {
        perror("Failed to open requested file");
        send(new_socket, "Failed to open requested file", strlen("Failed to open requested file"), 0);
        close(new_socket);
        return;
    }

    // Send the file size first
    struct stat st;
    if (fstat(file_fd, &st) != 0) {
        perror("Failed to get file size");
        close(file_fd);
        close(new_socket);
        return;
    }
    size_t file_size = st.st_size;
    send(new_socket, &file_size, sizeof(size_t), 0);
    printf("Sending file: %s, Size: %zu bytes\n", filepath, file_size);

    // Send the file content
    char buffer[1024];
    ssize_t bytes_read, bytes_sent = 0;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        if (send(new_socket, buffer, bytes_read, 0) < 0) {
            perror("Failed to send file content");
            break;
        }
        bytes_sent += bytes_read;
    }

    if (bytes_sent == file_size) {
        printf("File sent successfully: %s\n", filepath);
    } else {
        printf("Failed to send complete file: %s\n", filepath);
    }

    close(file_fd);
    close(new_socket);
}

// function get the rmfile command and it delete from the directory the pdf file 
void handleFileDeletion(int new_socket, char *filepath) {
    // Attempt to delete the specified file
    if (remove(filepath) == 0) {
        printf("File deleted: %s\n", filepath);
        send(new_socket, "File deleted successfully.", strlen("File deleted successfully."), 0);
    } else {
        perror("Failed to delete file");
        send(new_socket, "Failed to delete file.", strlen("Failed to delete file."), 0);
    }

    close(new_socket);
}

// this function list all the  pdf  file in spdf and send to smain 
void ListFile(int client_socket, const char *pathname, const char *filetype) {
    char command[1024];
    snprintf(command, sizeof(command), "find %s -type f -name '*%s'", pathname, filetype);
    printf("Running command: %s\n", command);  // Debug output

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run find command");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Found file: %s", buffer);  // Debug print for each file
        send(client_socket, buffer, strlen(buffer), 0);  // Send each line to Smain
    }

    pclose(fp);

    // Send end-of-list marker
    const char *end_of_list = "END_OF_LIST";
    send(client_socket, end_of_list, strlen(end_of_list), 0);
}

// this function manage all the request from smain and call the respective function to perform the task 
void handleRequest(int new_socket) {
    char buffer[2048] = {0};
    ssize_t bytes_read = read(new_socket, buffer, sizeof(buffer) - 1);

    if (bytes_read < 0) {
        perror("Read failed");
        close(new_socket);
        return;
    }

    // Null-terminate the received data
    buffer[bytes_read] = '\0';

    // Print the received data
    printf("PDF Server received data: %s\n", buffer);

    // Determine whether the request is to receive, send, or delete a file
    if (strncmp(buffer, "REQUEST_FILE;", 13) == 0) {
        char *filepath = buffer + 13; // The filepath starts after "REQUEST_FILE;"
        handleFileRequest(new_socket, filepath);
    } else if (strncmp(buffer, "DELETE_FILE;", 12) == 0) {
        char *filepath = buffer + 12; // The filepath starts after "DELETE_FILE;"
        handleFileDeletion(new_socket, filepath);
    } else if (strncmp(buffer, "REQUEST_PDF_TAR;", 16) == 0) {
        tarCreation("pdf.tar", ".pdf", "~/spdf");
        handleFileRequest(new_socket, "pdf.tar");
        remove("pdf.tar");
    } 
    else if (strncmp(buffer, "LIST_FILES;", 11) == 0) {
         char *pathname = buffer + 11;

        printf("Searching for .pdf files in: %s\n", pathname);
        ListFile(new_socket, pathname, ".pdf");  // Pass ".pdf
    }
    
    else {
        // Assume it's a file transfer if not a file request or deletion
        char *delimiter = strchr(buffer, ';');
        if (delimiter == NULL) {
            send(new_socket, "Invalid format", strlen("Invalid format"), 0);
            close(new_socket);
            return;
        }

        *delimiter = '\0'; // Split the string into filename and destination path
        char *filename = buffer;
        char *dest_path = delimiter + 1;

        handleFileTransfer(new_socket, filename, dest_path);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    // Set specific IP address
if (inet_pton(AF_INET, "10.60.8.51", &address.sin_addr) <= 0) {
    printf("Invalid address/Address not supported\n");
    return 0;
}

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept and handle incoming connections
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        handleRequest(new_socket);
    }

    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}