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

            options(int port,
                    int max_port_range,
                    int min_port_range):
                m_max_port_range(max_port_range),
                m_min_port_range(min_port_range),
                m_port(port),
                m_log_directory(".")
            { }

            options(int port,
                    int max_port_range,
                    int min_port_range,
                    std::string log_directory):
                m_max_port_range(max_port_range),
                m_min_port_range(min_port_range),
                m_port(port),
                m_log_directory(log_directory)
            { }

            ~options() { }

            int max_port_range(int i_max_port_range) {
                return this->m_max_port_range = i_max_port_range;
            }
            int max_port_range() {
                return this->m_max_port_range;
            }

            int min_port_range(int i_min_port_range) {
                return this->m_min_port_range = i_min_port_range;
            }
            int min_port_range() {
                return this->m_min_port_range;
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

            int port(int i_port) {
                return this->m_port = i_port;
            }
            int port() {
                return this->m_port;
            }

            int load_arguments(int argc, char **argv, chess::server::logger **logger) {
                boost::program_options::options_description desc("Allowed Options");
                desc.add_options()
                    ("port",boost::program_options::value<int>(),"Port to open for primary connections.")
                    ("min-port-range",boost::program_options::value<int>(),"Ports to use for children connections.")
                    ("max-port-range",boost::program_options::value<int>(),"Ports to use for children connections.")
                    ("log-directory",boost::program_options::value<std::string>(),"Logging directory.")
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
            
                if ( this->log_directory().length() > 0 ) {
                    *logger = new chess::server::logger(this->log_directory());
                } else {
                    *logger = new chess::server::logger();
                }
            
                (*logger)->log("Started logger");
            
                if ( vm.count("port") ) {
                    this->port(vm["port"].as<int>());
                    (*logger)->log("port retrieved " + boost::lexical_cast<std::string>(this->port()));
                } else {
                    (*logger)->error("Missing required option 'port'");
                    errors++;
                }
            
                if ( vm.count("min-port-range") ) {
                    this->min_port_range(vm["min-port-range"].as<int>());
                    (*logger)->log("Minimum port range set to " + boost::lexical_cast<std::string>(this->min_port_range()));
                } else {
                    (*logger)->error("Missing required option 'min-port-range'");
                    errors++;
                }
            
                if ( vm.count("max-port-range") ) {
                    this->max_port_range(vm["max-port-range"].as<int>());
                    (*logger)->log("Maximum port range set to " + boost::lexical_cast<std::string>(this->max_port_range()));
                } else {
                    (*logger)->error("Missing required option 'max-port-range'");
                    errors++;
                }
                return this->option_errors(errors);
            }

            protected:
            int m_port;
            int m_max_port_range;
            int m_min_port_range;
            int m_option_errors;
            std::string m_log_directory;
        };
    }
}

#endif
