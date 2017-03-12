/*------------------------------------------------------------------------------
-- SOURCE FILE: chatclnt.c - Implement a chat client capable of sending text 
--              strings to the server and also displaying the text sent by all
--              other clients.
-- 
-- PROGRAM:     chatclnt
-- 
-- FUNCTIONS:   int main(int argc, char *argv[])
--              int init_clnt(char* ipaddr, int port);
--              void add_set(fd_set *sockset, int sockfd);
--              void signal_clnt(int signo);
--              void leave();
-- 
-- DATE:        March 11, 2017
-- 
-- DESIGNER:    Fred Yang, Maitiu Morton
-- 
-- PROGRAMMER:  Fred Yang, Maitiu Morton
-- 
-- NOTES: 
-- Each chat participant will not only see the text string but also the client
-- information it was from. The client information includes hostname, ipaddress,
-- and the file descriptor connected.
-- User can specify (command line argument) that the chat session also be dumped
-- to a file with CR-LF terminated records.
------------------------------------------------------------------------------*/

#include "common.h"
using namespace std;

/*------------------------------------------------------------------------------
-- FUNCTION:    main
-- 
-- DATE:        March 11, 2017
-- 
-- DESIGNER:    Fred Yang, Maitiu Morton
-- 
-- PROGRAMMER:  Fred Yang, Maitiu Morton
--
-- INTERFACE:   int main(int argc, char* argv[]) 
--              int argc: the number of arguments input
--              char* argv[]: the list of arguments input
-- 
-- RETURNS:     return zero if it exits normally, otherwise return a 
--              non-zero value
-- 
-- NOTES: 
-- Main entry of the program.
--
------------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
    int     sockfd;             // socket file descriptor
    int     readbytes;          // bytes read from file descriptor specified
    int     tcpport;            // tcp port #
    char    hostaddr[IP_SIZE];  // host ip address
    char    msg[BUF_SIZE];      // message to transfer
    char    name[MAX_NAME];     // nick name
    char    input[MAX_NAME];    // input prompt
    char    file[MAX_NAME];     // file to dump the chat records
    fd_set  sockset;            // file descriptor sets

    // call signal_clnt() on SIGINT
    signal(SIGINT, signal_clnt);
    
    // program usage
    if(argc < 3) {
        printf("Usage: %s <IP> <Port> [File]\n", argv[0]);
        return ERROR_EXIT;
    }

    strcpy(hostaddr, argv[1]);
    tcpport = strtol(argv[2], NULL, PORT_SIZE);
    if (argc == 4) {
        strcpy(file, argv[3]);
    }
    else {
        strcpy(file, default_file);
    }
    
    // open file as ofstream
    ofstream ofs;
    ofs.open(file, ofstream::out | ofstream::app);
    
    // initialize client socket given host ip and port #
    sockfd = init_clnt(hostaddr, tcpport);
    
    if (sockfd == 0) {
        perror("Init client socket error.\n");
        fflush(stdout);
        exit(1);
    }

    fprintf(stdout, "- Chat room client running, press /q to leave the room\n");
    
    // add socket file descriptor onto the set
    clnt_sockfd = sockfd;
    add_set(&sockset, sockfd);

    // prompt for nickname
    strcpy(input, "Please input your nickname:");
    fprintf(stdout, input);
    ofs << input;
    fscanf(stdin, "%s", name);
    strcpy(msg, "/");
    strcat(msg, name);
    write(sockfd, msg, strlen(msg));
    ofs << name << endl;
    
    while (1) {
        /* 
         * select() allows the client to monitor multiple file descriptors
         * waiting until one or more of the file descriptors become "ready"
         * for I/O operation (input, output, read, and write).
         */
        select(sockfd + 1, &sockset, NULL, NULL, NULL);
        
        // socket fd is ready for READ
        if (FD_ISSET(sockfd, &sockset)) {
            readbytes = read(sockfd, msg, BUF_SIZE);
            if (readbytes == 0) exit(0);
            msg[readbytes] = '\0';
            ofs << msg << endl;
            printf("%s", msg);
            fflush(stdout);
        }
        
        // input fd is ready for READ and then WRITE
        if (FD_ISSET(0, &sockset)) {
            readbytes = read(0, msg, BUF_SIZE);
            msg[readbytes] = '\0';
            ofs << msg << endl;
            
            if (msg[0] == '/' && msg[1] == 'q') {
                leave();
            }
            
            int len = strlen(msg);
            if (write(sockfd, msg, len) != len) {
                printf("client: write socket error.\n");
                exit(0);
            }
        }
        
        // add sockfd onto the set
        add_set(&sockset, sockfd);
   }
   
   // close file stream
   ofs.close();
   return NORMAL_EXIT;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    init_clnt
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   int init_clnt(char* ipaddr, int port)
--              char* ipaddr: the ip address of the server
--              int port: the port # the server listening to
-- 
-- RETURNS:     return socket file descriptor on success, 0 on failure
-- 
-- NOTES:
-- This function is called to create a client socket and connect to the server
-- via this socket.
------------------------------------------------------------------------------*/
int init_clnt(char* ipaddr, int port)
{
    int     sockfd;
    struct  sockaddr_in serv_addr;

    // create a stream socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client: can't open stream socket.\n");
        fflush(stdout);
        return 0;
    }

    // bind an address to the socket
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ipaddr);
    serv_addr.sin_port = htons(port);
    
    // connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)) < 0) {
        perror("client: can't connect to server.\n");
        fflush(stdout);
        return 0;
    }
    
    return sockfd;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    add_set
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   void add_set(fd_set* sockset, int sockfd)
--              fd_set* sockset: the set that holds the socket file descriptors
--              int sockfd: the socket file descriptor to add to the set
-- 
-- RETURNS:     void
-- 
-- NOTES:
-- This function is called to add a given file descriptor to the fd set.
------------------------------------------------------------------------------*/
void add_set(fd_set* sockset, int sockfd)
{
    FD_ZERO(sockset);
    FD_SET(sockfd, sockset);
    FD_SET(0, sockset);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    leave
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Maitiu Morton
-- 
-- PROGRAMMER:  Maitiu Morton
-- 
-- INTERFACE:   void leave()
-- 
-- RETURNS:     void
-- 
-- NOTES:
-- This function is called to send a quit command to the server and then close
-- the client socket.
------------------------------------------------------------------------------*/
void leave() {
    write(clnt_sockfd, "/q\n", 3);
    close(clnt_sockfd);
    exit(0);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    signal_clnt
-- 
-- DATE:        March 11, 2017
-- 
-- DESIGNER:    Maitiu Morton
-- 
-- PROGRAMMER:  Maitiu Morton
-- 
-- INTERFACE:   void signal_clnt(int signo)
--              int signo: the signal # passed in
-- 
-- RETURNS:     void
-- 
-- NOTES: 
-- This function will be invoked when the user press 'Ctrl+C' to terminate
-- the program. It will call leave() method to quit the chat room and close 
-- the client socket.
------------------------------------------------------------------------------*/
void signal_clnt(int signo) {
    if (signo == SIGINT) {
        signal(SIGINT, NULL);
        leave();
    }
}
