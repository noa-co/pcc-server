
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#define BUFFER_SIZE 1000000 // 1MB

int count_printable(){
    // byte b is printable if 32<=b<=126
    // add count[b] ++
}

void sigint_handler(){
    // print counts
    // exit
}

void set_sigint_handler(){
    struct sigaction sig_info;
    sig_info.sa_flags = SA_RESTART;
    sig_info.sa_handler = sigint_handler;
    int sigac_out = sigaction(SIGINT, &sig_info, 0);
    if (sigac_out == -1){
       fprintf(stderr, "couldnt set sigint action. err: %s\n", strerror(errno)); 
    }
}

