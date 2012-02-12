#ifndef CHESS_SERVER_PRIMARY_VERSION
#define CHESS_SERVER_PRIMARY_VERSION 0.1

#ifndef DATA_BUFFER_SIZE
#define DATA_BUFFER_SIZE 1000
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>


#include <chess/server/options.hpp>
#include <chess/server/logger.hpp>

#include <chess/server/notifier.hpp>
#include <chess/server/secondary.hpp>

#include <boost/lexical_cast.hpp>

#include <string>
#include <stdio.h>

namespace chess {
    namespace server {
        void sigchld_handler(int);

        class primary {
            public:
                primary(chess::server::options *server_options,
                    chess::server::logger *logger):
                    m_options(server_options),
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
                    getaddrinfo(NULL,boost::lexical_cast<std::string>(m_options->port()).c_str(),&m_hints,&m_res);

                    this->m_logger->log("creating socket");
                    /* TODO: Change this socketing to match loop described in
                        http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
                        */
                    m_sockfd = socket(m_res->ai_family,m_res->ai_socktype,m_res->ai_protocol);

                    this->m_logger->log("binding socket to port" + boost::lexical_cast<std::string>(m_options->port()));
                    bind(m_sockfd,m_res->ai_addr,m_res->ai_addrlen);

                    this->m_logger->log("allowing port reuse option on port");
                    int yes = 1; // have to do, apparently...
                    if ( setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1 ) {
                        this->m_logger->error("Unable to set port reuse on port " 
                            + boost::lexical_cast<std::string>(m_options->port()));
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
                    listen(m_sockfd,m_options->backlog_limit());

                    this->m_logger->log("starting the notification server");

                    this->m_notifier = new chess::server::notifier(
                        this->m_sockfd,
                        this->m_logger,
                        this->m_options
                    );

                    this->m_notifier->start();

                    mkdir(this->m_options->temp_directory().c_str(),0755);

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

                        inet_ntop(AF_INET,&their_addr,peer_char_v4,INET_ADDRSTRLEN);
                        this->m_logger->debug((char*)peer_char_v4);
                        inet_ntop(AF_INET6,&their_addr,peer_char_v6,INET6_ADDRSTRLEN);
                        this->m_logger->debug((char*)peer_char_v6);

                        if ( ! fork() ) {
                            // Following is the actual server response code.
                            _handle_fork(new_fd);
                        }
                        close(new_fd);
                    }
                    /* TODO: 
                        * Implement getpeername so that I can know who connected.
                        * Implement gethostname so that I can know who I am.
                        */
                }
            protected:
                void _handle_fork(int new_fd) {
                    chess::server::secondary *new_controller = 
                        new chess::server::secondary(new_fd,m_sockfd,m_logger,m_options);
                    new_controller->start();
                    delete new_controller;
                }

                chess::server::options *m_options;
                chess::server::logger *m_logger;
                chess::server::notifier *m_notifier;

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
