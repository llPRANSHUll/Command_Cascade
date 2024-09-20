#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>


#define DEFAULT_PORT 4259 // Have define the  default port  4259 
#define DEFAULT_IP "10.60.8.51"  // have define default Ip 10.60.8.51

// have created function to receive file from server 
void FileReceive(int sock, const char *filename) {
    char data_buffer[1024];
    size_t receivedBytes = 0;
    ssize_t readBytes;
    size_t SizeOfFile;

    // function to check the file check
    if (read(sock, &SizeOfFile, sizeof(size_t)) <= 0) {
        printf("Failed to receive file size");
        return;
    }
    printf("Received file size: %zu bytes\n", SizeOfFile);

    // to store data , created temp file 
    FILE *f = fopen("temp_received_file", "wb");
    if (f == NULL) {
        printf("Failed to open temporary file for writing");
        return;
    }

    printf("Receiving file: %s\n", filename);

    // creted loop to receive the chuck of file conetent , utill complete content is received  
    while (receivedBytes < SizeOfFile) {
        readBytes = read(sock, data_buffer, sizeof(data_buffer));
        if (readBytes <= 0) {
            printf("Error receiving file content");
            break;
        }
        fwrite(data_buffer, 1, readBytes, f);  // Write the received data to the temporary file
        fflush(f); // flushing content to disk
        receivedBytes += readBytes;
    }

    fclose(f);

    // check that file receive successfully 
    if (receivedBytes == SizeOfFile) {
        printf("File content received successfully. Moving to final location...\n");

        // move the file to desire location using system command 
        char command[1024];
        snprintf(command, sizeof(command), "mv temp_received_file %s", filename);

        int result = system(command);
        if (result == 0) {
            printf("File successfully moved to: %s\n", filename);
        } else {
            printf("Failed to move file to: %s\n", filename);
            printf("System move command failed");
        }
    } else {
        printf("Failed to receive complete file.\n");
        // Clean up the incomplete file
        remove("temp_received_file");
    }
}

// Function to send the size of a file to the server
void sendFileSize(int sock, const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        printf("Failed to get file size");
        return;
    }

    size_t SizeOfFile = st.st_size;

    // Send the file size to the server
    send(sock, &SizeOfFile, sizeof(size_t), 0);
}

// Function to send the content of a file to the server
void sendFileContent(int sock, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        printf("Failed to open file for content transmission");
        return;
    }

    char data_buffer[1024];
    size_t readBytes;

    // Read and send the file content in chunks
    while ((readBytes = fread(data_buffer, 1, sizeof(data_buffer), f)) > 0) {
        if (send(sock, data_buffer, readBytes, 0) < 0) {
            printf("Failed to send file content");
            fclose(f);
            return;
        }
    }

    fclose(f);
    printf("File content sent.\n");
}

// Function to receive and categorize a list of files from the server
void FileReceiveList(int sock) {
    char data_buffer[1024];
    char *Cfiles[100];    // Created Array to store .c filenames
    char *pdfFiles[100];  //  Created Array to store .pdf filenames
    char *txtFiles[100];  // Created Array to store .txt filenames
    int Ccount = 0, pdfCount = 0, TXTcount = 0;
    int END_OF_LIST_COUNT = 0;  // keeping track of END_OF_LIST_COUNT , counter is created 

    while (1) {
        memset(data_buffer, 0, sizeof(data_buffer));  // use to clear the data_buffer
        int valread = read(sock, data_buffer, sizeof(data_buffer) - 1);
        if (valread <= 0) {
            break;  // Exit loop if there is an error or no more data
        }

        data_buffer[valread] = '\0';  // Null-terminate the data_buffer

        // Check for end-of-list marker
        if (strstr(data_buffer, "END_OF_LIST") != NULL) {
            END_OF_LIST_COUNT++;
            if (END_OF_LIST_COUNT >= 3) {  // Assuming one END_OF_LIST per server response
                break;  // Break out of the loop after the third END_OF_LIST
            }
            continue;  // Skiing processing for END_OF_LIST and continue to read more data
        }

        // Process each line in the data_buffer
        char *line = strtok(data_buffer, "\n");
        while (line != NULL) {
            // Extract the filename from the full path
            char *filename = strrchr(line, '/');
            if (filename) {
                filename++;  // Move past the '/' character to the actual filename
            } else {
                filename = line;  // If no '/' found, treat the entire line as the filename
            }

            // Store filenames in respective arrays based on file extension
            if (strstr(filename, ".c") != NULL) {
                Cfiles[Ccount] = strdup(filename);  // Store .c files
                Ccount++;
            } else if (strstr(filename, ".pdf") != NULL) {
                pdfFiles[pdfCount] = strdup(filename);  // Store .pdf files
                pdfCount++;
            } else if (strstr(filename, ".txt") != NULL) {
                txtFiles[TXTcount] = strdup(filename);  // Store .txt files
                TXTcount++;
            }

            line = strtok(NULL, "\n");
        }
    }

    // printing all .c files first
    for (int i = 0; i < Ccount; i++) {
        printf("%s\n", Cfiles[i]);
        free(Cfiles[i]);  // Free the allocated memory
    }

    // printing all .pdf files after .c files
    for (int i = 0; i < pdfCount; i++) {
        printf("%s\n", pdfFiles[i]);
        free(pdfFiles[i]);  // Free the allocated memory
    }

    // printing all .txt files after .pdf files
    for (int i = 0; i < TXTcount; i++) {
        printf("%s\n", txtFiles[i]);
        free(txtFiles[i]);  // Free the allocated memory
    }

    printf("File listing completed.\n");  // Final message after the loop ends
}

// created then fucntion to verify and send to signal 
void verify_and_send_command(int sock, char *command) {
    char data_buffer[1024] = {0};

    // Parse the command
    char *token = strtok(command, " ");
    if (token == NULL) {
        printf("Invalid command. Please try again.\n");
        return;
    }

    // Check if the command is 'ufile' (upload file)
    if (strcmp(token, "ufile") == 0) {
        // Handle 'ufile' command
        char *filename = strtok(NULL, " ");
        char *destination_path = strtok(NULL, " ");

        if (filename == NULL || destination_path == NULL) {
            printf("Usage: ufile <filename> <destination_path>\n");
            return;
        }

        // Sending the command to the server
        snprintf(data_buffer, sizeof(data_buffer), "ufile %s %s", filename, destination_path);
        send(sock, data_buffer, strlen(data_buffer), 0);

        // Sending the file size first
        sendFileSize(sock, filename);

        // Sending the file content
        sendFileContent(sock, filename);

        // Waiting for server response
        int valread = read(sock, data_buffer, sizeof(data_buffer));
        if (valread > 0) {
            data_buffer[valread] = '\0';
            printf("Server response: %s\n", data_buffer);
        }

    } else if (strcmp(token, "dfile") == 0) {
        char *filepath = strtok(NULL, " ");

        if (filepath == NULL) {
            printf("Usage: dfile <filepath>\n");
            return;
        }

        // Sending the command to the server
        snprintf(data_buffer, sizeof(data_buffer), "dfile %s", filepath);
        send(sock, data_buffer, strlen(data_buffer), 0);

        // Extracting the filename from the filepath
        char *filename = strrchr(filepath, '/');
        if (filename) {
            filename++; // Skip the '/'
        } else {
            filename = filepath; // No '/' found, use the full path as filename
        }

        // Receive the file from the server
        FileReceive(sock, filename);

        // After receiving the file, don't wait for any more responses
        printf("File received successfully.\n");

    } else if (strcmp(token, "rmfile") == 0) {
        // Handle 'rmfile' command (remove file)
        char *filepath = strtok(NULL, " ");

        if (filepath == NULL) {
            printf("Usage: rmfile <filepath>\n");
            return;
        }

        // Send the command to the server
        snprintf(data_buffer, sizeof(data_buffer), "rmfile %s", filepath);
        send(sock, data_buffer, strlen(data_buffer), 0);

        // Wait for server response
        int valread = read(sock, data_buffer, sizeof(data_buffer));
        if (valread > 0) {
            data_buffer[valread] = '\0';
            printf("Server response: %s\n", data_buffer);
        }

    } else if (strcmp(token, "dtar") == 0) {
        // Handle 'dtar' command (download tar file of specific file type)
        char *filetype = strtok(NULL, " ");
        if (filetype == NULL) {
            printf("Usage: dtar <filetype>\n");
            return;
        }

        // Send the command to the server
        snprintf(data_buffer, sizeof(data_buffer), "dtar %s", filetype);
        send(sock, data_buffer, strlen(data_buffer), 0);

        // Receive the tar file from the server
        char tar_filename[256];
        snprintf(tar_filename, sizeof(tar_filename), "%sfiles.tar", filetype + 1); // skip the dot
        FileReceive(sock, tar_filename);

        printf("Tar file %s received successfully.\n", tar_filename);
        
    } else if (strcmp(token, "display_file") == 0) {
        // Handle 'display_file' command (display list of files in directory)
        char *pathname = strtok(NULL, " ");

        if (pathname == NULL) {
            printf("Usage: display_file <pathname>\n");
            return;
        }

        // Send the command to the server
        snprintf(data_buffer, sizeof(data_buffer), "display_file %s", pathname);
        send(sock, data_buffer, strlen(data_buffer), 0);

        // Receive and display the list of files from the server
        FileReceiveList(sock);
    } else {
        printf("Unknown command: %s\n", token);
    }
}

// Function to handle communication with the server in a loop
void communicateServer(int sock) {
    char command[1024];

    while (1) {
        // Prompt for a command
        printf("Enter command: ");
        fgets(command, 1024, stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline character

        // Verify and send the command to the server
        verify_and_send_command(sock, command);

        // Ask if the user wants to continue or exit
        printf("Do you want to enter another command? (YES or NO): ");
        char user_response[10];
        fgets(user_response, sizeof(user_response), stdin);
        user_response[strcspn(user_response, "\n")] = 0; // Remove newline character

        if (strcasecmp(user_response, "NO") == 0) {
            // Notify the server that the client is disconnecting
            send(sock, "exit", strlen("exit"), 0);
            printf("Client disconnected.\n");
            break;
        }
    }
}

// Main function to set up the connection to the server and start communication
int main() {
    struct sockaddr_in serv_addr;
    int sock = 0;
    char server_ip[100];
    int port;

    // Get IP address from the user
    printf("Enter server IP address (or press enter to use default %s): ", DEFAULT_IP);
    fgets(server_ip, sizeof(server_ip), stdin);
    server_ip[strcspn(server_ip, "\n")] = 0; // Remove newline character

    // If the user presses enter without typing an IP address, use the default IP
    if (strlen(server_ip) == 0) {
        strcpy(server_ip, DEFAULT_IP);
    }

    // Get port number from the user
    printf("Enter server port (or press enter to use default %d): ", DEFAULT_PORT);
    char port_input[10];
    fgets(port_input, sizeof(port_input), stdin);

    if (strlen(port_input) > 1) {
        port = atoi(port_input); // Convert the input to an integer
    } else {
        port = DEFAULT_PORT; // Use default port if no input is given
    }

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Converting  the server IP address to binary form and store it in serv_addr
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Connecting  to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    // Now start the communication loop
    communicateServer(sock);

    close(sock);  // Close the socket when done
    return 0;
}
