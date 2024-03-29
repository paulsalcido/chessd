#ifndef CHESS_SERVER_SECONDARY_VERSION
#define CHESS_SERVER_SECONDARY_VERSION 0.1

#include <chess/server/logger.hpp>
#include <chess/server/notifier.hpp>
#include <chess/storage/postgres.hpp>
#include <chess/game.hpp>

#include <chess/game/standard_chess.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <time.h>

#include <mqueue.h>
#include <errno.h>

#include <string>
#include <map>
#include <vector>
#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/tokenizer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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

        class challenge {
            public:
                challenge(std::string player,
                        std::string challenged,
                        int increment,
                        int starting_time,
                        std::string game = "standard") : 
                    m_player(player),
                    m_challenged(challenged),
                    m_increment(increment),
                    m_starting_time(starting_time),
                    m_game(game){
                }
                ~challenge() {
                }

                int increment(int val) {
                    return m_increment = val;
                }
                int increment() {
                    return m_increment;
                }
                
                int starting_time(int val) {
                    return m_starting_time = val;
                }
                int starting_time() {
                    return m_starting_time;
                }

                std::string player(std::string val) {
                    return m_player = val;
                }
                std::string player() {
                    return m_player;
                }

                std::string challenged(std::string val) {
                    return m_challenged = val;
                }
                std::string challenged() {
                    return m_challenged;
                }

                std::string game(std::string val) {
                    return m_game = val;
                }
                std::string game() {
                    return m_game;
                }

            protected:
                std::string m_game;
                std::string m_player;
                std::string m_challenged;
                int m_increment;
                int m_starting_time;
        };

        class draw_request {
            public:
                draw_request(std::string player) {
                    m_player = player;
                }
                ~draw_request() {
                }

                std::string player() {
                    return m_player;
                }
            protected:
                std::string m_player;
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
                    m_actions = NULL;
                    m_player = NULL;
                    m_last_challenge = NULL;
                    m_game = NULL;
                    m_needs_prompt = false;
                    m_draw_request = NULL;
                    m_storage = NULL;
                    m_storage_player = NULL;
                }
                ~secondary() {
                    this->_shutdown();
                    if ( m_buf != NULL ) {
                        delete m_buf;
                    }
                    if ( m_storage_player != NULL ) {
                        delete m_storage_player;
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
                        /* TODO, make this pluginnable... */
                        std::map<std::string,std::string> lv;
                        lv["host"] = this->m_options->storage_host();
                        lv["port"] = this->m_options->storage_port();
                        lv["user"] = this->m_options->storage_user();
                        lv["password"] = this->m_options->storage_password();
                        m_storage = new chess::storage::postgres();
                        bool success = m_storage->connect(lv);
                        if ( ! success ) {
                            this->m_logger->error("Couldn't connect to storage container.");
                            exit(1);
                        }
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
                std::string player_name() {
                    std::string retval = "";
                    if ( this->m_player != NULL ) {
                        retval = this->m_player->name();
                    }
                    return retval;
                }

                void _login() {
                    struct timeval tmptv;
                    fd_set tmpfdset;
                    FD_ZERO(&tmpfdset);
                    FD_SET(m_file_desc,&tmpfdset);
                    boost::property_tree::ptree tester;
                    while ( m_player == NULL ) {
                        std::string username = "";
                        this->_send("Trying to login to psalcido-ics\n\n");
                        this->_send("login: ");
                        int read_amount = 0;

                        while ( ! username.length() ) {
                            memset(m_buf,0,DATA_BUFFER_SIZE);
                            read_amount = recv(m_file_desc,m_buf,DATA_BUFFER_SIZE,0);
                            if ( read_amount == 0 ) { 
                                this->_shutdown();
                            }
                            if ( m_buf[0] != '\xff' ) {
                                username = m_buf;
                                _chomp(username);
                                break;
                            } else {
                                tester.put<std::string>("value",m_buf);
                                this->m_logger->debug(chess::ptree::ptree_as_string(tester));
                            }
                        }

                        this->_send("\nYou are trying to login as \""+username+"\"\nIf this is you, try your password, otherwise, just press return.\n\npassword: ");
                        this->_send("\n\xff\xfb\x01");

                        std::string password = "";

                        while ( ! password.length() ) {
                            memset(m_buf,0,DATA_BUFFER_SIZE);
                            read_amount = recv(m_file_desc,m_buf,DATA_BUFFER_SIZE,0);
                            if ( read_amount == 0 ) { 
                                this->_shutdown();
                            }
                            if ( m_buf[0] != '\xff' ) {
                                password = m_buf;
                                _chomp(password);
                                break;
                            } else {
                                tester.put<std::string>("value",m_buf);
                                this->m_logger->debug(chess::ptree::ptree_as_string(tester));
                            }
                        }

                        this->_send("\xff\xfc\x01");
                        if ( username.length() && password.length() ) {
                            //this->_send("Logging you in as \"" + username + "\";\n");
                            this->m_logger->debug(username + ", " + password);
                            m_storage_player = m_storage->login(username,password);
                            if ( m_storage_player != NULL ) {
                                m_player = new chess::server::player(m_storage_player->short_name());
                                this->_send("\nLogging you in as \"" + username + "\";\n\n");
                                this->_send_login_notification();
                            } else {
                                this->_send("Could not log you in as " + username + "\n\n");
                            }
                        } else {
                            this->_send("Not enough information to log you in.\n\n");
                        }
                    }
                }
/*
     void echo_off(void)
     {
         struct termios new_settings;
         tcgetattr(0,&stored_settings);
         new_settings = stored_settings;
         new_settings.c_lflag &= (~ECHO);
         tcsetattr(0,TCSANOW,&new_settings);
         return;
     }
     
     void echo_on(void)
     {
         tcsetattr(0,TCSANOW,&stored_settings);
         return;
     }

 */

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
                            this->m_needs_prompt = true;
                            this->_send_command_prompt();
                        } else if ( FD_ISSET(m_my_queue,&readfds) ) {
                            // All messages from fifo must now be handled as json.
                            struct mq_attr newattr;
                            mq_getattr(m_my_queue,&newattr);
                            this->m_logger->debug("<" + this->m_player->name() + "> I have " + boost::lexical_cast<std::string>(newattr.mq_curmsgs) + " new messages");
                            for ( int i = 0 ; i < newattr.mq_curmsgs ; i ++ ) {
                                memset(m_buf,0,DATA_BUFFER_SIZE);
                                mq_receive(m_my_queue,m_buf,DATA_BUFFER_SIZE,0);
                                std::string buf_as_string(m_buf);
                                boost::property_tree::ptree pt;
                                chess::ptree::fill_ptree(buf_as_string,pt);
                                if ( pt.count("action") ) {
                                    this->m_logger->debug("<" + this->m_player->name() + "> action: " + pt.get<std::string>("action"));
                                    if ( m_actions->count(pt.get<std::string>("action")) ) {
                                        (*(*m_actions)[pt.get<std::string>("action")])(this,pt);
                                        this->_send_command_prompt();
                                    }
                                }
                            }
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
                        std::string obuf = true_command;
                        if ( command.length() > true_command.length() ) {
                            obuf = command.substr(true_command.length()+1);
                        }
                        (*(*m_commands)[true_command])(this,&args,obuf);
                        //this->_send_command_prompt();
                    } else {
                        if ( this->m_game != NULL && this->m_game->test_move(this->player_name(),command) ) {
                            this->_send(this->m_game->status(this->player_name(),"12") + "\n");
                            /* TODO: Put this in function */
                            boost::property_tree::ptree pt;
                            pt.put("player",this->m_player->name());
                            pt.put("action","game-refresh");
                            pt.put("game-id",this->m_game->game_number());
                            pt.put("move",command);
                            this->_enqueue(chess::ptree::ptree_as_string(pt));
                            if ( this->m_draw_request != NULL ) {
                                delete this->m_draw_request;
                                this->m_draw_request = NULL;
                            }
                        } else {
                            this->m_logger->log("Sending to notification queue: " + command);
                            boost::property_tree::ptree pt;
                            pt.put("message",command);
                            pt.put("action","unknown");
                            pt.put("player",this->m_player->name());
                            this->_enqueue(chess::ptree::ptree_as_string(pt));
                        }
                    }
                }

                void _send_login_notification() {
                    boost::property_tree::ptree pt;
                    pt.put("player",this->m_player->name());
                    pt.put("action","login");
                    this->_enqueue(chess::ptree::ptree_as_string(pt));
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
                    if ( this->m_needs_prompt ) {
                        this->m_needs_prompt = false;
                        this->_send(prompt,true);
                    }
                }

                void _send(std::string data,bool ignore_prompt = false) {
                    int data_length = data.length();
                    if ( this->m_player != NULL ) { 
                        this->m_logger->debug("Sending <" + this->m_player->name() + ">: " + data);
                    } else {
                        this->m_logger->debug("Sending: " + data);
                    }
                    data = "\n" + data;
                    if ( data_length && ! ignore_prompt ) {
                        this->m_needs_prompt = true;
                    }
                    while ( data_length ) {
                        data_length -= send(m_file_desc,
                            data.substr(data.length()-data_length).c_str(),
                            data_length
                            ,0);
                    }
                }

                void _shutdown() {
                    this->m_logger->debug("Closing socket connection");
                    if ( this->m_player != NULL ) {
                        boost::property_tree::ptree pt;
                        pt.put("player",this->m_player->name());
                        pt.put("action","logout");
                        this->_enqueue(chess::ptree::ptree_as_string(pt));
                        delete this->m_player;
                    }
                    if ( this->m_storage != NULL ) {
                        this->m_storage->disconnect();
                        delete this->m_storage;
                        m_storage = NULL;
                    }
                    close(m_file_desc);
                    exit(0);
                }

                void _command_setup() {
                    m_commands = new std::map<std::string,void(*)(chess::server::secondary*,std::vector<std::string>*,std::string)>;
                    (*m_commands)["quit"] = &_command_sm_quit;
                    (*m_commands)["whoami"] = &_command_sm_whoami;
                    (*m_commands)["iset"] = &_command_set;
                    (*m_commands)["set"] = &_command_set;
                    (*m_commands)["alias"] = &_command_sm_void;
                    (*m_commands)["style"] = &_command_set_style;
                    (*m_commands)["match"] = &_command_sm_match;
                    (*m_commands)["say"] = &_command_sm_say;
                    (*m_commands)["shout"] = &_command_sm_shout;
                    (*m_commands)["accept"] = &_command_sm_accept;
                    (*m_commands)["decline"] = &_command_sm_decline;
                    (*m_commands)["cancel"] = &_command_sm_cancel;
                    (*m_commands)["resign"] = &_command_sm_resign;
                    (*m_commands)["status"] = &_command_sm_status;
                    (*m_commands)["examine"] = &_command_sm_examine;
                    (*m_commands)["draw"] = &_command_sm_draw;

                    m_actions = new std::map<std::string,void (*)(chess::server::secondary*, const boost::property_tree::ptree &)>;
                    (*m_actions)["match"] = &_handle_action_match;
                    (*m_actions)["say"] = &_handle_action_say;
                    (*m_actions)["shout"] = &_handle_action_say;
                    (*m_actions)["cancel"] = &_handle_action_cancel;
                    (*m_actions)["decline"] = &_handle_action_decline;
                    (*m_actions)["accept"] = &_handle_action_accept;
                    (*m_actions)["resign"] = &_handle_action_resign;
                    (*m_actions)["logout"] = &_handle_action_logout;
                    (*m_actions)["game-refresh"] = &_handle_action_game_refresh;
                    (*m_actions)["examine"] = &_handle_action_examine;
                    (*m_actions)["accept-examine"] = &_handle_action_accept_examine;
                    (*m_actions)["draw-accept"] = &_handle_action_draw_accept;
                    (*m_actions)["draw-offer"] = &_handle_action_draw_request;
                }

                chess::server::logger *m_logger;
                chess::server::options *m_options;
                chess::server::client_state *m_state;
                chess::storage::base* m_storage;
                chess::storage::player *m_storage_player;
                chess::server::player *m_player;
                chess::server::challenge *m_last_challenge;
                chess::server::draw_request *m_draw_request;
                chess::game *m_game;

                int m_file_desc;
                int m_main_desc;
                fd_set m_master;
                struct timeval m_tv;
                char *m_buf;
                mqd_t m_my_queue;
                int m_queue;
                struct mq_attr m_mq_attr;
                bool m_needs_prompt;

                std::map <std::string,
                    void (*)(chess::server::secondary*,std::vector<std::string>*,std::string)> 
                    *m_commands;

                std::map <std::string,
                    void (*)(chess::server::secondary*,const boost::property_tree::ptree &)>
                    *m_actions;

                /* All commands start below this point. */

                static void _command_sm_void(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                }

                static void _command_sm_draw(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_draw_request == NULL && current->m_game != NULL ) {
                        boost::property_tree::ptree pt;
                        pt.put("action","draw-request");
                        pt.put("player",current->player_name());
                        pt.put("recipient",current->m_game->opponent());
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                        current->_send("You have offered draw to your opponent.");
                    } else if ( current->m_draw_request != NULL && current->m_draw_request->player().compare(current->player_name()) ) {
                        boost::property_tree::ptree pt;
                        pt.put("action","draw-accept");
                        pt.put("player",current->player_name());
                        pt.put("recipient",current->m_game->opponent());
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                        current->_send("You have accepted the draw request.");
                    }
                }

                static void _chomp(std::string &str) {
                    std::string sm_whitespaces = " \t\f\v\n\r";
                    str.erase(str.find_last_not_of(sm_whitespaces)+1);
                }

                static void _command_sm_quit(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    current->_shutdown();
                }

                static void _command_set_style(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
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
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
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

                static void _command_sm_accept(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_player != NULL && current->m_last_challenge != NULL &&
                            current->m_last_challenge->player().compare(current->m_player->name()) &&
                            current->m_game == NULL ) {
                        boost::property_tree::ptree pt;
                        int game_id = time(NULL);
                        current->m_game = new chess::games::standard_chess();
                        current->m_game->opponent(current->m_last_challenge->player());
                        current->m_game->owner(current->m_player->name());
                        pt.put("recipient",current->m_last_challenge->player());
                        pt.put("player",current->m_player->name());
                        pt.put("action","accept");
                        pt.put("starting-time",boost::lexical_cast<std::string>(current->m_last_challenge->starting_time()));
                        pt.put("increment",boost::lexical_cast<std::string>(current->m_last_challenge->increment()));
                        pt.put("game-type",current->m_last_challenge->game());
                        pt.put("owner",current->m_player->name());
                        pt.put("game-id",game_id);
                        pt.put("black",current->m_game->opponent());
                        pt.put("white",current->m_game->owner());
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                        current->_send("You have attempted to accept the game (id: "+boost::lexical_cast<std::string>(game_id)+") from " + current->m_last_challenge->player() + "\n");
                        boost::property_tree::ptree gpt;
                        gpt.put("white",current->m_game->owner());
                        gpt.put("black",current->m_game->opponent());
                        gpt.put("game-id",time(NULL));
                        gpt.put("starting-time",current->m_last_challenge->starting_time());
                        gpt.put("increment",current->m_last_challenge->increment());
                        current->m_game->setup(gpt);
                        current->m_logger->debug("After setting up the chess game.");
                        boost::property_tree::ptree pt2;
                        pt2.put<std::string>("action","game-refresh");
                        pt2.put<int>("game-id",current->m_game->game_number());
                        pt2.put<std::string>("player",current->m_player->name());
                        current->_enqueue(chess::ptree::ptree_as_string(pt2));
                        current->_send(current->m_game->status(current->m_player->name(),"12") + "\n");
                        delete current->m_last_challenge;
                        current->m_last_challenge = NULL;
                    } else if ( current->m_game == NULL ) {
                        current->_send("Error: You do not have a match to accept.\n");
                    } else {
                        current->_send("Error: You already have an active game.\n");
                    }
                }

                static void _command_sm_cancel (
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_player != NULL && current->m_last_challenge != NULL 
                            && ! current->m_last_challenge->player().compare(current->m_player->name()) ) {
                        boost::property_tree::ptree pt;
                        pt.put<std::string>("recipient",current->m_last_challenge->challenged());
                        pt.put<std::string>("player",current->m_player->name());
                        pt.put<std::string>("action","cancel");
                        delete current->m_last_challenge;
                        current->m_last_challenge = NULL;
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                        current->_send("You have cancelled your match request.\n");
                    } else {
                        current->_send("Error: You do not have a challenge to cancel. Perhaps decline?\n");
                    }
                }

                static void _command_sm_resign (
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_player != NULL && current->m_game != NULL &&
                            ( ! current->player_name().compare(current->m_game->get_attribute("white")) ||
                            ! current->player_name().compare(current->m_game->get_attribute("black")) ) ) {
                        boost::property_tree::ptree pt;
                        pt.put<std::string>("opponent",current->m_game->opponent());
                        pt.put<std::string>("player",current->m_player->name());
                        pt.put<std::string>("action","resign");
                        pt.put<std::string>("game-id",boost::lexical_cast<std::string>(current->m_game->game_number()));
                        pt.put<std::string>("black",((chess::games::standard_chess*)(current->m_game))->black());
                        pt.put<std::string>("white",((chess::games::standard_chess*)(current->m_game))->white());
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                    } else {
                        current->_send("Error: You do not have a game to resign.");
                    }
                }

                static void _command_sm_decline (
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_player != NULL && current->m_last_challenge != NULL 
                            && current->m_last_challenge->player().compare(current->m_player->name()) ) {
                        boost::property_tree::ptree pt;
                        pt.put<std::string>("recipient",current->m_last_challenge->player());
                        pt.put<std::string>("player",current->m_player->name());
                        pt.put<std::string>("action","decline");
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                        delete current->m_last_challenge;
                        current->m_last_challenge = NULL;
                        current->_send("You have declined the last challenge.\n");
                    } else {
                        current->_send("Error: You do not have a challenge to decline. Perhaps cancel?\n");
                    }
                }

                static void _command_sm_match(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_player != NULL ) {
                    /* TODO: Much better error detection here. */
                        if ( args->size() >= 3 ) {
                            boost::property_tree::ptree pt;
                            pt.put("recipient",(*args)[0]);
                            pt.put("game-type",(*args)[1]);
                            pt.put("starting-time",(*args)[2]);
                            pt.put("increment",(*args)[3]);
                            pt.put("player",current->m_player->name());
                            pt.put("action","match");
                            current->_enqueue(chess::ptree::ptree_as_string(pt));
                        } else {
                            current->_send("could not create match:\n   Usage: match <player> <starting-time (seconds)> <increment> (seconds)");
                        }
                    }
                }

                static void _command_sm_status(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args 
                        ,std::string obuf) {
                    if ( current->m_player != NULL ) {
                        current->_send("Player: " + current->m_player->name() + "\n");
                    }
                    if ( current->m_last_challenge != NULL )  {
                        current->_send("Last challenge: " + current->m_last_challenge->player() + " challenged " + current->m_last_challenge->challenged() + "\n");
                    }
                    if ( current->m_game != NULL ) {
                        current->_send("Game: owner: " + current->m_game->owner() + ", " + current->m_game->opponent() + "\n");
                    }
                };

                static void _command_sm_shout(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args 
                        ,std::string obuf) {
                    if ( current->m_player != NULL ) {
                        boost::property_tree::ptree pt;
                        pt.put("action","shout");
                        pt.put("player",current->m_player->name());
                        pt.put("message",obuf);
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                    }
                };

                static void _command_sm_examine(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args 
                        ,std::string obuf) {
                    if ( current->m_player != NULL && args->size() > 0 ) {
                        boost::property_tree::ptree pt;
                        pt.put("action","examine");
                        pt.put("player",current->m_player->name());
                        pt.put("game-id",(*args)[0]);
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                    } else {
                        current->_send("Error: usage: examine <game-id>");
                    }
                };

                static void _command_sm_say(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args 
                        ,std::string obuf) {
                    if ( current->m_player != NULL ) {
                        boost::property_tree::ptree pt;
                        pt.put("action","say");
                        pt.put("player",current->m_player->name());
                        pt.put("message",obuf);
                        current->_enqueue(chess::ptree::ptree_as_string(pt));
                    }
                };

                static void _command_sm_whoami(
                        chess::server::secondary* current
                        ,std::vector<std::string>* args
                        ,std::string obuf) {
                    if ( current->m_player != NULL ) {
                        current->_send(current->m_player->name() + "\n");
                    } else {
                        current->_send("I don't know who you are.\n");
                    }
                }

                static void _handle_action_match(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_player != NULL ) {
                        /* TODO: Check to see if I have challenged someone else first,
                         * if I have, then turn off the ability to receive challenges? */
                        if ( ! pt.get<std::string>("recipient").compare(current->m_player->name()) ) {
                            std::ostringstream challenge_string;
                            challenge_string << std::endl << "Challenge: " 
                                << pt.get<std::string>("player") << " (++++) "
                                << current->m_player->name() << "(++++) "
                                // Game type...
                                << "unrated standard "  
                                << pt.get<int>("starting-time") << " "
                                << pt.get<int>("increment") << std::endl
                                << "Use the command 'accept' to play this match." << std::endl;
                            current->_send(challenge_string.str());
                            if ( current->m_last_challenge != NULL ) {
                                delete current->m_last_challenge;
                            }
                            current->m_last_challenge = new chess::server::challenge(
                                pt.get<std::string>("player"),
                                pt.get<std::string>("recipient"),
                                pt.get<int>("increment"),
                                pt.get<int>("starting-time"),
                                "standard"
                            );
                        } else if ( ! pt.get<std::string>("player").compare(current->m_player->name()) ) {
                            std::ostringstream challenge_string;
                            challenge_string << "\nYou have challenged "
                                << pt.get<std::string>("recipient") << " (++++) "
                                << " to a match." << std::endl;
                            current->_send(challenge_string.str());
                            if ( current->m_last_challenge != NULL ) {
                                delete current->m_last_challenge;
                            }
                            current->m_last_challenge = new chess::server::challenge(
                                pt.get<std::string>("player"),
                                pt.get<std::string>("recipient"),
                                pt.get<int>("increment"),
                                pt.get<int>("starting-time"),
                                "standard"
                            );
                        }
                    }
                }

                static void _handle_action_cancel(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_player != NULL && current->m_last_challenge != NULL ) {
                        delete current->m_last_challenge;
                        current->m_last_challenge = NULL;
                        current->_send("\nThe challenge by " + pt.get<std::string>("player") + " was cancelled.\n");
                    }
                }

                static void _handle_action_decline(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_player != NULL && current->m_last_challenge != NULL ) {
                        delete current->m_last_challenge;
                        current->m_last_challenge = NULL;
                        current->_send("\nYour challenge has been declined.\n");
                    }
                }

                static void _handle_action_accept(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( pt.count("status") && ! pt.get<std::string>("status").compare("failure") ) {
                        if ( current->m_last_challenge != NULL ) {
                            delete current->m_last_challenge;
                            current->m_last_challenge = NULL;
                        }
                        current->_send("\nYour attempt to accept the game failed.\n");
                    } else if ( current->m_player != NULL && current->m_last_challenge != NULL &&
                        current->m_game == NULL ) {
                        current->m_game = new chess::games::standard_chess();
                        current->m_game->setup(pt);
                        current->m_game->opponent(pt.get<std::string>("player"));
                        current->_send("\nYour game has been accepted.\n");
                    } else if ( current->m_game != NULL ) {
                        /* TODO: figure out why players are sending this to themselves. */
                        boost::property_tree::ptree spt;
                        spt.put("action","failed-accept");
                        spt.put("recipient",pt.get<std::string>("player"));
                        spt.put("player",current->m_player->name());
                        current->_enqueue(chess::ptree::ptree_as_string(spt));
                    }
                }

                static void _handle_action_failed_accept(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree pt ) {
                    if ( current->m_game != NULL ) {
                        delete current->m_game;
                        current->m_game = NULL;
                        current->_send("\nThe game could not be accepted because of an error.\n");
                    }
                }

                static void _handle_action_say(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_player != NULL ) {
                        /*if ( ! pt.get<std::string>("recipient").compare(current->m_player->name()) ) {
                        }*/
                        current->_send(chess::ptree::action_as_send_string(pt));
                    }
                }

                static void _handle_action_resign(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_player != NULL && current->m_game->game_number() == pt.get<int>("game-id") ) {
                        std::string result = "0-1";
                        if ( current->m_game != NULL && ! ((chess::games::standard_chess*)(current->m_game))->black().compare(pt.get<std::string>("player")) ) {
                            result = "1-0";
                        }
                        current->_send("\n{Game " + pt.get<std::string>("game-id") + " (" + pt.get<std::string>("white")  + " vs. " + pt.get<std::string>("black") + ") " + pt.get<std::string>("player") + " resigned.} " + result + "\n");
                        delete current->m_game;
                        current->m_game = NULL;
                    }
                }

                static void _handle_action_logout(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    /* Someone has logged out, is it our opponent? */
                    if ( current->m_player != NULL ) {
                        // Probably couldn't get the message any other way.
                        current->_send("\n" + pt.get<std::string>("player") + " has logged out.\n");
                    }
                    if ( current->m_game != NULL && 
                            ! current->m_game->opponent().compare(pt.get<std::string>("player")) ) {
                        /* Mark as win then delete */
                        current->_send("\nTODO: Mark win due to logout or disconnect.\n");
                        delete current->m_game;
                        current->m_game = NULL;
                    } else if ( current->m_last_challenge != NULL &&
                            !current->m_last_challenge->player().compare(pt.get<std::string>("player")) ) {
                        delete current->m_last_challenge;
                        current->_send("\nLast challenge is closed because of logout or disconnect.\n");
                    }
                }

                static void _handle_action_game_refresh(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_player != NULL && current->m_game != NULL && pt.get<std::string>("player").compare(current->player_name()) ) {
                        if ( pt.get<int>("game-id") == current->m_game->game_number() ) {
                            if ( pt.count("move") ) {
                                current->m_game->test_move(pt.get<std::string>("player"),pt.get<std::string>("move"));
                                if ( current->m_draw_request != NULL ) {
                                    delete current->m_draw_request;
                                    current->m_draw_request = NULL;
                                }
                            } 
                            current->_send(current->m_game->status(current->player_name(),"12") + "\n");
                        }
                    }
                }

                static void _handle_action_accept_examine(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    bool success = false;
                    if ( current->m_game != NULL && 
                        ! pt.get<std::string>("recipient").compare(current->m_player->name()) &&
                        current->m_player->name().compare(((chess::games::standard_chess*)(current->m_game))->black()) &&
                        current->m_player->name().compare(((chess::games::standard_chess*)(current->m_game))->white()) ) {
                        delete current->m_game;
                        current->m_game = NULL;
                        success = true;
                    } else if ( current->m_game == NULL ) {
                        success = true;
                    }
                    if ( success ) {
                        current->m_game = new chess::games::standard_chess();
                        current->m_logger->debug("<" + current->m_player->name() + "> Accepting game examine.");
                        current->m_game->setup(pt);
                        std::istringstream is2(pt.get<std::string>("move-list"));
                        std::string test1;
                        is2 >> test1;
                        int plmod = 1;
                        std::string curplayer;
                        /* This is a quick fix.  TODO: Make better loop. */
                        std::string lastmove;
                        while ( test1.length() && lastmove.compare(test1) ) {
                            if ( plmod % 2 ) {
                                curplayer = ((chess::games::standard_chess*)(current->m_game))->white();
                            } else {
                                curplayer = ((chess::games::standard_chess*)(current->m_game))->black();
                            }
                            plmod++;
                            current->m_game->test_move(curplayer,test1);
                            current->m_logger->debug("<" + current->m_player->name() + "> doing move: " + test1);
                            lastmove = test1;
                            is2 >> test1;
                        }
                        current->_send(current->m_game->status(current->m_player->name(),"12") + "\n");
                    }
                }

                static void _handle_action_examine(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_game != NULL && pt.get<int>("game-id") == current->m_game->game_number() && ! current->m_game->owner().compare(current->m_player->name()) ) {
                        boost::property_tree::ptree spt;
                        spt.put<std::string>("player",current->m_player->name());
                        spt.put<std::string>("action","accept-examine");
                        spt.put<std::string>("recipient",pt.get<std::string>("player"));
                        spt.put<int>("starting-time",((chess::games::standard_chess*)(current->m_game))->starting_time());
                        spt.put<int>("increment",((chess::games::standard_chess*)(current->m_game))->increment());
                        spt.put<int>("game-id",((chess::games::standard_chess*)(current->m_game))->game_number());
                        spt.put<std::string>("move-list",((chess::games::standard_chess*)(current->m_game))->move_list());
                        spt.put<std::string>("black",((chess::games::standard_chess*)(current->m_game))->black());
                        spt.put<std::string>("white",((chess::games::standard_chess*)(current->m_game))->white());
                        current->_enqueue(chess::ptree::ptree_as_string(spt));
                        current->_send("\nThe player " + pt.get<std::string>("player") + " is examining your game.\n");
                    }
                }

                static void _handle_action_draw_request(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_game != NULL ) {
                        if ( current->m_draw_request == NULL ) {
                            current->m_draw_request = new chess::server::draw_request(pt.get<std::string>("player"));
                            current->_send("\nYour opponent has offered draw in this game.\nTo accept, type 'draw'.  To reject, move.\n");
                        }
                    }
                }

                static void _handle_action_draw_accept(
                        chess::server::secondary *current
                        ,const boost::property_tree::ptree &pt) {
                    if ( current->m_game != NULL && current->m_draw_request != NULL ) {
                        current->_send("{Game " + boost::lexical_cast<std::string>(current->m_game->game_number()) 
                            + " (" + current->m_game->get_attribute("white") + " vs. " + current->m_game->get_attribute("black") + ") Game drawn by mutual agreement} 1/2-1/2\n");
                        delete current->m_game;
                        current->m_game = NULL;
                        delete current->m_draw_request;
                        current->m_draw_request = NULL;
                    }
                }

        };
    }
}

#endif
