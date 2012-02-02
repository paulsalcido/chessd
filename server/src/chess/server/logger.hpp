#ifndef CHESS_SERVER_LOG_VERSION
#define CHESS_SERVER_LOG_VERSION 0.1

#include <iostream>
#include <fstream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace chess {
    namespace server {
        class logger {
            public:
                logger() {
                    m_os = &std::cout;
                    m_dbg = &std::cout;
                    m_err = &std::cerr;
                    m_std_out = true;
                }
                logger(std::string log_directory) {
                    m_log_directory = log_directory;
                    m_std_out = false;
                    m_os = NULL;
                    this->open();
                }

                ~logger() {
                    this->close();
                }

                void log(std::string output) {
                    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                    *m_os << "[" << now << "] " << output << std::endl;
                }

                void error(std::string output) {
                    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                    *m_os << "[" << now << "] " << output << std::endl;
                }

                std::ostream *os() {
                    return m_os;
                }

            protected:
                bool open() {
                    bool success = false;
                    if ( m_std_out == false ) {
                        success = true;
                        if ( m_log_directory.length() == 0 ) {
                            m_log_directory = ".";
                        }
                        m_log_directory += "/";
                        std::ofstream *logfile = new std::ofstream();
                        std::string curfile = m_log_directory + "chessserver.log";
                        logfile->open(curfile.c_str());
                        m_os = (std::ostream*)logfile;

                        std::ofstream *errfile = new std::ofstream();
                        std::string errfilename = m_log_directory + "chessserver_errors.log";
                        errfile->open(errfilename.c_str());
                        m_err = (std::ostream*)errfile;

                        std::ofstream *dbgfile = new std::ofstream();
                        std::string dbgfilename = m_log_directory + "chessserver_debug.log";
                        dbgfile->open(dbgfilename.c_str());

                        m_dbg = (std::ostream*)dbgfile;
                        success = ((std::ofstream*)m_os)->is_open();
                    }
                    return success;
                }

                bool close() {
                    bool success = false;
                    if ( m_std_out == false && m_os != NULL ) {
                        success = true;
                        ((std::ofstream*)m_os)->close();
                        delete m_os;
                    }
                    return success;
                }

                std::string m_log_directory;
                std::ostream *m_os;
                std::ostream *m_err;
                std::ostream *m_dbg;
                bool m_std_out;
        };
    }
}

#endif
