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

struct f_data{
    uint32_t size;
    char* data;
} typedef file_data;

file_data* read_file(char* file_path){
    // make sure file exits
    // read it and create struct
    // what to do with data buffer size comment? should i split?
    // return null if err
    // make sure N in network byte order
}

int main(int argc, char* argv[]){
    int  sockfd     = -1;
    long l;
    int  bytes_read =  0, write_out = 0;
    file_data* file_info;
    uint32_t printable_rcvd;
    uint16_t port_num;
    struct sockaddr_in serv_addr;

    if (argc<4){
        fprintf(stderr, "invalid number of arguments. err- %s \n", strerror(errno));
        return 1;
    }

    file_info = read_file(argv[3]);
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

    l = strtol(argv[2], NULL, 10);
    if(errno || l<0 || l >= 0x10000){
        fprintf(stderr, "error parsing port number. err- %s\n", strerror(errno));
        return 1;
    }
    port_num = (uint16_t)l;

    // create server address data
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num); 
    inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr));

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

    // send file data
    write_out = write(sockfd, file_info->data, file_info->size);
    if( write_out <= 0 ){
        printf(stderr, "Error sending server file data bytes. err- %s \n", strerror(errno));
        return 1;
    }

    // revc num printable characters
    bytes_read = read(sockfd, printable_rcvd, sizeof(uint32_t));
    if( bytes_read <= 0 ){
        printf(stderr, "Error reading message from server. err- %s \n", strerror(errno));
        return 1;   
    }

    printf("# of printable characters: %u\n", printable_rcvd);

    close(sockfd);
    return 0;

}