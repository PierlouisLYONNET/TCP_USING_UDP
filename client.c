#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define MAXLINE 1024 // TODO CHANGING IT DO NOT WORK
#define SIZE_HEADER 6
#define WINDOW_SIZE 10

// Driver code
int main()
{

    // TODO argc

    int server_socket;
    char buffer[MAXLINE];
    char *syn = "SYN";
    char *ack = "ACK";
    struct sockaddr_in servaddr, addr;
    int n, len, portserv;



    /* --- Create the socket --- */

    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;



    /* --- Threeway Handshake --- */

    // Send SYN
    sendto(server_socket, (const char *)syn, strlen(syn), MSG_CONFIRM, (const struct sockaddr *)&addr, sizeof(addr));
    printf("Send : syn message.\n");

    // Waiting SYN ACK
    n = recvfrom(server_socket, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&addr, &len);
    buffer[n] = '\0';
    printf("Received from server : %s\n", buffer);

    // Send ACK
    sendto(server_socket, (const char *)ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr *)&addr, sizeof(addr));
    printf("ack message sent.\n");

    // Waiting Server port
    n = recvfrom(server_socket, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&addr, &len);
    buffer[n] = '\0';
    portserv = strtol(buffer, NULL, 10);
    printf("Received from server : %s\n", buffer);
    printf("Server port : %i\n", portserv);

    /* --- End Threeway Handshake --- */



    /* -- Create new socket port server --- */

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portserv);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int data_socket;
    if ((data_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }



    /* --- Send data on data_socket --- */

    char buffer_data[MAXLINE];
    char buffer_writer[MAXLINE];
    fgets(buffer_data, MAXLINE, stdin);
    printf("\n\n\n---------- Send : ----------\n%s\n", buffer_data);
    sendto(data_socket, (char *)buffer_data, strlen(buffer_data), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    char buffer_sequence[(SIZE_HEADER + 1)];
    char temp_buffer[MAXLINE];

    // Tableau de int qui récupère les num de séquence
    int sequence_packets_received[WINDOW_SIZE];
    // Permet de voir à quel numéro on est actuellement (les autres sont inutiles)
    int current_index = 0;

    int sequence_number_current = 0;

    FILE *file_ptr;
    file_ptr = fopen("received.txt", "wb");

    int end_transmission = 1;



    /* --- Timeout socket --- */

    struct timeval tv;
    tv.tv_sec  = 10;
    tv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


    /* --- Receive data on data_socket --- */

    while (end_transmission)
    {

        n = recvfrom(data_socket, (char *)buffer_data, MAXLINE, MSG_WAITALL, (struct sockaddr *)&addr, &len);
        buffer_data[n] = '\0';

        printf("\n-----Received from server-----\n\n\n");       
        memcpy(buffer_writer, buffer_data + SIZE_HEADER, n);
        
        printf("Size received : %i", n);
        

        /* --- Take the packets number --- */

        strcpy(temp_buffer, buffer_data);
        // Prevent the buffer to be taking all TODO better way
        buffer_sequence[SIZE_HEADER] = '\0';
        strncpy(buffer_sequence, temp_buffer, SIZE_HEADER);

        sequence_number_current = atoi(buffer_sequence);

        sequence_packets_received[current_index] = sequence_number_current;
        if (current_index == ((WINDOW_SIZE - 1)))
        {
            current_index = 0;
        }
        else
        {
            current_index += 1;
        }

        // DEBUG
        //printf("\nNum séquence: %s\n\n", buffer_sequence);
        // printf("\nNum séquence int: %i\n", sequence_number_current);
        // printf("\nCurrent Index: %i\n\n", current_index);
        // for(int j= 0; j< 10; j++) {
        //     printf("\nSequence packets j:  %i\n", sequence_packets_received[j]);
        // }



        /* --- Send ACK --- */

        char buffer_ack[(SIZE_HEADER + 4)];
        strcpy(buffer_ack, "ACK_");
        strcat(buffer_ack, buffer_sequence);

        printf("\nNum séquence: %s\n", buffer_ack);
        sendto(data_socket, (char *)buffer_ack, strlen(buffer_ack), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));



        /* --- Writing or End of transmission --- */

        if(sequence_number_current != 0) {
            /* Create the file or reopen it */
            // TODO create file transmited by server (.jpg, .txt)
            printf("J'écris\n\n");
            fwrite(buffer_writer, 1, (n - SIZE_HEADER), file_ptr);
        } else {
            printf("STOP\n\n");
            end_transmission = 0;
        }
        
    }


    /* --- Closing file and socket --- */

    fclose(file_ptr);
    printf("Is closed ?\n");

    close(server_socket);
    return 0;
}