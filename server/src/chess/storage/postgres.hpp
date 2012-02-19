#ifndef CHESS_SERVER_STORAGE_POSTGRES_VERSION
#define CHESS_SERVER_STORAGE_POSTGRES_VERSION 0.1

#include <postgresql/libpq-fe.h>
#include <chess/storage/base.hpp>

#include <string.h>
#include <map>
#include <string>

namespace chess {
    namespace storage {
        class postgres:public chess::storage::base {
            public:
                postgres() {
                }
                ~postgres() {
                }

                virtual bool connect(std::map<std::string,std::string>& lv) {
                    bool success = false;
                    if ( ! is_connected() ) {
                        char *keys[20];
                        char *vals[20];
                        std::string tmp;
                        int arg_count = 0;
                        for ( std::map<std::string,std::string>::iterator it = lv.begin() ;
                                it != lv.end() ; it ++ ) {
                            keys[arg_count] = new char[it->first.length()+1];
                            strcpy(keys[arg_count],it->first.c_str());
                            vals[arg_count] = new char[it->second.length()+1];
                            strcpy(vals[arg_count],it->second.c_str());
                            arg_count++;
                        }
                        keys[arg_count] = NULL;
                        vals[arg_count] = NULL;
                        m_connection = PQconnectdbParams((const char**)keys,(const char**)vals,0);
                        if ( PQstatus(m_connection) != CONNECTION_OK ) {
                            /* TODO, put this in some local variable. */
                        } else {
                            m_is_connected = true;
                            success = true;
                        }
                    }
                    return success;
                }

                virtual bool disconnect() {
                    bool success = false;
                    if ( is_connected() ) {
                        PQfinish(m_connection);
                        m_is_connected = false;
                        m_connection = NULL;
                    }
                    return success;
                }

                virtual chess::storage::player* login(const std::string&,const std::string&) {
                    return NULL;
                }
                virtual chess::storage::player* get_player(std::map<std::string,std::string>) {
                    return NULL;
                }
                virtual std::map<std::string,int>* can_store_games() {
                    return NULL;
                }
                virtual bool store(chess::game*) {
                    return false;
                }
            protected:
                PGconn* m_connection;
        };
    }
}

#endif
