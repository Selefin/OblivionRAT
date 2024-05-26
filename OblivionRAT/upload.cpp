#include "upload.h"

void handle_upload(SOCKET sock, char* bufferin) {
    char filename[256];
    int received;
    FILE* fp;

    // Clear the buffer and receive the filename
    ZeroMemory(bufferin, sizeof(bufferin));
    if (recv(sock, filename, sizeof(filename), 0) <= 0) {
        send(sock, "Error receiving filename\n", strlen("Error receiving filename\n"), 0);
        return;
    }

    // Remove newline character if present
    char* newline = strchr(filename, '\n');
    if (newline) {
        *newline = '\0';
    }
    send(sock, "File created\n", strlen("File created\n"), 0);

    // Open the file for writing
    fp = fopen(filename, "w");
    if (fp == NULL) {
        send(sock, "Error opening file for writing\n", strlen("Error opening file for writing\n"), 0);
        return;
    }

    // Receive the file content
    char stop[] = "stop upload";
    while ((received = recv(sock, bufferin, sizeof(bufferin), 0)) > 0) {
        if (strstr(bufferin, "stop upload") != NULL) {
            break;
        }
        fwrite(bufferin, 1, received, fp);
    }
    fclose(fp);

    send(sock, "File received successfully\n", strlen("File received successfully\n"), 0);
}