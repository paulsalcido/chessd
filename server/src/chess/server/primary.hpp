#ifndef CHESS_SERVER_PRIMARY_VERSION
#define CHESS_SERVER_PRIMARY_VERSION 0.1

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include <chess/server/options.hpp>
#include <chess/server/logger.hpp>

#include <boost/lexical_cast.hpp>

#include <string>

namespace chess {
    namespace server {
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
                    memset(&m_hints,0,sizeof m_hints);

                    m_hints.ai_family = AF_UNSPEC;
                    m_hints.ai_socktype = SOCK_STREAM;
                    m_hints.ai_flags = AI_PASSIVE;

                    this->m_logger->log("getting address information");
                    getaddrinfo(NULL,boost::lexical_cast<std::string>(m_server_options->port()).c_str(),&m_hints,&m_res);

                    this->m_logger->log("creating socket");
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
                    listen(m_sockfd,m_server_options->backlog_limit());

                    this->m_logger->log("beginning the accept loop");

                    struct sockaddr_storage their_addr;
                    socklen_t addr_size;
                    addr_size = sizeof their_addr;

                    int new_fd = accept(m_sockfd,(struct sockaddr *)&their_addr,&addr_size);
                    std::string typical_message = "chess server what nots";
                    char msg[200];

                    recv(new_fd,msg,200,0);
                    send(new_fd,typical_message.c_str(),strlen(typical_message.c_str()),0);
                    close(new_fd);
                }
            protected:

                chess::server::options *m_server_options;
                chess::server::logger *m_logger;

                struct addrinfo m_hints;
                struct addrinfo *m_res;
                int m_sockfd;
        };
    }
}

#endif
