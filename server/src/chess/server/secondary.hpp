#ifndef CHESS_SERVER_SECONDARY_VERSION
#define CHESS_SERVER_SECONDARY_VERSION 0.1

#include <chess/server/logger.hpp>
#include <chess/server/notifier.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <mqueue.h>
#include <errno.h>

#include <string>
#include <map>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/tokenizer.hpp>

namespace chess {
    namespace server {
        class secondary;

        class player {
            public:
                player(std::string username) {
                    m_name = username;
                    m_login_time = boost::posix_time::second_clock::local_time();
                }
                ~player() {
                }

                std::string name() {
                    return this->m_name;
                }
            protected:
                std::string m_name;
                boost::posix_time::ptime m_login_time;
        };

        class client_state {
            public:
                client_state() {
                }
                ~client_state() {
                }

                void set(std::string v1,std::string v2) {
                    m_status[v1] = v2;
                }
            protected:
                std::map<std::string,std::string> m_status;
        };

        class secondary {
            public:
                secondary(int file_descriptor
                    , int main_descriptor
                    , chess::server::logger* logger
                    , chess::server::options* options):
                        m_file_desc(file_descriptor),
                        m_main_desc(main_descriptor),
                        m_options(options),
                        m_logger(logger) {
                    m_buf = NULL;
                    m_commands = NULL;
                    m_player = NULL;
                }
                ~secondary() {
                    if ( m_buf != NULL ) {
                        delete m_buf;
                    }
                    if ( m_commands != NULL ) {
                        delete m_commands;
                    }
                    close(m_file_desc);
                }

                void start() {
                    if ( ! fork() ) {
                        this->m_logger->debug("Started child process.");
                        // the only reason we pass this.
                        close(m_main_desc);

                        m_state = new chess::server::client_state();

                        m_buf = new char[DATA_BUFFER_SIZE];
                        this->_setup_message_queue();

                        this->_login();

                        std::string queuename = NOTIFIER_MESSAGE_QUEUE_NAME + this->m_player->name();
                        this->m_logger->debug("Creating queue: " + queuename);
                        errno = 0;
                        m_mq_attr.mq_flags = 0;
                        m_mq_attr.mq_maxmsg = 10;
                        m_mq_attr.mq_msgsize = NOTIFIER_MESSAGE_SIZE;
                        errno = 0;
                        m_my_queue = mq_open(queuename.c_str(),
                            O_RDWR | O_CREAT ,
                            0644 ,
                            &m_mq_attr );
                        if ( m_my_queue == -1 ) {
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
                        }
                        FD_ZERO(&m_master);
                        FD_SET(m_file_desc,&m_master);
                        FD_SET(m_my_queue,&m_master);

                        this->m_logger->debug("Finished queue: " + queuename);

                        // TODO: Add polling rate options load.
                        m_tv.tv_sec=0;
                        m_tv.tv_usec=1000;

                        this->_command_setup();
                        this->_main_loop();
                    }
                }
            protected:
                void _login() {
                    while ( m_player == NULL ) {
                        this->_send("Login --> ");
                        int read_amount = recv(m_file_desc,m_buf,DATA_BUFFER_SIZE,0);
                        std::string username = m_buf;
                        _chomp(username);
                        if ( username.length() ) {
                            this->_send("Logging you in as \"" + username + "\";\n");
                        }
                        m_player = new player(username);
                        this->_enqueue("<login>:" + username);
                    }
                }

                void _main_loop() {
                    this->m_logger->log("Starting main_loop for " + this->m_player->name());
                    fd_set readfds;
                    int select_desc = m_file_desc;
                    if ( m_my_queue > m_file_desc ) {
                        select_desc = m_my_queue;
                    }
                    while(1) {
                        readfds = m_master;
                        m_tv.tv_sec = 0;
                        m_tv.tv_usec = 500000;
                        select(select_desc+1,&readfds,NULL,NULL,&m_tv);
                        if ( FD_ISSET(m_file_desc,&readfds) ) {
                            this->m_logger->log("Reading from main socket.");
                            memset(m_buf,0,DATA_BUFFER_SIZE);
                            int read_amount = recv(m_file_desc,m_buf,DATA_BUFFER_SIZE,0);
                            std::string buf_as_string(m_buf);
                            _chomp(buf_as_string);
                            if ( ! read_amount ) {
                                this->_handle_command(buf_as_string,read_amount);
                            }
                            while ( buf_as_string.length() ) {
                                int found = buf_as_string.find("\n");
                                if ( found != std::string::npos ) {
                                    this->_handle_command(buf_as_string.substr(0,found),read_amount);
                                    buf_as_string = buf_as_string.substr(found+1);
                                    this->m_logger->debug("Buffer as string: " + buf_as_string);
                                } else {
                                    this->_handle_command(buf_as_string,read_amount);
                                    buf_as_string = "";
                                }
                            }
                        } else if ( FD_ISSET(m_my_queue,&readfds) ) {
                            this->m_logger->debug("Reading from fifo");
                            struct mq_attr newattr;
                            mq_getattr(m_my_queue,&newattr);
                            this->m_logger->debug("I have " + boost::lexical_cast<std::string>(newattr.mq_curmsgs) + " new messages");
                            for ( int i = 0 ; i < newattr.mq_curmsgs ; i ++ ) {
                                memset(m_buf,0,DATA_BUFFER_SIZE);
                                mq_receive(m_my_queue,m_buf,DATA_BUFFER_SIZE,0);
                                std::string buf_as_string(m_buf);
                                this->_send("\n" + buf_as_string + "\n");
                            }
                            this->_send_command_prompt();
                        }
                    }
                }

                void _handle_command(std::string command,int length) {
                    this->m_logger->debug("Handling command: " + command);
                    if ( ! length ) {
                        this->_shutdown();
                    }
                    std::string true_command = "";
                    std::vector<std::string> args;
                    boost::tokenizer<> tok(command);
                    for(boost::tokenizer<>::iterator beg=tok.begin(); beg!=tok.end();++beg){
                        if ( ! true_command.length() ) {
                            true_command = *beg;
                        } else {
                            args.push_back(*beg);
                        }
                    }
                    this->m_logger->debug("Got true command: " + true_command);
                    if ( m_commands->count(true_command) ) {
                        this->m_logger->log("Attempting to run command: " + command);
                        (*(*m_commands)[true_command])(this,&args);
                        this->_send_command_prompt();
                    } else {
                        this->m_logger->log("Sending to notification queue: " + command);
                        std::string send = this->m_player->name() + ":" + command;
                        mq_send(m_queue,send.c_str(),send.length(),0);
                    }
                }

                void _setup_message_queue() {
                    m_queue = mq_open(NOTIFIER_MESSAGE_QUEUE_NAME,O_WRONLY | O_NONBLOCK);
                }

                void _enqueue (std::string data){
                    mq_send(m_queue,data.c_str(),data.length(),0);
                }

                void _send_command_prompt() {
                    std::string prompt = this->m_options->command_prompt();
                    prompt += "-ics% ";
                    this->_send(prompt);
                }

                void _send(std::string data) {
                    int data_length = data.length();
                    this->m_logger->debug("Sending: " + data);
                    data = "\n" + data;
                    while ( data_length ) {
                        data_length -= send(m_file_desc,
                            data.substr(data.length()-data_length).c_str(),
                            data_length
                            ,0);
                    }
                }

                void _shutdown() {
                    this->m_logger->debug("Closing socket connection");
                    close(m_file_desc);
                    exit(0);
                }

                void _command_setup() {
                    m_commands = new std::map<std::string,void(*)(chess::server::secondary*,std::vector<std::string>*)>;
                    (*m_commands)["quit"] = &_command_sm_quit;
                    (*m_commands)["whoami"] = &_command_sm_whoami;
                    (*m_commands)["iset"] = &_command_set;
                    (*m_commands)["set"] = &_command_set;
                    (*m_commands)["alias"] = &_command_sm_void;
                    (*m_commands)["style"] = &_command_set_style;
                }

                chess::server::logger *m_logger;
                chess::server::options *m_options;
                chess::server::client_state *m_state;
                chess::server::player *m_player;

                int m_file_desc;
                int m_main_desc;
                fd_set m_master;
                struct timeval m_tv;
                char *m_buf;
                mqd_t m_my_queue;
                int m_queue;
                struct mq_attr m_mq_attr;

                std::map <std::string,
                    void (*)(chess::server::secondary*,std::vector<std::string>*)> 
                    *m_commands;

                static void _command_sm_void(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args) {
                }

                static void _chomp(std::string &str) {
                    std::string sm_whitespaces = " \t\f\v\n\r";
                    str.erase(str.find_last_not_of(sm_whitespaces)+1);
                }

                static void _command_sm_quit(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args) {
                    current->_shutdown();
                }

                static void _command_set_style(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args) {
                        if ( args->size() > 0 ) {
                            std::string val = "";
                            for ( int i = 0 ; i < args->size() ; i ++ ) {
                                if ( i > 0 ) {
                                    val += " ";
                                }
                                val += (*args)[i];
                            }
                            current->m_state->set("style",val);
                        }
                }

                static void _command_set(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args) {
                        if ( args->size() == 1 ) {
                            current->m_state->set((*args)[0],"1");
                        } else if ( args->size() > 0 ) {
                            std::string val = "";
                            for ( int i = 1 ; i < args->size() ; i ++ ) {
                                if ( i > 1 ) {
                                    val += " ";
                                }
                                val += (*args)[i];
                            }
                            current->m_state->set((*args)[0],val);
                        }
                }

                static void _command_sm_whoami(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args) {
                    if ( current->m_player != NULL ) {
                        current->_send(current->m_player->name() + "\n");
                    } else {
                        current->_send("I don't know who you are.\n");
                    }
                }
        };
    }
}

#endif
