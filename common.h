/*------------------------------------------------------------------------------
-- HEADER FILE: common.h
-- 
-- DATE:        March 11, 2017
-- 
-- DESIGNER:    Fred Yang, Maitiu Morton
-- 
-- PROGRAMMER:  Fred Yang, Maitiu Morton
-- 
-- NOTES: 
-- This header file includes the basic macro definitions and function 
-- declarations for the application.
-------------------------------------------------------------------------------*/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <map>

#define NORMAL_EXIT     0       // normal exit
#define ERROR_EXIT      1       // error exit

#define MAX_CLIENT      30      // maximum # of clients allowed
#define MAX_NAME        100     // maximum length of name string
#define IP_SIZE         16      // maximum length of ip address string
#define PORT_SIZE       10      // maximum length of port string
#define BUF_SIZE        512     // buffer size
#define TCP_PORT        7000    // tcp port #

// color styles
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

// global variables
int clnt_sockfd;    // client socket file descriptor
int srv_sockfd;     // server socket file descriptor
char default_file[MAX_NAME] = "log.txt";  // default dump file
char default_host[MAX_NAME] = "datacomm"; // default host name
std::map<int, std::string> usermap; // store user info (hostname:ip:fd)

// function prototypes
// server side
int init_srv(int port);
int max(int a, int b);
int user_free(int user_link[MAX_CLIENT]);
char* get_hostname(const char* ipaddr);
void signal_srv(int signo);
void add_name(char* line, const char* name, int);
void set_name(char* line, char* name);
void remove_name(char* line, const char* name);
void add_sockset(fd_set* sockset, int sockfd, int* usrfd, const int* usrlink);

// client side
void leave();
void signal_clnt(int signo);
void add_set(fd_set *sockset, int sockfd);
int init_clnt(char* ipaddr, int port);

#endif
