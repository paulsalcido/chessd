#ifndef CHESS_SERVER_STORAGE_VERSION
#define CHESS_SERVER_STORAGE_VERSION 0.1

#include <chess/game.hpp>

#include <string>
#include <map>

namespace chess {
    namespace storage {
        class player;

        class base {
            public:
                base() {
                    m_is_connected = false;
                }
                ~base() {
                }

                virtual bool connect(std::map<std::string,std::string>&) = 0;
                virtual chess::storage::player* login(const std::string&,const std::string&) = 0;
                virtual chess::storage::player* get_player(std::map<std::string,std::string>) = 0;
                virtual bool store(chess::game*) = 0;
                virtual std::map<std::string,int>* can_store_games() = 0;
                virtual bool disconnect() = 0;
                int is_connected() { return m_is_connected; }
            protected:
                bool m_is_connected;
        };

        class player {
            public:
                player() {
                }
                player(std::string short_name,std::string long_name,std::string id) :
                    m_short_name(short_name),m_long_name(long_name),m_id(id)
                {
                }
                ~player() {
                }

                std::string short_name() { return m_short_name; }
                std::string long_name() { return m_long_name; }
                std::string id() { return m_id; }

            protected:
                std::string m_short_name;
                std::string m_long_name;
                std::string m_id;
        };

    }
}

#endif
