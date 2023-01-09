#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#define BUFFER_SIZE 1000000 // 1MB

// note - a lot of code copied from rec10 code examples
// todo - make sure N in network byte order
struct f_data{
    uint32_t size;
    FILE* fp;
} typedef file_data;

file_data* open_file(char* file_path){
    uint32_t size;
    file_data* fd;
    FILE *fp = fopen(file_path, "rb");
    if(fp == NULL){
        return NULL;
    }
    fseek(fp, 0 , SEEK_END); // goes to end of file
    size = (uint32_t)ftell(fp);

    fd = (file_data*)malloc(sizeof(file_data));
    fd->size = size;
    fd->fp = fp;
}

void create_server_address(struct sockaddr_in* serv_addr, uint16_t port_num, char* serv_ip){
    memset(serv_addr, 0, sizeof(*serv_addr));
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(port_num); 
    inet_pton(AF_INET, serv_ip, &(serv_addr->sin_addr));
}

uint16_t parse_port_num(char* str){
    long l = strtol(str, NULL, 10);
    if(errno || l<0 || l >= 0x10000){
        fprintf(stderr, "error parsing port number. err- %s\n", strerror(errno));
        exit(1);
    }
    return (uint16_t)l;

}

void send_file_in_chunks(file_data* fd, int sockfd){
    FILE* fp = fd->fp;
    size_t bytes_read = 0;
    int write_out = 0;
    char buffer[BUFFER_SIZE]; 

    while(bytes_read = fread(buffer, 1, BUFFER_SIZE, fd)>0){ // todo - think about error
        write_out = write(sockfd, buffer, bytes_read);
        if( write_out <= 0 ){
            printf(stderr, "Error sending file data bytes. err- %s \n", strerror(errno));
            return 1;
        }
    }
    
}

int main(int argc, char* argv[]){
    int  sockfd     = -1;
    long l;
    int  bytes_read =  0, write_out = 0;
    file_data* file_info;
    char send_buff[1024];
    uint32_t printable_rcvd;
    uint16_t port_num;
    struct sockaddr_in serv_addr;

    if (argc<4){
        fprintf(stderr, "invalid number of arguments. err- %s \n", strerror(errno));
        return 1;
    }

    file_info = open_file(argv[3]);
    if(file_info == NULL){
        fprintf(stderr, "error reading file. err- %s \n", strerror(errno));
        return 1;
    }       

    // create client socket
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Could not create socket. err- %s\n", strerror(errno));
        return 1;
    }

    port_num = parse_port_num(argv[2]); 
    create_server_address(&serv_addr, port_num, argv[1]);

    printf("Client: connecting...\n");
    // connect socket to the target address
    if( connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf(stderr, "Connect to server failed. err- %s \n", strerror(errno));
        return 1;
    }
    
    // send N
    write_out = write(sockfd, file_info->size, sizeof(uint32_t));
    if( write_out <= 0 ){
        printf(stderr, "Error sending server N. err- %s \n", strerror(errno));
        return 1;
    }

    send_file_in_chunks(file_info, sockfd);

    // revc num printable characters
    bytes_read = read(sockfd, printable_rcvd, sizeof(uint32_t));
    if( bytes_read <= 0 ){
        printf(stderr, "Error reading message from server. err- %s \n", strerror(errno));
        return 1;   
    }

    printf("# of printable characters: %u\n", printable_rcvd);
    close(file_info->fp);
    close(sockfd);
    return 0;

}