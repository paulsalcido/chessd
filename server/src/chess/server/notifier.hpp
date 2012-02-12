#ifndef CHESS_SERVER_NOTIFIER_VERSION
#define CHESS_SERVER_NOTIFIER_VERSION 0.1

#ifndef NOTIFIER_MESSAGE_SIZE
#define NOTIFIER_MESSAGE_SIZE 1000
#endif

#ifndef NOTIFIER_MESSAGE_QUEUE_NAME
#define NOTIFIER_MESSAGE_QUEUE_NAME "/shorty"
#endif

/*******************************************************
 * chess::server::notifier
 *
 * This class is the main notification engine, which
 * will be used for chat information and what not.  All
 * of the chess games will be run in a 'master fork', if
 * you will, and will have it's own notifier for
 * informing individuals watching the game as well.
 ******************************************************/

#include <chess/server/logger.hpp>
#include <chess/server/options.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <mqueue.h>
#include <stdlib.h>
#include <errno.h>

#include <boost/lexical_cast.hpp>

namespace chess {
    namespace server {
        class comm {
            public:
                comm(std::string player_name,int queue):
                    m_queue(queue),
                    m_player_name(player_name) {
                }
                ~comm() {
                }

                std::string player_name() {
                    return m_player_name;
                }

                int queue() {
                    return m_queue;
                }
            protected:
                mqd_t m_queue;
                std::string m_player_name;
        };

        class notifier {
            public:
                notifier(int main_desc,chess::server::logger *logger,chess::server::options *options):
                    m_logger(logger),
                    m_main_desc(main_desc),
                    m_options(options) {
                }
                ~notifier() {
                }

                void start() {
                    m_mq_attr.mq_maxmsg = 10;
                    m_mq_attr.mq_msgsize = NOTIFIER_MESSAGE_SIZE;
                    m_mq_attr.mq_flags = 0;
                    errno = 0;
                    std::string test(NOTIFIER_MESSAGE_QUEUE_NAME);
                    this->m_logger->debug("Message: " + test + ", " +
                        boost::lexical_cast<std::string>(m_mq_attr.mq_maxmsg) + ", " +
                        boost::lexical_cast<std::string>(m_mq_attr.mq_msgsize) + ", " +
                        boost::lexical_cast<std::string>(m_mq_attr.mq_flags));
                    m_main_mq_desc = mq_open(
                        NOTIFIER_MESSAGE_QUEUE_NAME ,
                        O_RDWR | O_CREAT ,
                        0644 ,
                        &m_mq_attr );
                    if ( m_main_mq_desc == -1 ) {
                        this->m_logger->debug("Notifier failed with error.");
                        perror("mq_open");
                        switch ( errno ) {
                            case EEXIST: this->m_logger->debug("EEXIST"); break;
                            case EACCES: this->m_logger->debug("EACCES"); break;
                            case EINVAL: this->m_logger->debug("EINVAL"); break;
                            case EMFILE: this->m_logger->debug("EMFILE"); break;
                            case ENAMETOOLONG: this->m_logger->debug("ENAMETOOLONG"); break;
                            case ENFILE: this->m_logger->debug("ENFILE"); break;
                            case ENOENT: this->m_logger->debug("ENOENT"); break;
                            case ENOMEM: this->m_logger->debug("ENOMEM"); break;
                            case ENOSPC: this->m_logger->debug("ENOSPC"); break;
                        }
                        exit(errno);
                    } else if ( ! fork() ) {
                        close(m_main_desc);
                        // TODO: More command line options?
                        // This should be a blocking queue, hopefully it is.
                        this->_main_loop();
                    }
                }
            protected:

                void _main_loop() {
                    struct mq_attr old_attr;
                    unsigned int prio;
                    m_buf = new char[NOTIFIER_MESSAGE_SIZE];
                    while ( 1 ) {
                        memset(m_buf,0,NOTIFIER_MESSAGE_SIZE);
                        m_mq_attr.mq_flags = 0;
                        mq_setattr(m_main_mq_desc,&m_mq_attr,&old_attr);
                        int test = mq_receive(m_main_mq_desc,
                            &m_buf[0],NOTIFIER_MESSAGE_SIZE,&prio);
                        std::string str_buf(m_buf);
                        m_logger->debug("Notifier caught: " + str_buf + " " +
                            boost::lexical_cast<std::string>(test));
                        this->handle(str_buf);
                    }
                }
                
                void handle(std::string buf) {
                    std::vector<std::string> notification;
                    std::string obuf = buf;
                    this->m_logger->debug("Handling " + buf);
                    while ( buf.length() ) {
                        int found = buf.find(":");
                        if ( found != std::string::npos) {
                            notification.push_back(buf.substr(0,found));
                            this->m_logger->debug("Pushing: " + buf.substr(0,found) + "; buf now: " + buf.substr(found+1));
                            buf = buf.substr(found+1);
                        } else {
                            notification.push_back(buf);
                            this->m_logger->debug("Pushing: " + buf);
                            buf = "";
                        }
                    }
                    this->m_logger->debug("Trying on: " + notification[0]);
                    if ( ! notification[0].compare("<login>") ) {
                        // Open a queue that is based on the name.
                        // TODO: Create add player and remove player functions.
                        std::string queuename = NOTIFIER_MESSAGE_QUEUE_NAME + notification[1];
                        this->m_logger->debug("Notifier building child queue: " + queuename);
                        mqd_t mq = mq_open(queuename.c_str(),
                            O_WRONLY | O_CREAT ,
                            0644 ,
                            &m_mq_attr );
                        if ( mq == -1 ) {
                            perror("mq_open");
                            switch ( errno ) {
                                case EEXIST: this->m_logger->debug("EEXIST"); break;
                                case EACCES: this->m_logger->debug("EACCES"); break;
                                case EINVAL: this->m_logger->debug("EINVAL"); break;
                                case EMFILE: this->m_logger->debug("EMFILE"); break;
                                case ENAMETOOLONG: this->m_logger->debug("ENAMETOOLONG"); break;
                                case ENFILE: this->m_logger->debug("ENFILE"); break;
                                case ENOENT: this->m_logger->debug("ENOENT"); break;
                                case ENOMEM: this->m_logger->debug("ENOMEM"); break;
                                case ENOSPC: this->m_logger->debug("ENOSPC"); break;
                            }
                        }
                        m_comms[notification[1]] = new chess::server::comm(notification[1],mq);
                        std::string login_buffer = notification[1] + " has logged in.";
                        this->_send_all(login_buffer);
                    } else {
                        this->_send_all(obuf);
                    }
                }

                void _send_all(std::string buffer) {
                    for ( std::map<std::string,chess::server::comm*>::iterator it = m_comms.begin();
                            it != m_comms.end() ; it ++ ) {
                        this->m_logger->debug("Sending data over queue: " + buffer);
                        mq_send(it->second->queue(),buffer.c_str(),buffer.length(),0);
                    }
                }

                chess::server::logger *m_logger;
                chess::server::options *m_options;
                std::map<std::string,chess::server::comm*> m_comms;
                struct mq_attr m_mq_attr;
                int m_main_desc;
                mqd_t m_main_mq_desc;
                char* m_buf;

                key_t m_ipckey;
                int mq_id;

                static void _sig_handler(int signum) {
                }
        };
    }
}

#endif
