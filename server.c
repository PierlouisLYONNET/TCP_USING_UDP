#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <time.h>

    
#define PORT_PUBLIC 8080 
#define PORT_DATA 6666
#define MAXLINE 1024 // TODO CHANGING IT DO NOT WORK
#define SIZE_HEADER 6
#define WINDOW_SIZE 10
    
int main() { 

    // TODO argc

    // Modèle d'en tête de nos packets :
    // 6 premiers caractères : NUM SEQ
    // 1018 autres cractères : DATA

    int server_socket,data_socket; 
    char buffer[MAXLINE]; 
    char *syn_ack = "SYN-ACK6666"; 
    char portserv[6];
    sprintf(portserv,"%i", PORT_DATA);
    char *port = portserv; 
    struct sockaddr_in servaddr, cliaddr,exchangeaddr;
    int len, n; 
    len = sizeof(cliaddr);  //len is value/result 


        
    /* --- Create the socket --- */

    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    if ((data_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr));


        
    /* --- Server information --- */

    inet_pton(AF_INET, "172.17.0.1", &(servaddr.sin_addr));    

    servaddr.sin_family = AF_INET; // IPv4 
    //servaddr.sin_addr.s_addr = atoi("178.20.10.5"); 
    servaddr.sin_port = htons(PORT_PUBLIC); 

    exchangeaddr.sin_family = AF_INET;
    exchangeaddr.sin_addr.s_addr = INADDR_ANY;
    exchangeaddr.sin_port = htons(PORT_DATA); 


     
    /* --- Bind the socket with the server address --- */

    if (bind(server_socket, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (bind(data_socket, (const struct sockaddr *)&exchangeaddr, sizeof(exchangeaddr)) < 0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 


    /* --- Timeout socket --- */

    struct timeval tv;
    tv.tv_sec  = 5;
    tv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    // setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);




    /* --- Threeway Handshake --- */

    // Waiting SYN client
    n = recvfrom(server_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0'; 

    // Send "SYN-ACK0000" to client
    sendto(server_socket, (const char *)syn_ack, 12, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
    printf("Send : syn_ack message.\n");  

    // Waiting ACK client
    n = recvfrom(server_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 
    printf("Received from client : %s\n", buffer);

    /*
    // Send Port
    sendto(server_socket, (const char *)port, strlen(port),  MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
    printf("Send : port adress.\n");  
    */

    /* --- End of Threeway handshake --- */




    /* --- Data exchange --- */

    /* --- Waiting for ftp --- */

    char buffer_file_name[200]; 

    n = recvfrom(data_socket, (char *)buffer_file_name, sizeof(buffer_file_name), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    buffer_file_name[n] = '\0'; 
    printf("Client : %s\n", buffer_file_name); 



    /* --- Check if file exist --- */

    FILE *file_ptr = fopen(buffer_file_name, "rb");

    if (file_ptr == NULL) {
        printf("File not found\n");
        exit(1);
    }


    /* --- Number of sequence --- */

    int num_seq = 1;
    int num_seq_last_ack = 0;
    char num_seq_char[SIZE_HEADER];

    char buffer_to_send[MAXLINE];
    char buffer_data_packet[(MAXLINE - SIZE_HEADER)];

    

    /* --- Buffer TODO BIG of the file --- */

    // Go to end of file to count number of bytes
    fseek(file_ptr, 0, SEEK_END);
    long size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);  // Go back to beginning of file

    // Allocate memory for entire content of file
    char *buffer_file_big = malloc(size + 1);
    fread(buffer_file_big, 1, size, file_ptr);

    // Add null terminator
    buffer_file_big[size] = 0;

    fclose(file_ptr);

    

    /* --- Sending file data socket --- */

    int end_transmission = 1;
    int current_index = 0;

    // Cut whole file in packet of 1018 bytes
    int max_index = size/(MAXLINE - SIZE_HEADER);
    printf("max index : %i", max_index);

    int count = 0;
    int lost_ACK = 0;

    int count_packet = 1; // in order to optimize the client1
    int next_dropped_packet = 4; // in order to optimize the client1
    int n_packet = 2;

    while(end_transmission) {
        
        // Empty buffer
        memset(buffer_to_send, 0, sizeof(buffer_to_send)); // TODO OPTIMIZE
        memset(num_seq_char, 0, sizeof(num_seq_char));

        // Gestion des doublons pour le client1
        if(count_packet == next_dropped_packet) { // in order to optimize the client1
            sprintf(num_seq_char, "%06d%d", (num_seq - 1)); // integer to string

            memcpy(buffer_to_send, num_seq_char, 6);

            printf("\nBuffer_to_send %s\n", buffer_to_send);
            
            printf("\n\n--------------Send-------------\n");
            sendto(data_socket, (char *)buffer_to_send, strlen(buffer_to_send), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 

            n_packet += 1;
            next_dropped_packet = 1 + 3 * (n_packet - 1); 
            
        } else {
            sprintf(num_seq_char, "%06d%d", num_seq); // integer to string

            memcpy(buffer_to_send, num_seq_char, 6);

            printf("Current index : %i\n", current_index);

            // If last index : send last N bytes remaining
            // Else : send 1018 bytes
            if(current_index == max_index){
                memcpy(buffer_to_send + 6, buffer_file_big + (current_index * (MAXLINE - SIZE_HEADER)), size - ((current_index) * (MAXLINE - SIZE_HEADER)));
            } else {
                memcpy(buffer_to_send + 6, buffer_file_big + (current_index * (MAXLINE - SIZE_HEADER)), MAXLINE - SIZE_HEADER);
            }
            

            printf("\nBuffer_to_send %s\n", buffer_to_send);
            
            printf("\n\n--------------Send-------------\n");
            sendto(data_socket, (char *)buffer_to_send, strlen(buffer_to_send), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 

            num_seq += 1;
            current_index += 1;


            // If last index : end transmission
            if(current_index > max_index) {
                end_transmission = 0;
            }
        }

        count_packet += 1;


        /* --- Waiting ACK_seq client --- */
        
        printf("\n\n----------Receive_ACK----------\n");

        n = recvfrom(data_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0'; 

        printf("Received from client : %s\n", buffer);

        // Remove "ACK" from buffer
        memmove(buffer, buffer + 3, strlen(buffer));

        int seq_received = atoi(buffer);

        if(seq_received < (num_seq - 1)) {
            printf("**********ACK lost\n");
            count += 1;
            if(count == 3) {
                printf("Lost ACK : %i\n", lost_ACK);
                num_seq = num_seq_last_ack;
                current_index = num_seq_last_ack - 1;
                count = 0;
            }
        } else {
            count = 0;
        }

        // printf("Received from client without ACK : %s\n", buffer);

        num_seq_last_ack = seq_received; // string to integer

        printf("num_seq_last_ack : %i\n", num_seq_last_ack);
        printf("num_seq : %i\n", num_seq);

        // TODO ACK lost

    }

    // TELL THE CLIENT TO STOP
    // buffer_to_send[0] = '\0';
    // buffer_data_packet[0] = '\0';
    // num_seq_char[0] = '\0';

    memset(buffer_to_send, 0, (MAXLINE - 1)); // TODO OPTIMIZE
    // memset(buffer_data_packet, 0, (MAXLINE - SIZE_HEADER - 1));
    // memset(num_seq_char, 0, SIZE_HEADER);

    strcat(buffer_to_send, "FIN");

    /* --- Sending "FIN" to client to end transmission --- */

    printf("\n\n--------------Send-------------n");
    sendto(data_socket, (char *)buffer_to_send, 3, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);

    //printf("buffer : \n%s\n\n\n", buffer_to_send);


    /* --- Closing the sockets --- */
    close(server_socket);
    close(data_socket);
    return 0; 
}
