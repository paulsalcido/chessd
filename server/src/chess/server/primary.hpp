#ifndef CHESS_SERVER_PRIMARY_VERSION
#define CHESS_SERVER_PRIMARY_VERSION 0.1

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>


#include <chess/server/options.hpp>
#include <chess/server/logger.hpp>

#include <boost/lexical_cast.hpp>

#include <string>
#include <stdio.h>


// For interprocess communication
// Will need to go to the forking class later on.
#include <boost/interprocess/ipc/message_queue.hpp>

namespace chess {
    namespace server {
        void sigchld_handler(int);

        class primary {
            public:
                primary(chess::server::options *server_options,
                    chess::server::logger *logger):
                    m_server_options(server_options),
                    m_logger(logger) {
                    this->m_logger->log("Creating primary server");
                }

                ~primary() {
                    this->m_logger->log("Destroying primary server");
                }

                bool main_connect() {
                    this->m_logger->log("creating primary connection");
                    /* TODO:
                        * handle errors for the following functions:
                            * getaddrinfo
                            * bind
                            * setsockopt
                        * add better response.
                            */
                    memset(&m_hints,0,sizeof m_hints);

                    m_hints.ai_family = AF_UNSPEC;
                    m_hints.ai_socktype = SOCK_STREAM;
                    m_hints.ai_flags = AI_PASSIVE;

                    this->m_logger->log("getting address information");
                    getaddrinfo(NULL,boost::lexical_cast<std::string>(m_server_options->port()).c_str(),&m_hints,&m_res);

                    this->m_logger->log("creating socket");
                    /* TODO: Change this socketing to match loop described in
                        http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
                        */
                    m_sockfd = socket(m_res->ai_family,m_res->ai_socktype,m_res->ai_protocol);

                    this->m_logger->log("binding socket to port" + boost::lexical_cast<std::string>(m_server_options->port()));
                    bind(m_sockfd,m_res->ai_addr,m_res->ai_addrlen);

                    this->m_logger->log("allowing port reuse option on port");
                    int yes = 1; // have to do, apparently...
                    if ( setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1 ) {
                        this->m_logger->error("Unable to set port reuse on port " 
                            + boost::lexical_cast<std::string>(m_server_options->port()));
                        // Keep going...
                    }

                    this->m_logger->log("creation of primary connection complete");
                    return true;
                }

                void start_listen() {
                    this->m_logger->log("start_listen called");
                    /* TODO:
                        * Add error handling for the following functions:
                            * listen
                            * accept
                            * recv
                            * send
                            */
                    listen(m_sockfd,m_server_options->backlog_limit());

                    this->m_logger->log("beginning the accept loop");

                    while (1) {
                        struct sockaddr_storage their_addr;
                        struct sockaddr_in peer_addr;
                        struct sockaddr_in6 peer_addr6;
                        socklen_t addr_size;
                        socklen_t peer_addr_size;
                        socklen_t peer_addr_size6;
                        addr_size = sizeof their_addr;

                        int new_fd = accept(m_sockfd,(struct sockaddr *)&their_addr,&addr_size);
                        std::string typical_message = "chess server what nots\n";
                        char msg[200];

                        peer_addr_size = sizeof peer_addr;
                        peer_addr_size6 = sizeof peer_addr6;

                        char peer_char_v4[INET_ADDRSTRLEN];
                        char peer_char_v6[INET6_ADDRSTRLEN];

                        //getpeername(new_fd,(struct sockaddr *)&their_addr,&peer_addr_size);
                        inet_ntop(AF_INET,&their_addr,peer_char_v4,INET_ADDRSTRLEN);
                        this->m_logger->debug((char*)peer_char_v4);
                        inet_ntop(AF_INET6,&their_addr,peer_char_v6,INET6_ADDRSTRLEN);
                        this->m_logger->debug((char*)peer_char_v6);

                        // Temporarily handle one connection.
                        //if ( ! fork() ) {
                            // Following is the actual server response code.
                            _handle_fork(new_fd);
                        //}
                        close(new_fd);
                        /*recv(new_fd,msg,200,0);
                        send(new_fd,typical_message.c_str(),strlen(typical_message.c_str()),0);*/
                    }
                    /* TODO: 
                        * Implement getpeername so that I can know who connected.
                        * Implement gethostname so that I can know who I am.
                        */
                }
            protected:
                bool _handle_fork(int new_fd) {
                    struct timeval tv;
                    tv.tv_sec = 2;
                    tv.tv_usec = 0;
                    char buf[1000];
                    std::string whitespaces(" \t\f\v\n\r");
                    close(m_sockfd);
                    fd_set readfds;
                    fd_set master;
                    FD_ZERO(&master);
                    FD_SET(new_fd,&master);
                    FD_SET(fileno(stdin),&master);
                    while ( 1 ) {
                        readfds = master;
                        select(new_fd+1,&readfds,NULL,NULL,&tv);
                        if ( FD_ISSET(new_fd,&readfds) ) {
                            memset(buf,0,1000);
                            this->m_logger->debug("Read data from socket.");
                            int readamount = recv(new_fd,buf,1000,0);
                            std::string teststr(buf);
                            teststr.erase(teststr.find_last_not_of(whitespaces)+1);
                            this->m_logger->debug(teststr);
                            if ( teststr == "quit" || readamount == 0 ) {
                                this->m_logger->debug("Closing socket connection.");
                                close(new_fd);
                                exit(0);
                            }
                        } else if ( FD_ISSET(fileno(stdin),&readfds) ) {
                            std::string fullmessage = "";
                            int notsent = 1;
                            while ( notsent ) {
                                std::cin.getline(buf,1000);
                                std::string teststr3(buf);
                            /*    if ( teststr3.length() > 7 ) {
                                    this->m_logger->debug("Ending: " + teststr3.substr(teststr3.length()-7,7) );
                                }
                            if ( teststr3.length() > 7 && teststr3.substr(teststr3.length()-7,7) == "endline" ) {
                                teststr3.erase(teststr3.length()-7,7);
                                teststr3 += "\nfics% ";
                            }
                            teststr3 = "\n" + teststr3;
                            this->m_logger->debug("Sending: " + teststr3);
                            int data = send(new_fd,teststr3.c_str(),strlen(teststr3.c_str()),0);
                            this->m_logger->debug("Sent " + boost::lexical_cast<std::string>(data));*/
                                if ( teststr3 == "finished" ) {
                                    int test = 0;
                                    while ( test < strlen(fullmessage.c_str()) ) {
                                        test += send(new_fd,fullmessage.c_str(),strlen(fullmessage.c_str()),0);
                                    }
                                    this->m_logger->debug("Sent " + boost::lexical_cast<std::string>(test));
                                    notsent = 0;
                                } else {
                                    if ( fullmessage.length() > 0 ) {
                                        fullmessage += "\n";
                                    }
                                    fullmessage += teststr3;
                                }
                            }
                        }
                        sleep(1);
                    }
                    /*if ( send(new_fd,typical_message.c_str(),strlen(typical_message.c_str()),0) == -1 ) {
                        this->m_logger->error("Could not send message in child pid.");
                    }
                    sleep(10);
                    if ( send(new_fd,typical_message.c_str(),strlen(typical_message.c_str()),0) == -1 ) {
                        this->m_logger->error("Could not send message in child pid.");
                    }
                    close(new_fd);*/
                    close(new_fd);
                    exit(0);
                }

                chess::server::options *m_server_options;
                chess::server::logger *m_logger;

                struct addrinfo m_hints;
                struct addrinfo *m_res;
                int m_sockfd;
        };

        void sigchld_handler(int s) {
            while(waitpid(-1,NULL,WNOHANG) > 0);
        }
    }
}

#endif
