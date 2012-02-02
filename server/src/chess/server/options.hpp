#ifndef CHESS_SERVER_OPTIONS_VERSION
#define CHESS_SERVER_OPTIONS_VERSION 0.1

#include <string>

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

            int port(int i_port) {
                return this->m_port = i_port;
            }
            int port() {
                return this->m_port;
            }

            protected:
            int m_port;
            int m_max_port_range;
            int m_min_port_range;
            std::string m_log_directory;
        };
    }
}

#endif
