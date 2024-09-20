#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_PORT 6969    // set the default port to 6969
#define PDF_SERVER_PORT 6978  // set the default port pdf port to 6978
#define TEXT_SERVER_PORT 6980   // set the deafult txt port 6980 

void send_to_pdf_server(int client_socket, const char *filename, const char *dest_path, const char *spdf_ip, size_t file_size) {
    struct sockaddr_in pdf_server_address;
    int sock = 0;
    char message[2048];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for PDF server\n");
        return;
    }

    pdf_server_address.sin_family = AF_INET;
    pdf_server_address.sin_port = htons(PDF_SERVER_PORT);

    if (inet_pton(AF_INET, spdf_ip, &pdf_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for PDF server\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&pdf_server_address, sizeof(pdf_server_address)) < 0) {
        printf("Connection failed to PDF server\n");
        close(sock);
        return;
    }

    // Replace "smain" with "spdf" in the destination path
    char modified_dest_path[1024];
    snprintf(modified_dest_path, sizeof(modified_dest_path), "%s", dest_path);
    char *replace = strstr(modified_dest_path, "smain");
    if (replace) {
        memmove(replace + 4, replace + 5, strlen(replace + 5) + 1);  // Move the string after "smain"
        memcpy(replace, "spdf", 4);  // Replace "smain" with "spdf"
    }

    // Send the filename and modified destination path to the Spdf server
    snprintf(message, sizeof(message), "%s;%s", filename, modified_dest_path);
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to send filename and destination path to PDF server");
        close(sock);
        return;
    }

    // Send the file size to the Spdf server
    if (send(sock, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to send file size to PDF server");
        close(sock);
        return;
    }

    // Send the file content directly from the Smain client connection to the Spdf server
    char buffer[1024];
    size_t bytes_sent = 0;
    ssize_t bytes_read;

    while (bytes_sent < file_size) {
        bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            printf("Error reading file content from client");
            break;
        }
        if (send(sock, buffer, bytes_read, 0) < 0) {
            printf("Failed to send file content to PDF server");
            break;
        }
        bytes_sent += bytes_read;
    }

    if (bytes_sent == file_size) {
        printf("File content sent to PDF server.\n");
    } else {
        printf("Failed to send complete file to PDF server.\n");
    }

    close(sock);
}

void send_to_txt_server(int client_socket, const char *filename, const char *dest_path, const char *stxt_ip, size_t file_size) {
    struct sockaddr_in txt_server_address;
    int sock = 0;
    char message[2048];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for PDF server\n");
        return;
    }

    txt_server_address.sin_family = AF_INET;
    txt_server_address.sin_port = htons(TEXT_SERVER_PORT);

    if (inet_pton(AF_INET, stxt_ip, &txt_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for TXT server\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&txt_server_address, sizeof(txt_server_address)) < 0) {
        printf("Connection failed to TXT server\n");
        close(sock);
        return;
    }

    // Replace "smain" with "stxt" in the destination path
    char modified_dest_path[1024];
    snprintf(modified_dest_path, sizeof(modified_dest_path), "%s", dest_path);
    char *replace = strstr(modified_dest_path, "smain");
    if (replace) {
         memcpy(replace, "stext", 5);
    }

    // Send the filename and modified destination path to the Spdf server
    snprintf(message, sizeof(message), "%s;%s", filename, modified_dest_path);
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to send filename and destination path to TXT server");
        close(sock);
        return;
    }

    // Send the file size to the Stxt server
    if (send(sock, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to send file size to TXT server");
        close(sock);
        return;
    }

    // Send the file content directly from the Smain client connection to the Stxt server
    char buffer[1024];
    size_t bytes_sent = 0;
    ssize_t bytes_read;

    while (bytes_sent < file_size) {
        bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            printf("Error reading file content from client");
            break;
        }
        if (send(sock, buffer, bytes_read, 0) < 0) {
            printf("Failed to send file content to TXT server");
            break;
        }
        bytes_sent += bytes_read;
    }

    if (bytes_sent == file_size) {
        printf("File content sent to TXT server.\n");
    } else {
        printf("Failed to send complete file to TXT server.\n");
    }

    close(sock);
}

// have created funtion created to  request pdf from pdf server 
void request_pdf_from_spdf(int client_socket, const char *filename, const char *spdf_ip) {
    struct sockaddr_in pdf_server_address;  // structure created to hold pdf address 
    int sock = 0;  //variable to store socket descriptor 
    char message[2048]; 

    // havew created socket for communication with the PDF server
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for PDF server\n");
        return;  
    }

    // have set the server address family to Internet, and the port number to the PDF server's port
    pdf_server_address.sin_family = AF_INET;
    pdf_server_address.sin_port = htons(PDF_SERVER_PORT);

    // Converting  the server IP address from text to binary form and store it in the address structure
    if (inet_pton(AF_INET, spdf_ip, &pdf_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for PDF server\n");
        close(sock);  // If the IP address is invalid, print an error, close the socket, and exit the function
        return;
    }

    // have attempt  to connect to the PDF server using the specified address and port
    if (connect(sock, (struct sockaddr *)&pdf_server_address, sizeof(pdf_server_address)) < 0) {
        printf("Connection failed to PDF server\n");
        close(sock);  // If connection fails, print an error, close the socket, and exit the function
        return;
    }

    // Modifying the destination path from "smain" to "spdf"
    char modified_dest_path[1024];
    snprintf(modified_dest_path, sizeof(modified_dest_path), "%s", filename); 
    char *replace = strstr(modified_dest_path, "smain");  
    if (replace) {
        memmove(replace + 4, replace + 5, strlen(replace + 5) + 1);  // Move the part after "smain" to adjust the string length
        memcpy(replace, "spdf", 4);  // Replace "smain" with "spdf" in the path
    }

    // Preparing  the request message to send to the PDF server
    snprintf(message, sizeof(message), "REQUEST_FILE;%s", modified_dest_path);
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to request file from PDF server");
        close(sock);  
        return;
    }

    // variable Receive the file size from the PDF server
    size_t file_size;
    if (read(sock, &file_size, sizeof(size_t)) <= 0) {
        printf("Failed to receive file size from PDF server");
        close(sock);  // If receiving the file size fails, print an error, close the socket, and exit the function
        return;
    }
    printf("Expected file size from Spdf: %zu bytes\n", file_size);

    if (send(client_socket, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to send file size to client");
        close(sock);  // If sending the file size to the client fails, print an error, close the socket, and exit the function
        return;
    }

    char buffer[1024];  
    size_t bytes_received = 0;  
    ssize_t bytes_read;  

    // Loop to receive the file in chunks until the entire file is received
    while (bytes_received < file_size) {
        bytes_read = read(sock, buffer, sizeof(buffer));  // Read a chunk of data from the PDF server
        if (bytes_read <= 0) {
            printf("Error receiving file content from PDF server");
            break; 
        }
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            printf("Failed to send file content to client");
            break;  
        }
        bytes_received += bytes_read; 
    }

    // Check if the entire file was successfully received and sent to the client
    if (bytes_received == file_size) {
        printf("File content received from PDF server and sent to client.\n");
    } else {
        printf("Failed to receive complete file from PDF server.\n");
    }

    close(sock);  
}

// have created function to request txt from txt server 
void request_txt_from_stxt(int client_socket, const char *filename, const char *stxt_ip) {
    struct sockaddr_in txt_server_address;  // created Structure to hold the TXT server address information
    int sock = 0;  // created Variable to hold the socket descriptor
    char message[2048];  

    // Created  a socket for communication with the TXT server
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for TXT server\n");
        return;  // If socket creation fails, print an error and exit the function
    }

    txt_server_address.sin_family = AF_INET;
    txt_server_address.sin_port = htons(TEXT_SERVER_PORT);

    // it is creating the server IP address from text to binary form and store it in the address structure
    if (inet_pton(AF_INET, stxt_ip, &txt_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for TXT server\n");
        close(sock);  // If the IP address is invalid, print an error, close the socket, and exit the function
        return;
    }

    // Attempting  to connect to the TXT server using the specified address and port
    if (connect(sock, (struct sockaddr *)&txt_server_address, sizeof(txt_server_address)) < 0) {
        printf("Connection failed to TXT server\n");
        close(sock);  // If connection fails, print an error, close the socket, and exit the function
        return;
    }

    // Modifying  the destination path from "smain" to "stxt"
    char modified_dest_path[1024];
    snprintf(modified_dest_path, sizeof(modified_dest_path), "%s", filename);  // Copy the filename into modified_dest_path
    char *replace = strstr(modified_dest_path, "smain");  // Find the substring "smain" in the path
    if (replace) {
        memcpy(replace, "stext", 5);  // Replace "smain" with "stxt" in the path
    }

    // Prepareing  the request message to send to the TXT server
    snprintf(message, sizeof(message), "REQUEST_FILE;%s", modified_dest_path);
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to request file from TXT server");
        close(sock);  // If sending the request fails, print an error, close the socket, and exit the function
        return;
    }

    // Receiveing  the file size from the TXT server
    size_t file_size;
    if (read(sock, &file_size, sizeof(size_t)) <= 0) {
        printf("Failed to receive file size from TXT server");
        close(sock);  // If receiving the file size fails, print an error, close the socket, and exit the function
        return;
    }
    printf("Expected file size from Stxt: %zu bytes\n", file_size);

    // Send the received file size to the client
    if (send(client_socket, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to send file size to client");
        close(sock);  // If sending the file size to the client fails, print an error, close the socket, and exit the function
        return;
    }

    // Receive the file content from the TXT server and forward it to the client
    char buffer[1024];  
    size_t bytes_received = 0;  
    ssize_t bytes_read;

    // Loop to receive the file in chunks until the entire file is received
    while (bytes_received < file_size) {
        bytes_read = read(sock, buffer, sizeof(buffer));  // Read a chunk of data from the TXT server
        if (bytes_read <= 0) {
            printf("Error receiving file content from TXT server");
            break;  // If reading fails, print an error and break out of the loop
        }
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            printf("Failed to send file content to client");
            break;  // If sending the data to the client fails, print an error and break out of the loop
        }
        bytes_received += bytes_read;  // Update the total number of bytes received
    }

    // Checking if the entire file was successfully received and sent to the client
    if (bytes_received == file_size) {
        printf("File content received from TXT server and sent to client.\n");
    } else {
        printf("Failed to receive complete file from TXT server.\n");
    }

    close(sock);  // Close the socket after the operation is complete
}

// have created function to delete pdf file request from pdf server 
void request_pdf_deletion_from_spdf(int client_socket, const char *filepath, const char *spdf_ip) {
    struct sockaddr_in pdf_server_address;  
    int sock = 0;  
    char message[2048];  

    // Creating  a socket for communication with the PDF server
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for PDF server\n");
        return;  // If socket creation fails, print an error and exit the function
    }

    // Seting  the server address family to Internet, and the port number to the PDF server's port
    pdf_server_address.sin_family = AF_INET;
    pdf_server_address.sin_port = htons(PDF_SERVER_PORT);

    // Converting  the server IP address from text to binary form and store it in the address structure
    if (inet_pton(AF_INET, spdf_ip, &pdf_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for PDF server\n");
        close(sock);  // If the IP address is invalid, print an error, close the socket, and exit the function
        return;
    }

    // Attempting to connect to the PDF server using the specified address and port
    if (connect(sock, (struct sockaddr *)&pdf_server_address, sizeof(pdf_server_address)) < 0) {
        printf("Connection failed to PDF server\n");
        close(sock);  // If connection fails, print an error, close the socket, and exit the function
        return;
    }

    // replacing smain path to spdf
    char modified_filepath[1024];
    snprintf(modified_filepath, sizeof(modified_filepath), "%s", filepath);  
    char *replace = strstr(modified_filepath, "smain");  
    if (replace) {
        memmove(replace + 4, replace + 5, strlen(replace + 5) + 1); 
        memcpy(replace, "spdf", 4);  
    }

    // Prepare the deletion request message to send to the PDF server
    snprintf(message, sizeof(message), "DELETE_FILE;%s", modified_filepath);
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to request file deletion from PDF server");
        close(sock);  // If sending the deletion request fails, print an error, close the socket, and exit the function
        return;
    }

    char buffer[1024];  // Buffer to hold the server's response
    // Wait for the response from the Spdf server
    ssize_t valread = read(sock, buffer, sizeof(buffer));  // Read the response from the server
    if (valread > 0) {
        buffer[valread] = '\0';  // Null-terminate the response string
        printf("Response from Spdf server: %s\n", buffer);
        send(client_socket, buffer, strlen(buffer), 0);  // Forward the server's response to the original client
    } else {
        printf("Failed to receive response from PDF server.\n");
        send(client_socket, "Failed to delete file.", strlen("Failed to delete file."), 0);  // Inform the client of the failure
    }

    close(sock);  // Close the socket after the operation is complete
}


// have created function to delete txt file request from pdf server 
void request_txt_deletion_from_stxt(int client_socket, const char *filepath, const char *stxt_ip) {
    struct sockaddr_in txt_server_address;
    int sock = 0;
    char message[2048];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for TXT server\n");
        return;
    }

    txt_server_address.sin_family = AF_INET;
    txt_server_address.sin_port = htons(TEXT_SERVER_PORT);

    if (inet_pton(AF_INET, stxt_ip, &txt_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for TXT server\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&txt_server_address, sizeof(txt_server_address)) < 0) {
        printf("Connection failed to TXT server\n");
        close(sock);
        return;
    }

    // Replace "smain" with "stxt" in the file path
    char modified_filepath[1024];
    snprintf(modified_filepath, sizeof(modified_filepath), "%s", filepath);
    char *replace = strstr(modified_filepath, "smain");
    if (replace) {
         memcpy(replace, "stext", 5);
    }

    // Send the deletion request for the txt file to the Stxt server
    snprintf(message, sizeof(message), "DELETE_FILE;%s", modified_filepath);

    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to request file deletion from TXT server");
        close(sock);
        return;
    }

    char buffer[1024];
    // Wait for the response from Spdf server
    ssize_t valread = read(sock, buffer, sizeof(buffer));
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Response from Stxt server: %s\n", buffer);
        send(client_socket, buffer, strlen(buffer), 0);  // Forward response to original client
    } else {
        printf("Failed to receive response from PDF server.\n");
        send(client_socket, "Failed to delete file.", strlen("Failed to delete file."), 0);
    }

    close(sock);
}

//have created function to created tar file
void create_tar(const char *tar_name, const char *filetype, const char *root_dir) {
    char command[512];
    snprintf(command, sizeof(command), "find %s -type f -name '*%s' -print0 | xargs -0 tar -cvf %s", root_dir, filetype, tar_name);
    int result = system(command);
    if (result == -1) {
        printf("Failed to create tar file");
    } else {
        printf("Tar file created: %s\n", tar_name);
    }
}

void receive_and_forward_tar_pdf(int source_sock, int dest_sock, const char *tar_name) {
    size_t file_size;
    if (read(source_sock, &file_size, sizeof(size_t)) <= 0) {
        printf("Failed to receive tar file size from PDF server");
        return;
    }

    if (send(dest_sock, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to forward tar file size to client");
        return;
    }

    char buffer[1024];
    size_t bytes_received = 0;
    ssize_t bytes_read;
    while (bytes_received < file_size) {
        bytes_read = read(source_sock, buffer, sizeof(buffer));
        if (bytes_read <= 0) break;
        if (send(dest_sock, buffer, bytes_read, 0) < 0) {
            printf("Failed to forward tar file content to client");
            return;
        }
        bytes_received += bytes_read;
    }
}

void request_pdf_tar_from_spdf(int client_socket, const char *spdf_ip) {
    struct sockaddr_in pdf_server_address;
    int sock = 0;
    char message[2048];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for PDF server\n");
        return;
    }

    pdf_server_address.sin_family = AF_INET;
    pdf_server_address.sin_port = htons(PDF_SERVER_PORT);

    if (inet_pton(AF_INET, spdf_ip, &pdf_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for PDF server\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&pdf_server_address, sizeof(pdf_server_address)) < 0) {
        printf("Connection failed to PDF server\n");
        close(sock);
        return;
    }

    // Send the request for the tar of .pdf files to Spdf server
    snprintf(message, sizeof(message), "REQUEST_PDF_TAR;");
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to request tar from PDF server");
        close(sock);
        return;
    }

    // Receive and forward the tar file from Spdf
    receive_and_forward_tar_pdf(sock, client_socket, "pdf.tar");

    close(sock);
}


//function receive and sending all txt to  tar  file
void receive_and_forward_tar_txt(int source_sock, int dest_sock, const char *tar_name) {
    size_t file_size;
    if (read(source_sock, &file_size, sizeof(size_t)) <= 0) {
        printf("Failed to receive tar file size from TXT server");
        return;
    }

    if (send(dest_sock, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to forward tar file size to client");
        return;
    }

    char buffer[1024];
    size_t bytes_received = 0;
    ssize_t bytes_read;
    while (bytes_received < file_size) {
        bytes_read = read(source_sock, buffer, sizeof(buffer));
        if (bytes_read <= 0) break;
        if (send(dest_sock, buffer, bytes_read, 0) < 0) {
            printf("Failed to forward tar file content to client");
            return;
        }
        bytes_received += bytes_read;
    }
}

// function to send the final tar file to server , compile from all txt files 
void request_txt_tar_from_stxt(int client_socket, const char *stxt_ip) {
    struct sockaddr_in txt_server_address;
    int sock = 0;
    char message[2048];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for TXT server\n");
        return;
    }

    txt_server_address.sin_family = AF_INET;
    txt_server_address.sin_port = htons(TEXT_SERVER_PORT);

    if (inet_pton(AF_INET, stxt_ip, &txt_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for TXT server\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&txt_server_address, sizeof(txt_server_address)) < 0) {
        printf("Connection failed to TXT server\n");
        close(sock);
        return;
    }

    // Send the request for the tar of .pdf files to Stxt server
    snprintf(message, sizeof(message), "REQUEST_TXT_TAR;");
    if (send(sock, message, strlen(message), 0) <= 0) {
        printf("Failed to request tar from TXT server");
        close(sock);
        return;
    }

    // Receiving and then forwarding  the tar file from Stxt
    receive_and_forward_tar_txt(sock, client_socket, "txt.tar");

    close(sock);
}

//function to send content to file 
void send_file_content(int sock, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        printf("Failed to open file for content transmission");
        return;
    }

    // Get the file size
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Send the file size to the client
    if (send(sock, &file_size, sizeof(size_t), 0) <= 0) {
        printf("Failed to send file size");
        fclose(f);
        return;
    }

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        if (send(sock, buffer, bytes_read, 0) < 0) {
            printf("Failed to send file content");
            fclose(f);
            return;
        }
    }

    fclose(f);
    printf("File content sent: %s\n", filename);
}


// function created to perform action to handle tar file creation 
void handle_dtar(int client_socket, const char *filetype) {
    if (strcmp(filetype, ".c") == 0) {
        // Creating a tar of all .c files in ~/smain
        create_tar("cfiles.tar", ".c", "~/smain");
        send_file_content(client_socket, "cfiles.tar");
        remove("cfiles.tar");
    } else if (strcmp(filetype, ".pdf") == 0) {
        // Requesting  the tar of .pdf files from the Spdf server
        request_pdf_tar_from_spdf(client_socket, "10.60.8.51");
    } 
    else if (strcmp(filetype, ".txt") == 0) {
        // Requesting the tar of .pdf files from the Spdf server
        request_txt_tar_from_stxt(client_socket, "10.60.8.51");
    }
    else {
        char response[] = "Unsupported file type.";
        send(client_socket, response, strlen(response), 0);
    }
}

//function receive and sending all pdf to  tar  file



// function to send the final tar file to server , compile from all pdf files 


// void list_files(int client_socket, const char *pathname, const char *filetype) {
//     char command[1024];
//     snprintf(command, sizeof(command), "find %s -type f -name '*%s'", pathname, filetype);

//     FILE *fp = popen(command, "r");
//     if (fp == NULL) {
//         printf("Failed to run find command");
//         return;
//     }

//     char buffer[1024];
//     while (fgets(buffer, sizeof(buffer), fp) != NULL) {
//         printf("Sending file: %s", buffer);  // Debug: print the file name being sent
//         if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
//             printf("Failed to send file name");
//         }
//     }

//     pclose(fp);

//     // Send end-of-list marker
//     const char *end_of_list = "END_OF_LIST";
//     if (send(client_socket, end_of_list, strlen(end_of_list), 0) == -1) {
//         printf("Failed to send end-of-list marker");
//     }
// }








// function create to get all the listed file inside pdf server 
void request_file_list_from_spdf(int client_socket, const char *pathname, const char *spdf_ip) {
    struct sockaddr_in pdf_server_address;
    int sock = 0;
    char message[2048];
    char buffer[1024];

    printf("Attempting to create socket for Spdf server...\n");
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for PDF server\n");
        return;
    }

    pdf_server_address.sin_family = AF_INET;
    pdf_server_address.sin_port = htons(PDF_SERVER_PORT);

    if (inet_pton(AF_INET, spdf_ip, &pdf_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for PDF server\n");
        close(sock);
        return;
    }

    printf("Attempting to connect to Spdf server at %s:%d...\n", spdf_ip, PDF_SERVER_PORT);
    if (connect(sock, (struct sockaddr *)&pdf_server_address, sizeof(pdf_server_address)) < 0) {
        printf("Connection failed to PDF server\n");
        close(sock);
        return;
    }
    printf("Connected to Spdf server successfully.\n");

    // Extract the base directory from the provided pathname
    const char *smain_base = "/smain";
    char translated_pathname[1024];
    
    const char *base_pos = strstr(pathname, smain_base);
    if (base_pos) {
        // Translate the pathname from Smain's path to Spdf's path
        snprintf(translated_pathname, sizeof(translated_pathname), "%.*s/spdf%s", (int)(base_pos - pathname), pathname, base_pos + strlen(smain_base));
    } else {
        printf("Error: Pathname does not contain '/smain'.\n");
        close(sock);
        return;
    }

    // Request the list of .pdf files from the Spdf server with the translated path
    snprintf(message, sizeof(message), "LIST_FILES;%s", translated_pathname);
    printf("Sending file list request to Spdf server: %s\n", message);
    send(sock, message, strlen(message), 0);

    // Receive the list of files from the Spdf server and forward them to the client
    ssize_t bytes_read;
    printf("Waiting to receive file list from Spdf server...\n");
    while ((bytes_read = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received from Spdf: %s", buffer);  // Print received data for debugging
        send(client_socket, buffer, bytes_read, 0);
        if (strstr(buffer, "END_OF_LIST") != NULL) {
            printf("End of file list received from Spdf server.\n");
            break;
        }
    }

    printf("Closing connection to Spdf server.\n");
    close(sock);
}

// function to compile entire file list 
void get_file_list(char *pathname, int client_socket) {
    printf("Running find command on: %s\n", pathname);  // Debug output

    char command[1024];
    snprintf(command, sizeof(command), "find %s -type f", pathname);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run find command");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Found file: %s", buffer);  // Debug print
        send(client_socket, buffer, strlen(buffer), 0);  // Send each file path to the client
    }

    pclose(fp);
    printf("Finished sending files from: %s\n", pathname);  // Debug output
}

// function create to get all the listed file inside txt server 
void request_file_list_from_stext(int client_socket, const char *pathname, const char *stext_ip) {
    struct sockaddr_in text_server_address;
    int sock = 0;
    char message[2048];
    char buffer[1024];

    printf("Attempting to create socket for Stext server...\n");
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error for Stext server\n");
        return;
    }

    text_server_address.sin_family = AF_INET;
    text_server_address.sin_port = htons(TEXT_SERVER_PORT);

    if (inet_pton(AF_INET, stext_ip, &text_server_address.sin_addr) <= 0) {
        printf("Invalid address/Address not supported for Stext server\n");
        close(sock);
        return;
    }

    printf("Attempting to connect to Stext server at %s:%d...\n", stext_ip, TEXT_SERVER_PORT);
    if (connect(sock, (struct sockaddr *)&text_server_address, sizeof(text_server_address)) < 0) {
        printf("Connection failed to Stext server\n");
        close(sock);
        return;
    }
    printf("Connected to Stext server successfully.\n");

    // Extract the base directory from the provided pathname
    const char *smain_base = "/smain";
    char translated_pathname[1024];
    
    const char *base_pos = strstr(pathname, smain_base);
    if (base_pos) {
        // Translate the pathname from Smain's path to Stext's path
        snprintf(translated_pathname, sizeof(translated_pathname), "%.*s/stext%s", (int)(base_pos - pathname), pathname, base_pos + strlen(smain_base));
    } else {
        printf("Error: Pathname does not contain '/smain'.\n");
        close(sock);
        return;
    }

    // Request the list of .txt files from the Stext server with the translated path
    snprintf(message, sizeof(message), "LIST_FILES;%s", translated_pathname);
    printf("Sending file list request to Stext server: %s\n", message);
    send(sock, message, strlen(message), 0);

    // Receive the list of files from the Stext server and forward them to the client
    ssize_t bytes_read;
    printf("Waiting to receive file list from Stext server...\n");
    while ((bytes_read = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received from Stext: %s", buffer);  // Print received data for debugging
        send(client_socket, buffer, bytes_read, 0);
        if (strstr(buffer, "END_OF_LIST") != NULL) {
            printf("End of file list received from Stext server.\n");
            break;
        }
    }

    printf("Closing connection to Stext server.\n");
    close(sock);
}


// function to list all  local file
void list_local_c_files(int client_socket, const char *pathname) {
    char command[1024];
    snprintf(command, sizeof(command), "find %s -type f -name '*.c'", pathname);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run find command");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        send(client_socket, buffer, strlen(buffer), 0);
    }

    pclose(fp);

      const char *end_of_list = "END_OF_LIST";

    send(client_socket, end_of_list, strlen(end_of_list), 0);
}


// create this function to handle all the client commands 
void prcclient(int new_socket) {
    char buffer[1024] = {0};
    char file_content[1024] = {0};
    size_t file_size = 0;

    while (1) {
        // Receive the command from the client
        ssize_t command_length = read(new_socket, buffer, sizeof(buffer) - 1);
        if (command_length <= 0) {
            printf("Client disconnected or error reading from socket.\n");
            break;
        }

        buffer[command_length] = '\0'; // Null-terminate the received command
        buffer[strcspn(buffer, "\n")] = 0; // Remove any newline character
        printf("Received command: '%s'\n", buffer);

        // If the command is empty, continue to the next iteration
        if (strlen(buffer) == 0) {
            printf("Empty command received, ignoring.\n");
            continue;
        }

        // Parse the command
        char *token = strtok(buffer, " ");
        if (token == NULL) {
            printf("Invalid command received.\n");
            continue;
        }

       if (strcmp(token, "display_file") == 0) {
    // Handle 'display_file' command
    char *pathname = strtok(NULL, " ");
    if (pathname == NULL) {
        char response[] = "Usage: display_file <pathname>";
        send(new_socket, response, strlen(response), 0);
        continue;
    }

    printf("Processing display_file command for pathname: %s\n", pathname);  // Debug print

    // 1. Obtain the list of files from Spdf server
    printf("Requesting file list from Spdf server...\n");
    request_file_list_from_spdf(new_socket, pathname, "10.60.8.51");


    printf("Requesting file list from Stext server...\n");
    request_file_list_from_stext(new_socket, pathname, "10.60.8.51");

    // 3. Get the list of files locally in Smain
    printf("Getting file list from Smain...\n");
    get_file_list(pathname, new_socket);

    // Send the end-of-list marker after sending all files
    const char *end_of_list = "END_OF_LIST";
    send(new_socket, end_of_list, strlen(end_of_list), 0);

    printf("Completed processing display_file command.\n");  // Debug print
} else if (strcmp(token, "ufile") == 0) {
            // Handle 'ufile' command
            char *filename = strtok(NULL, " ");
            char *destination_path = strtok(NULL, " ");

            if (filename == NULL || destination_path == NULL) {
                char response[] = "Usage: ufile <filename> <destination_path>";
                send(new_socket, response, strlen(response), 0);
                continue;
            }

            // Receive the file size
            if (read(new_socket, &file_size, sizeof(size_t)) <= 0) {
                printf("Failed to receive file size");
                continue;
            }
            printf("Expected file size: %zu bytes\n", file_size);

            // Determine the file type and process accordingly
            if (strstr(filename, ".pdf") != NULL) {
                // Forward to Spdf server
                send_to_pdf_server(new_socket, filename, destination_path, "10.60.8.51", file_size); // Use the correct IP for Spdf
                char response[] = ".pdf file received and saved.";
                send(new_socket, response, strlen(response), 0);
            } else if (strstr(filename, ".txt") != NULL) {
                send_to_txt_server(new_socket, filename, destination_path, "10.60.8.51", file_size); // Use the correct IP for Spdf
                char response[] = ".txt file received and saved.";
                send(new_socket, response, strlen(response), 0);
                // Handle .txt file similarly with Stext server (implement this as needed)
            } else if (strstr(filename, ".c") != NULL) {
                // Store locally in Smain
                struct stat st = {0};

                // Create the directory path recursively if it doesn't exist
                char temp_path[1024];
                snprintf(temp_path, sizeof(temp_path), "%s", destination_path);

                for (char *p = temp_path + 1; *p; p++) {
                    if (*p == '/') {
                        *p = '\0';
                        if (stat(temp_path, &st) == -1) {
                            if (mkdir(temp_path, 0700) != 0) {
                                printf("Failed to create intermediate directory");
                                char response[] = "Failed to create destination directory.";
                                send(new_socket, response, strlen(response), 0);
                                return;  // Exit the function on failure to avoid further errors
                            }
                        }
                        *p = '/';
                    }
                }

                // Finally, create the destination path directory
                if (stat(destination_path, &st) == -1) {
                    if (mkdir(destination_path, 0700) != 0) {
                        printf("Failed to create destination directory");
                        char response[] = "Failed to create destination directory.";
                        send(new_socket, response, strlen(response), 0);
                        return;  // Exit the function on failure to avoid further errors
                    }
                }

                char server_file_path[1024];
                snprintf(server_file_path, sizeof(server_file_path), "%s/%s", destination_path, filename);

                FILE *fp = fopen(server_file_path, "wb");
                if (!fp) {
                    printf("Failed to open file for writing");
                    char response[] = "Failed to store file on Smain.";
                    send(new_socket, response, strlen(response), 0);
                    return;  // Exit the function on failure to avoid further errors
                }

                size_t bytes_received = 0;
                ssize_t bytes_read;

                // Receive and write the file content
                while (bytes_received < file_size) {
                    bytes_read = read(new_socket, file_content, sizeof(file_content));
                    if (bytes_read <= 0) {
                        printf("Error receiving file content");
                        break;
                    }
                    fwrite(file_content, 1, bytes_read, fp);
                    fflush(fp); // Ensure the content is written immediately
                    bytes_received += bytes_read;
                }

                fclose(fp);

                if (bytes_received == file_size) {
                    printf("C file saved to: %s\n", server_file_path);
                    char response[] = "C file received and saved on Smain.";
                    send(new_socket, response, strlen(response), 0);
                } else {
                    printf("Failed to receive complete file.\n");
                    char response[] = "Failed to receive complete file.";
                    send(new_socket, response, strlen(response), 0);
                }

                // Reset file size for next command
                file_size = 0;
            } else {
                char response[] = "Unsupported file type.";
                send(new_socket, response, strlen(response), 0);
            }

        } else if (strcmp(token, "dfile") == 0) {
            // Handle 'dfile' command
            char *filepath = strtok(NULL, " ");
            if (filepath == NULL) {
                char response[] = "Usage: dfile <filepath>";
                send(new_socket, response, strlen(response), 0);
                continue;
            }

            if (strstr(filepath, ".pdf") != NULL) {
                // Request the file from Spdf server and send it to the client
                request_pdf_from_spdf(new_socket, filepath, "10.60.8.51"); // Use the correct IP for Spdf
            } else if (strstr(filepath, ".txt") != NULL) {
                // Handle .txt file similarly with Stext server 
                 request_txt_from_stxt(new_socket, filepath, "10.60.8.51"); // Use the correct IP for Spdf
            } else if (strstr(filepath, ".c") != NULL) {
                // Serve the .c file from the local Smain server
                int file_fd = open(filepath, O_RDONLY);
                if (file_fd < 0) {
                    printf("Failed to open requested file");
                    char response[] = "ERROR: Failed to open requested file";
                    send(new_socket, response, strlen(response), 0);
                    close(new_socket); // Close the connection after sending the error
                    return; // Exit the current loop to stop further processing
                }

                // Send the file size first
                struct stat st;
                if (fstat(file_fd, &st) != 0) {
                    printf("Failed to get file size");
                    close(file_fd);
                    continue;
                }
                size_t file_size = st.st_size;
                send(new_socket, &file_size, sizeof(size_t), 0);

                // Send the file content
                char buffer[1024];
                ssize_t bytes_read, bytes_sent = 0;
                while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                    if (send(new_socket, buffer, bytes_read, 0) < 0) {
                        printf("Failed to send file content to client");
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
            } else {
                char response[] = "Unsupported file type.";
                send(new_socket, response, strlen(response), 0);
            }

        } else if (strcmp(token, "rmfile") == 0) {
            // Handle 'rmfile' command
            char *filepath = strtok(NULL, " ");
            if (filepath == NULL) {
                char response[] = "Usage: rmfile <filepath>";
                send(new_socket, response, strlen(response), 0);
                continue;
            }

            if (strstr(filepath, ".pdf") != NULL) {
                // Send a request to Spdf server to delete the PDF file
                request_pdf_deletion_from_spdf(new_socket,filepath, "10.60.8.51");
               
            } 
            else if (strstr(filepath, ".txt") != NULL) {
                // Send a request to Spdf server to delete the PDF file
                request_txt_deletion_from_stxt(new_socket,filepath, "10.60.8.51");

            } 
            else if (strstr(filepath, ".c") != NULL) {
                // Delete the file locally in Smain
                if (remove(filepath) == 0) {
                    printf("File deleted: %s\n", filepath);
                    char response[] = "File deleted successfully.";
                    send(new_socket, response, strlen(response), 0);
                } else {
                    printf("Failed to delete file");
                    char response[] = "ERROR: Failed to delete file.";
                    send(new_socket, response, strlen(response), 0);
                }
            } else {
                char response[] = "Unsupported file type.";
                send(new_socket, response, strlen(response), 0);
            }

        } else if (strcmp(token, "dtar") == 0) {
            // Handle 'dtar' command
            char *filetype = strtok(NULL, " ");
            if (filetype == NULL) {
                char response[] = "Usage: dtar <filetype>";
                send(new_socket, response, strlen(response), 0);
                continue;
            }
            handle_dtar(new_socket, filetype);
        } else {
            char response[] = "Invalid command.";
            send(new_socket, response, strlen(response), 0);
        }
    }

    close(new_socket);
    exit(0); // Terminate the child process when done
}



int main(int argc, char const *argv[]) {
    int server_fd, new_socket;  
    struct sockaddr_in address;  
    int opt = 1;  
    int addrlen = sizeof(address);  

    const char *server_ip;  
    int port;  

    if (argc > 1) {
        server_ip = argv[1];
    } else {
        server_ip = "10.60.8.51";  // picking Default IP address
    }

    // Check if the port number is provided as a command-line argument, otherwise use a default port
    if (argc > 2) {
        port = atoi(argv[2]);  // Converting  the provided port number from string to integer
    } else {
        port = DEFAULT_PORT; 
    }

    // have created  a socket for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("Socket creation error");
        exit(EXIT_FAILURE);  // Exit if socket creation fails
    }

    // have Set the  socket options to allow address and port reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("Setsockopt error");
        exit(EXIT_FAILURE);  // Exit if setting socket options fails
    }

    // have Set the server address family to Internet, and assign the IP address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(port);

    // perform Binding to bind the server to the specified IP address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Bind error");
        exit(EXIT_FAILURE);  // Exit if binding fails
    }

    // listening for incoming ping with a backlog of 3
    if (listen(server_fd, 3) < 0) {
        printf("Listen error");
        exit(EXIT_FAILURE);  // Exit if listening fails
    }

    // Printing the server IP and port, indicating that the server is running
    printf("Server running on %s:%d\n", server_ip, port);

    // Continuously accept incoming connections
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        pid_t pid = fork();  //forking the process to handle the process that in incoming 
        if (pid < 0) {
            printf("Fork error");
            exit(EXIT_FAILURE);  
        }
        if (pid == 0) {
            // Child process
            close(server_fd);  
            prcclient(new_socket);  
            exit(0);  
        } else {
            // Parent process
            close(new_socket);  
        }

        // we are Optionally, clean up zombie processes by non-blocking wait for any child processes that have finished
        waitpid(-1, NULL, WNOHANG);
    }

    // Checking for errors in accepting connections
    if (new_socket < 0) {
        printf("Accept error");
        exit(EXIT_FAILURE);  
    }

    return 0;  
}
