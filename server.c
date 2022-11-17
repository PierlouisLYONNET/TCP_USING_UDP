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
#define PORTSERV 6666
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
    char *syn_ack = "SYN_ACK"; 
    char portserv[6];
    sprintf(portserv,"%i",PORTSERV);
    char *port= portserv; 
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
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 

    exchangeaddr.sin_family=AF_INET;
    exchangeaddr.sin_addr.s_addr = INADDR_ANY;
    exchangeaddr.sin_port = htons(PORTSERV); 


     
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

    // Waiting ACK client
    n = recvfrom(server_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0'; 
    printf("Received from client : %s\n", buffer);

    // Send SYN ACK
    sendto(server_socket, (const char *)syn_ack, strlen(syn_ack),  MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
    printf("Send : syn_ack message.\n");  

    // Waiting Port client
    n = recvfrom(server_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 
    printf("Received from client : %s\n", buffer);

    // Send Port
    sendto(server_socket, (const char *)port, strlen(port),  MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
    printf("Send : port adress.\n");  

    /* --- End of Threeway handshake --- */



    
    /* --- Receiving data from data_socket --- */

    n = recvfrom(data_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 
    printf("Client : %s\n", buffer); 

    char buffer_ftp[] = "ftp";



    /* --- Number of sequence --- */

    int num_seq = 1;
    int num_seq_last_ack = 0;
    char num_seq_char[SIZE_HEADER];
    char buffer_to_send[MAXLINE];



    /* --- Waiting for ftp --- */

    if(memcmp(buffer, buffer_ftp, 3)) { // TODO WIP "ftp"

        FILE* file_ptr;
        file_ptr = fopen("test.txt", "rb");

        if (NULL == file_ptr) {
            printf("File can't be opened \n");
        }

        char buffer_file[(MAXLINE - SIZE_HEADER)];

        

        /* --- Buffer TODO BIG of the file --- */

        fseek(file_ptr, 0, SEEK_END);
        long size = ftell(file_ptr);
        fseek(file_ptr, 0, SEEK_SET);  // On revient au début

        char *buffer_file_big = malloc(size + 1);
        fread(buffer_file_big, size, 1, file_ptr);

        buffer_file_big[size] = 0;

        fclose(file_ptr);

        

        /* --- Sending file data socket --- */

        int end_transmission = 1;
        int current_index = 0;
        int max_index = size/(MAXLINE - SIZE_HEADER);
        printf("max index : %i",max_index);

        while(end_transmission) {

            memset(buffer_to_send, 0, sizeof(buffer_to_send)); // TODO OPTIMIZE
            memset(num_seq_char, 0, sizeof(num_seq_char));

            
            sprintf(num_seq_char, "%06d%d", num_seq); // integer to string

            memcpy(buffer_to_send, num_seq_char, 6);

            printf("current index : %i\n", current_index);

            if(current_index == max_index){
                memcpy(buffer_to_send + 6, buffer_file_big + (current_index * (MAXLINE - SIZE_HEADER)), size - ((current_index) * (MAXLINE - SIZE_HEADER)));
            } else {
                memcpy(buffer_to_send + 6, buffer_file_big + (current_index * (MAXLINE - SIZE_HEADER)), MAXLINE - SIZE_HEADER);
            }
            

            printf("\n\nbuffer_to_send %s\n", buffer_to_send);
            

            printf("\n\n\n--------------Send-------------\n\n");
            sendto(data_socket, (char *)buffer_to_send, strlen(buffer_to_send), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 


            num_seq += 1;
            current_index += 1;

            if(current_index > max_index) {
                end_transmission = 0;
            }


            /* --- Waiting ACK_seq client --- */
            
            printf("\n\n\n----------Receive_ACK----------\n\n");
            n = recvfrom(data_socket, (char *)buffer, MAXLINE,  MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
            buffer[n] = '\0'; 
            printf("Received from client : %s\n", buffer);

            // Remove "ACK_" from buffer
            memmove(buffer, buffer + 4, strlen(buffer));        
            // printf("Received from client without ACK_ : %s\n", buffer);
            num_seq_last_ack = atoi(buffer);
            printf("num_seq_last_ack : %i\n", num_seq_last_ack);
            printf("num_seq : %i\n", num_seq);

            // TODO ACK
            // TODO Tableau de tableau
            // TODO JUSTE CE QU'IL FAUT

        }

        // TELL THE CLIENT TO STOP
        buffer_to_send[0] = '\0';
        buffer_file[0] = '\0';
        num_seq_char[0] = '\0';

        memset(buffer_to_send, 0, (MAXLINE - 1)); // TODO OPTIMIZE
        memset(buffer_file, 0, (MAXLINE - SIZE_HEADER - 1));
        memset(num_seq_char, 0, SIZE_HEADER);

        num_seq = 000000;

        sprintf(num_seq_char, "%i", num_seq); // integer to string

        strcat(buffer_to_send, num_seq_char);
        strcat(buffer_to_send, buffer_file);
        


        /* --- Sending num_seq = 0 to client to end transmission --- */

        printf("\n\n\n--------------Send-------------\n\n");
        sendto(data_socket, (char *)buffer_to_send, (n + SIZE_HEADER), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
        //printf("buffer : \n%s\n\n\n", buffer_to_send);


    } else {
        printf("nope\n");
    }


    /* --- Closing the sockets --- */

    close(server_socket);
    close(data_socket);
    return 0; 
}

