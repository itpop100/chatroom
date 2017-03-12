/*------------------------------------------------------------------------------
-- SOURCE FILE: chatsrv.c - This program acts as a chat room server to serve 
--              multiple clients. The server accepts connections on a specified 
--              port and once clients have established a connection with it, it
--              will echo whatever it receives to all other connected clients.
-- 
-- PROGRAM:     chatsrv
-- 
-- FUNCTIONS:   int main(void)
--              int init_srv(int port);
--              int max(int a, int b);
--              int user_free(int user_link[MAX_CLIENT]);
--              char* get_hostname(const char*);
--              void signal_srv(int signo);
--              void add_name(char* line, const char* name, int);
--              void set_name(char* line, char* name);
--              void remove_name(char* line, const char* name);
--              void add_sockset(fd_set* sockset, int sockfd, int* userfd, 
--                               const int* usrlink);
-- 
-- DATE:        March 11, 2017
-- 
-- DESIGNER:    Fred Yang, Maitiu Morton
-- 
-- PROGRAMMER:  Fred Yang, Maitiu Morton
-- 
-- NOTES:
-- Multiplexed I/O is implemented in the program. The server maintains a list 
-- of all connected clients (host names) and display the updated list on the 
-- console.
-- The server echoes the text strings it receives from each client to all other
-- clients except the one that sent it.
--
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
-- INTERFACE:   int main(void) 
-- 
-- RETURNS:     return zero if it exits normally, otherwise return a 
--              non-zero value
-- 
-- NOTES: 
-- Main entry of the program.
--
------------------------------------------------------------------------------*/
int main(void)
{
    int     sockfd;                 // socket file descriptor
    int     newsockfd;              // socket file descriptor for new connection
    int     userlink[MAX_CLIENT];   // array that stores client connect status
    int     userfd[MAX_CLIENT];     // array that stores client connect fd
    char    userinfo[MAX_NAME];     // hostname:ip:fd
    char    username[MAX_CLIENT][MAX_NAME];// user nickname
    char    line[BUF_SIZE];         // temporary line (message)
    char    ipbuf[IP_SIZE];         // stores client ip address
    int     userindex;              // first index of userlink with value=0
    int     length, i, j;           // temporary variables
    int     maxfd = 0;              // maximum file descriptor
    unsigned int cli_len;           // size of sockaddr_in struct
    struct  sockaddr_in cli_addr;   // socketaddr_in struct
    fd_set  sockset;                // fd set
    
    // call signal_srv() on SIGINT
    signal(SIGINT, signal_srv);
    
     // initialize server socket given port #
    sockfd = init_srv(TCP_PORT);

    if (sockfd == 0) {
        perror(" - Init server socket error.\n");
        fflush(stdout);
        exit(1);
    }

    srv_sockfd = sockfd;
    fprintf(stdout, " - Chat room server running, press CTRL+C to exit\n");
    
    // listen for connections, BACKLOG = MAX_CLIENT
    listen(sockfd, MAX_CLIENT);
    cli_len = sizeof(cli_addr);
    
    for ( i = 0; i < MAX_CLIENT; i++) {
        userlink[i] = 0;
        username[i][0] = '\0';
    }
    
    userindex = 0;
    
    // clear the fd set
    FD_ZERO(&sockset);
    
    // add sockfd to the set
    FD_SET(sockfd, &sockset);
    
    // update maxfd
    maxfd = max(maxfd, sockfd + 1);

    while (1) {
        /* 
         * select() allows the server to monitor multiple file descriptors
         * waiting until one or more of the file descriptors become "ready"
         * for I/O operation (input, output, read, and write).
         */
        select(maxfd, &sockset, NULL, NULL, NULL);
        
        // new client connection
        if (FD_ISSET(sockfd, &sockset)
            && (userindex = user_free(userlink)) >= 0) {
            // accept new connection
            newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
            
            if (newsockfd < 0) {
                userlink[userindex] = 0;
                perror(" - server: accept error.\n");
            } else {
                userlink[userindex] = 1;
                userfd[userindex] = newsockfd;
                FD_SET(newsockfd, &sockset);
                maxfd = max(maxfd, newsockfd + 1);
                
                // get client ip address
                strcpy(ipbuf, inet_ntoa(cli_addr.sin_addr));
                
                // build userinfo - hostname:ip:fd
                sprintf(userinfo, "%s:%s:%d", get_hostname(ipbuf), ipbuf, userindex+4);
                
                // store the userinfo in a map
                if (usermap.find(newsockfd) == usermap.end())
                    usermap.insert(pair<int, string>(userindex+4, userinfo));
                
                printf(" - Connection established: [%s]\n", userinfo);
            }
        }
        
        // traverse clients to transfer messages
        for (i = 0; i < MAX_CLIENT; i++) {
            if ((userlink[i] == 1) && (FD_ISSET(userfd[i], &sockset))) {
                length = read(userfd[i], line, BUF_SIZE);
                
                if (length == 0) { // socket is closed.
                    userlink[i] = 0;
                    username[i][0] = '\0';
                    FD_CLR(userfd[i], &sockset);
                } else if (length > 0) {
                    line[length] = '\0';
                    
                    if ((line[0] == '/') && (username[i][0] == '\0')) {
                        // set nick name
                        set_name(line, username[i]);
                    } else if (line[0] == '/' && line[1] == 'q') {
                        // user quit the chat room
                        remove_name(line, username[i]);
                        string removeinfo(usermap.find(i+4)->second);
                        printf(" - Connection removed: [%s]\n", removeinfo.c_str());
                        usermap.erase(i + 4);
                        userlink[i] = 0;
                        username[i][0] = '\0';
                        FD_CLR(userfd[i], &sockset);
                    } else {
                        // build the message body - name: message (userinfo)
                        add_name(line, username[i], i + 4);
                    }
                    
                    // distribute messages to all clients except the sender
                    for (j = 0; j < MAX_CLIENT; j++) {
                        if ((j != i) && (userlink[j] == 1)) {
                            write(userfd[j], line, strlen(line));
                        }
                    }
                }
            }
        }
        
        // update fd set
        add_sockset(&sockset, sockfd, userfd, userlink);
    }
    
    return NORMAL_EXIT;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    init_srv
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   int init_srv(int port)
--              int port: the port # the server listening to
-- 
-- RETURNS:     return socket file descriptor on success, 0 on failure
-- 
-- NOTES:
-- This function is called to create a server socket listening for 
-- client connections.
------------------------------------------------------------------------------*/
int init_srv(int port)
{
    int     sockfd;
    struct  sockaddr_in serv_addr;

    // create a stream socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(" - server: can't open stream socket.\n");
        fflush(stdout);
        return 0;
    }

    // bind an address to the socket
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    // bind to the socket created
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror(" - server: can't bind local address.\n");
        fflush(stdout);
        return 0;
    }
    
    return sockfd;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    set_name
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   void set_name(char* line, char* name)
--              char* line: the input line
--              char* name: nickname specified
-- 
-- RETURNS:     void
-- 
-- NOTES:
-- This function is called to set the user nick name and output message 
-- "... join the room..."
------------------------------------------------------------------------------*/
void set_name(char* line, char* name)
{
    strcpy(name, &line[1]);
    sprintf(line, "%s%s join the room...%s\n", MAG, name, RESET);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    remove_name
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   void remove_name(char* line, const char* name)
--              char* line: the input line
--              char* name: nickname to remove
-- 
-- RETURNS:     void
-- 
-- NOTES:
-- This function is called to output the message "... leave the room..."
------------------------------------------------------------------------------*/
void remove_name(char* line, const char* name)
{
    sprintf(line, "%s%s leave the room...%s\n", MAG, name, RESET);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    add_set
-- 
-- DATE:        March 13, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   void add_name(char* line, const char* name, int sockfd)
--              char* line: the input line
--              char* name: nickname specified
--              int sockfd: the socket file descriptor specified
-- 
-- RETURNS:     void
-- 
-- NOTES:
-- This function is called to output the message body plus user info.
------------------------------------------------------------------------------*/
void add_name(char* line, const char* name, int sockfd)
{
    char theline[BUF_SIZE];
    string userinfo(usermap.find(sockfd)->second);
    
    strcpy(theline, name);
    strcat(theline, ": ");
    strcat(theline, line);
    string msg(theline);
    sprintf(line, "%s%s %s[from %s]%s\n", YEL, msg.substr(0,msg.size()-1).c_str(), 
            CYN, userinfo.c_str(), RESET);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    user_free
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   int user_free(int usrlink[MAX_CLIENT])
--              int usrlink[MAX_CLIENT]: array that stores client connect status
-- 
-- RETURNS:     the first index of the array usrlink with value = 0 (free to 
--              connect); return -1 if all values = 1 (connected)
-- 
-- NOTES:
-- This function is called to return the first index of the array usrlink
-- with the value = 0 (free to connect).
------------------------------------------------------------------------------*/
int user_free(int usrlink[MAX_CLIENT])
{
    int i = 0;

    while ((usrlink[i] != 0) && ( i < MAX_CLIENT)) i++;
    if (i == MAX_CLIENT) return -1;
    return i;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    add_sockset
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Fred Yang
-- 
-- PROGRAMMER:  Fred Yang
-- 
-- INTERFACE:   void add_sockset(fd_set* sockset, int sockfd, int* usrfd, const int* usrlink)
--              fd_set* sockset: the set that holds the socket file descriptors
--              int sockfd: the socket file descriptor to add to the set
--              int* usrfd: points to userfd array
--              const int* usrlink: points to userlink array
-- 
-- RETURNS:     void
-- 
-- NOTES:
-- This function is called to add a given sockfd and userfd to the fd set.
------------------------------------------------------------------------------*/
void add_sockset(fd_set* sockset, int sockfd, int* usrfd, const int* usrlink)
{
    int i;

    FD_ZERO(sockset);
    FD_SET(sockfd, sockset);
    for (i = 0; i < MAX_CLIENT; i++) {
        if (usrlink[i] == 1) {
            FD_SET(usrfd[i], sockset);
        }
    }
}

/*------------------------------------------------------------------------------
-- FUNCTION:    get_hostname
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Maitiu Morton
-- 
-- PROGRAMMER:  Maitiu Morton
-- 
-- INTERFACE:   char* get_hostname(const char* ipaddr)
--              const char* ipaddr: host ip address to be resolved
-- 
-- RETURNS:     returns the host name given the ip address
-- 
-- NOTES: 
-- Resolve host name given an ip address.
------------------------------------------------------------------------------*/
char* get_hostname(const char* ipaddr)
{
    struct hostent *hp;
    struct in_addr addr, *addr_p;
    char **phost;

    char* host = (char*) malloc(255);
    host[0] = '\0';

    addr_p = &addr;
    inet_aton(ipaddr, &addr);

    hp = gethostbyaddr(addr_p, sizeof(addr), PF_INET);
    if (hp == NULL)
    {
        return default_host;
    }

    for (phost = hp->h_addr_list; *phost != 0; phost++)
    {
        strcpy(host, hp->h_name);
        break;
    }

    return host;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    max
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Maitiu Morton
-- 
-- PROGRAMMER:  Maitiu Morton
-- 
-- INTERFACE:   int max(int a, int b)
--              int a: the first integer to be evaluated
--              int b: the second integer to be evaluated
-- 
-- RETURNS:     returns the bigger one of a & b
-- 
-- NOTES: 
-- Called to return the bigger one of a & b.
------------------------------------------------------------------------------*/
int max(int a, int b)
{
    return (a > b) ? a : b;
}
    
/*------------------------------------------------------------------------------
-- FUNCTION:    signal_srv
-- 
-- DATE:        March 12, 2017
-- 
-- DESIGNER:    Maitiu Morton
-- 
-- PROGRAMMER:  Maitiu Morton
-- 
-- INTERFACE:   void signal_srv(int signo)
--              int signo: the signal # passed in
-- 
-- RETURNS:     void
-- 
-- NOTES: 
-- This function will be invoked when the user press 'Ctrl+C' to terminate
-- the program. It will call close the server socket before termination.
------------------------------------------------------------------------------*/
void signal_srv(int signo) {
    if (signo == SIGINT) {
        signal(SIGINT, NULL);
        close(srv_sockfd);
        exit(1);
    }
}