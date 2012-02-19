#ifndef CHESS_SERVER_OPTIONS_VERSION
#define CHESS_SERVER_OPTIONS_VERSION 0.1

#include <string>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <chess/server/logger.hpp>

namespace chess {
    namespace server {
        class options {
            public:
            options()
            { }

            ~options() { }

            std::string temp_directory(std::string i_temp_directory) {
                return this->m_temp_directory = i_temp_directory;
            }
            std::string temp_directory() {
                return this->m_temp_directory;
            }

            std::string log_directory(std::string i_log_directory) {
                return this->m_log_directory = i_log_directory;
            }
            std::string log_directory() {
                return this->m_log_directory;
            }

            int option_errors(int i_option_errors) {
                return this->m_option_errors = i_option_errors;
            }
            int option_errors() {
                return this->m_option_errors;
            }

            int backlog_limit(int i_backlog_limit) {
                return this->m_backlog_limit = i_backlog_limit;
            }
            int backlog_limit() {
                return this->m_backlog_limit;
            }

            int port(int i_port) {
                return this->m_port = i_port;
            }
            int port() {
                return this->m_port;
            }

            std::string command_prompt(std::string i_command_prompt) {
                return this->m_command_prompt = i_command_prompt;
            }
            std::string command_prompt() {
                return this->m_command_prompt;
            }
            /* TODO: Make these command line options */
            std::string storage_port() {
                return "5432";
            }
            std::string storage_password() {
                return "game_serve";
            }
            std::string storage_user() {
                return "game_serve";
            }
            std::string storage_host() {
                return "localhost";
            }

            int load_arguments(int argc, char **argv, chess::server::logger **logger) {
                boost::program_options::options_description desc("Allowed Options");
                desc.add_options()
                    ("port",boost::program_options::value<int>(),"Port to open for primary connections.")
                    ("command-prompt",boost::program_options::value<std::string>(),"What the command prompt should start with.")
                    ("backlog-limit",boost::program_options::value<int>(),"Backlog limit on main port.")
                    ("log-directory",boost::program_options::value<std::string>(),"Logging directory.")
                    ("temp-directory",boost::program_options::value<std::string>(),"Temp files directory.")
                ;

                int errors = 0;
                boost::program_options::variables_map vm;
                boost::program_options::store(
                    boost::program_options::parse_command_line(argc,argv,desc)
                    ,vm);
                boost::program_options::notify(vm);

                if ( vm.count("log-directory") ) {
                    this->log_directory(vm["log-directory"].as<std::string>());
                }

                if ( vm.count("temp-directory") ) {
                    this->temp_directory(vm["temp-directory"].as<std::string>());
                } else {
                    this->temp_directory("/tmp/open-chess-server/");
                }

                if ( this->log_directory().length() > 0 ) {
                    *logger = new chess::server::logger(this->log_directory());
                } else {
                    *logger = new chess::server::logger();
                }
            
                (*logger)->log("Started logger");
 
                if ( vm.count("backlog-limit") ) {
                    this->backlog_limit(vm["backlog-limit"].as<int>());
                    (*logger)->log("backlog_limit retrieved " + boost::lexical_cast<std::string>(this->backlog_limit()));
                } else {
                    (*logger)->log("Missing option 'backlog-limit', setting to 20");
                    this->backlog_limit(20);
                }
 
                if ( vm.count("port") ) {
                    this->port(vm["port"].as<int>());
                    (*logger)->log("port retrieved " + boost::lexical_cast<std::string>(this->port()));
                } else {
                    (*logger)->error("Missing required option 'port'");
                    errors++;
                }
 
                if ( vm.count("command-prompt") ) {
                    this->command_prompt(vm["command-prompt"].as<std::string>());
                    (*logger)->log("command-prompt retrieved " + this->command_prompt());
                } else {
                    (*logger)->error("Missing required option 'command-prompt'");
                    errors++;
                }
            
                if ( this->log_directory().length() > 0 ) {
                    *logger = new chess::server::logger(this->log_directory());
                } else {
                    *logger = new chess::server::logger();
                }
            
                (*logger)->log("Started logger");
 
                if ( vm.count("backlog-limit") ) {
                    this->backlog_limit(vm["backlog-limit"].as<int>());
                    (*logger)->log("backlog_limit retrieved " + boost::lexical_cast<std::string>(this->backlog_limit()));
                } else {
                    (*logger)->log("Missing option 'backlog-limit', setting to 20");
                    this->backlog_limit(20);
                }
 
                if ( vm.count("port") ) {
                    this->port(vm["port"].as<int>());
                    (*logger)->log("port retrieved " + boost::lexical_cast<std::string>(this->port()));
                } else {
                    (*logger)->error("Missing required option 'port'");
                    errors++;
                }
 
                if ( vm.count("command-prompt") ) {
                    this->command_prompt(vm["command-prompt"].as<std::string>());
                    (*logger)->log("command-prompt retrieved " + this->command_prompt());
                } else {
                    (*logger)->error("Missing required option 'command-prompt'");
                    errors++;
                }

                return this->option_errors(errors);
            }

            protected:
            int m_port;
            int m_option_errors;
            int m_backlog_limit;
            int m_data_buffer_size;
            std::string m_log_directory;
            std::string m_temp_directory;
            std::string m_command_prompt;
        };
    }
}

#endif
