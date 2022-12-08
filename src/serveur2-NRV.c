#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

    
#define PORT_DATA 6666
#define SIZE_PACKET 1500 // MAX 1500 => MTU
#define SIZE_HEADER 6
    
int main(int argc, char* argv[]) { 

    if(argc != 2) {
        printf("Usage : ./serveur2-NRV <PORT>\n");
        exit(1);
    }

    int PORT_PUBLIC = atoi(argv[1]);

    // Modèle d'en tête de nos packets :
    // 6 premiers caractères : NUM SEQ
    // SIZE_PACKET - SIZE_HEADER autres cractères : DATAA

    int server_socket, data_socket; 
    char buffer[SIZE_PACKET]; 
    char *syn_ack = "SYN-ACK6666"; 

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

    servaddr.sin_family = AF_INET; // IPv4  
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
    tv.tv_sec  = 1.000001;  // TODO depending RTT
    tv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


    /* --- Threeway Handshake --- */

    // Waiting SYN client
    printf("\n/* ------- Threeway Handshake ------ */\n\n");

    n = recvfrom(server_socket, (char *)buffer, SIZE_PACKET,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0'; 

    // Send "SYN-ACK0000" to client
    sendto(server_socket, (const char *)syn_ack, 12, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
    printf("Sent syn_ack message.\n");  

    // Waiting ACK client
    n = recvfrom(server_socket, (char *)buffer, SIZE_PACKET,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 
    printf("Received : %s\n", buffer);

    printf("\n/* --- End of Threeway Handshake --- */\n\n");

    /* --- End of Threeway handshake --- */



    /* ---------------------------------------- Data exchange ---------------------------------------- */

    /* --- Waiting for ftp --- */

    char buffer_file_name[200]; 

    n = recvfrom(data_socket, (char *)buffer_file_name, sizeof(buffer_file_name), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    buffer_file_name[n] = '\0'; 
    printf("Asking for file : %s\n", buffer_file_name); 



    /* --- Check if file exist --- */

    FILE *file_ptr = fopen(buffer_file_name, "rb");

    if (file_ptr == NULL) {
        printf("File not found\n");
        exit(1);
    } else {
        printf("File founded\n\n");
    }

    /* --- Window --- */

    int window_size = 20;
    int window_start = 0;
    int window_end = window_size; //TODO SLOW START CHANGE WINDOW SIZE

    /* --- Sequence number --- */

    int seq_num = 1;
    int higher_seq_num_received = 0;
    char num_seq_char[SIZE_HEADER];
    int seq_limit = window_size + window_size/2; // Limit of difference between seq_num and received seq
    int seq_received = -1;

    char buffer_to_send[SIZE_PACKET];
    char buffer_data_packet[(SIZE_PACKET - SIZE_HEADER)];


    /* --- Buffer TODO BIG of the file --- */

    // Go to end of file to count number of bytes
    fseek(file_ptr, 0L, SEEK_END);
    int size = ftell(file_ptr);
    rewind(file_ptr);

    // Allocate memory for entire content of file
    char *buffer_file_big = malloc(size + 1);
    fread(buffer_file_big, 1, size, file_ptr);

    // Add null terminator at the end of the buffer
    buffer_file_big[size] = 0;

    fclose(file_ptr);
    

    /* --- Sending file data socket --- */

    int end_transmission = 1;
    int current_index = 0;

    // Cut whole file in packet of 1018 bytes
    int max_index = size/(SIZE_PACKET - SIZE_HEADER);
    printf("File buffered - max index : %i\n", max_index);

    int count = 0;

    int count_packet = 1; // in order to optimize the client1
    int next_dropped_packet = 4; // in order to optimize the client1
    int n_packet = 2;

    printf("\n/* ----- Starting sending data ----- */\n\n");

    printf("Empty packet in the window : %i\n",window_end-window_start);

    while(end_transmission) {

        // On envoie le nombre de paquet que l'on veut puis on enchaîne avec le reste
        for(window_start; window_start < window_end; window_start++) {

            // Empty buffer
            memset(buffer_to_send, 0, sizeof(buffer_to_send));
            memset(num_seq_char, 0, sizeof(num_seq_char));
                
            sprintf(num_seq_char, "%06d%d", seq_num); // integer to string

            memcpy(buffer_to_send, num_seq_char, 6);

            // If last index : send last N bytes remaining
            // Else : send SIZE_PACKET bytes
            if(current_index == max_index) {

                memcpy(buffer_to_send + 6, buffer_file_big + (current_index * (SIZE_PACKET - SIZE_HEADER)), size - ((current_index) * (SIZE_PACKET - SIZE_HEADER)));

                //printf("\nBuffer_to_send %s\n", buffer_to_send);
            
                printf("\n-------------->> Send\n");
                printf("Num seq : %i\n", seq_num);
                sendto(data_socket, (char *)buffer_to_send, size - ((current_index) * (SIZE_PACKET - SIZE_HEADER)) + SIZE_HEADER, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 

                seq_num += 1;
                current_index += 1;

                count_packet += 1;

            } else {
                if(current_index < max_index) { // To prevent the for to send data not existing
                    memcpy(buffer_to_send + 6, buffer_file_big + (current_index * (SIZE_PACKET - SIZE_HEADER)), SIZE_PACKET - SIZE_HEADER);

                    //printf("\nBuffer_to_send %s\n", buffer_to_send);
                
                    printf("\n-------------->> Send\n");
                    printf("Num seq : %i\n", seq_num);
                    sendto(data_socket, (char *)buffer_to_send, SIZE_PACKET, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
                }

                seq_num += 1;
                current_index += 1;

                count_packet += 1;
            }
            
        }

        /* --- Waiting ACK_seq client --- */

        while(window_start == window_end) {

            printf("\n<<------------- Receiving ACK \n");

            n = -1;

            n = recvfrom(data_socket, (char *)buffer, SIZE_PACKET,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
            buffer[n] = '\0';

            printf("Received : %s\n", buffer);

            if(n != -1) {

                // Remove "ACK" from buffer
                memmove(buffer, buffer + 3, strlen(buffer));

                seq_received = atoi(buffer);
    
                if(seq_received > higher_seq_num_received) {

                    printf("-> **New higher**\n");
                    count = 0;

                    window_start = 0;
                    window_end = seq_received - higher_seq_num_received;

                    if(window_end > window_size) {
                        window_end = window_size;
                    }
                    higher_seq_num_received = seq_received;

                    // printf("Sequence rec : %i\n",seq_received); // Update window to the last ACK received
                    // printf("Window end : %i\n", window_end);


                } else {

                    // Client 2 has a loss of connection, so we go back

                    seq_num = higher_seq_num_received + 1;
                    current_index = higher_seq_num_received;
                    
                    window_start = 0;
                    window_end = window_size;
                }
            } else {
                printf("++++++++++++++ Nothing received ERROR ++++++++++++++++\n");
                seq_num = higher_seq_num_received + 1;
                current_index = higher_seq_num_received;

                window_start = 0;
                window_end = window_size;
            }


            printf("Empty packet in the window : %i\n",window_end-window_start);

            if(higher_seq_num_received == max_index + 1) {
                end_transmission = 0;
            }

            printf("num seq : %i\n", seq_num);
            printf("higher seq num received : %i\n", higher_seq_num_received);
                    
            
        }
    }

    memset(buffer_to_send, 0, (SIZE_PACKET - 1));

    strcat(buffer_to_send, "FIN");

    /* --- Sending "FIN" to client to end transmission --- */ 

    printf("-------------->> Send 3 times FIN in case drop\n\n");
    for(int i = 0; i < 3; i++) {
        sendto(data_socket, (char *)buffer_to_send, 3, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
    }


    printf("\n/* -------- End sending data ------- */\n\n");

    /* --- Closing the sockets --- */
    close(server_socket);
    close(data_socket);
    free(buffer_file_big);
    return 0; 
}
