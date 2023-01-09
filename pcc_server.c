
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1000000 // 1MB

// global variables for sigint to use
int no_sigint = 1; 
int connfd = -1;
uint32_t pcc_total[95] = {0};

void reset_pcc(uint32_t pcc_client[], int size){
    int i;
    for (i=0; i<size; i++){
        pcc_client[i] = 0;
    }
}

void update_pcc_and_reset(uint32_t pcc_client[], int size){
    int i;
    for (i=0; i<size; i++){
        pcc_total[i] = pcc_total[i] + pcc_client[i];
        pcc_client[i] = 0;
    }
}

int count_printable_bytes(char bytes_buffer[], int size, uint32_t pcc_count[]){
    int i;
    int count = 0;
    uint32_t char_num;
    for (i=0; i < size; i++){
        char_num = (uint32_t)bytes_buffer[i];
        if(char_num < 32 || char_num > 126)
            continue;
        count++;
        pcc_count[char_num-32] = pcc_count[char_num-32] + 1; // todo - make sure set to zero at begining
    }
    return count;
}

void print_and_exit(){
    int i;
    for (i = 0; i < 95; i++){
        printf("char '%c' : %u times\n", (char)(i+32), pcc_total[i]);
    }
    exit(0);
}

uint16_t parse_port_num(char* str){
    long l = strtol(str, NULL, 10);
    if(errno || l<0 || l >= 0x10000){
        fprintf(stderr, "error parsing port number. err- %s\n", strerror(errno));
        exit(1);
    }
    if(l<1024){
        fprintf(stderr, "invalid port number.\n");
        exit(1);
    }
    return (uint16_t)l;

}

void sigint_handler(){
    if(connfd == -1){
        print_and_exit();
    }
    no_sigint = 0;
}

void set_sigint_handler(){
    struct sigaction sig_info;
    sig_info.sa_flags = SA_RESTART;
    sig_info.sa_handler = &sigint_handler;
    int sigac_out = sigaction(SIGINT, &sig_info, 0);
    if (sigac_out == -1){
       fprintf(stderr, "couldnt set sigint action. err: %s\n", strerror(errno)); 
       exit(1);
    }
}

int  main(int argc, char *argv[]){
    int listenfd  = -1, connfd = -1;
    int bytes_read = 0, write_out = 0;
    int move_to_next_client = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    uint32_t pcc_client[95] = {0};
    uint16_t port_num;
    uint32_t client_N, left_to_read, printable_count;
    char bytes_buffer[BUFFER_SIZE];
    socklen_t addrsize = sizeof(struct sockaddr_in );

    if(argc < 2){
        fprintf(stderr, "invalid number of arguments.\n");
        return 1;
    }

    port_num = parse_port_num(argv[1]);

    if((listenfd = socket( AF_INET, SOCK_STREAM, 0 )) < 0){
        fprintf(stderr, "failed creating listening socket. err- %s \n", strerror(errno));
        return 1;
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){ // todo make sure
        fprintf(stderr, "failed to setsocketopt SO_REUSEADDR. err- %s \n", strerror(errno));
        return 1;
    }

    memset( &serv_addr, 0, addrsize);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port_num);
    if( 0 != bind( listenfd, (struct sockaddr*) &serv_addr, addrsize)) {
        fprintf(stderr, "failed to bind. err- %s \n", strerror(errno));
        return 1;
    }

    if( 0 != listen( listenfd, 10 )){
        fprintf(stderr, "failed to listen. err- %s \n", strerror(errno));
        return 1;
    }

    set_sigint_handler();
    while(no_sigint) 
    {
        connfd = accept( listenfd, (struct sockaddr*) &peer_addr, &addrsize);
        if(connfd < 0){    
            fprintf(stderr, "failed to accept client connection. err- %s \n", strerror(errno));
            return 1;
        }

        bytes_read = read(connfd, &client_N, sizeof(uint32_t));
        if( bytes_read <= 0 ){
            fprintf(stderr, "Error reading N from client. err- %s \n", strerror(errno));
            if (bytes_read == 0 || errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                connfd = -1;
                continue; // continuing to next connection
            }
            return 1;   
        }
        

        move_to_next_client = 0;
        printable_count = 0;
        left_to_read = ntohl(client_N);
        while (left_to_read > 0)
        {
            bytes_read = read(connfd, bytes_buffer, BUFFER_SIZE);
            if( bytes_read <= 0 ){
                fprintf(stderr, "Error reading bytes data from client. err- %s \n", strerror(errno));
                if (bytes_read == 0 || errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                    move_to_next_client = 1;
                    break;
                }
                return 1;   
            }
            printable_count = printable_count + count_printable_bytes(bytes_buffer, bytes_read, pcc_client);
            left_to_read = left_to_read - bytes_read;
        }

        if(move_to_next_client){
            reset_pcc(pcc_client, 95);
            close(connfd);
            connfd = -1;
            continue;
        }

        printable_count = htonl(printable_count);
        write_out = write(connfd, &printable_count, sizeof(uint32_t));
        if( write_out <= 0 ){
            fprintf(stderr, "Error sending printable count to client. err- %s \n", strerror(errno));
            if (bytes_read == 0 || errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                reset_pcc(pcc_client, 95);
                close(connfd);
                connfd = -1;
                continue;
            }
            return 1;
        }
        update_pcc_and_reset(pcc_client, 95);
        close(connfd);
        connfd = -1;
    }
    print_and_exit();
    return 0;
}

