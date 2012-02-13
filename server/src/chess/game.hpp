#ifndef CHESS_GAME_VERSION
#define CHESS_GAME_VERSION 0.1

#include <string>

namespace chess {
    /* TODO: This will eventually be a base class only.  For now... */
    class game {
        public:
            game() {}
            ~game() {}

            std::string opponent(std::string val) {
                return m_opponent = val;
            }
            std::string opponent() {
                return m_opponent;
            }

            std::string owner(std::string val) {
                return m_owner = val;
            }
            std::string owner() {
                return m_owner;
            }

            virtual bool test_move(std::string)=0; 
        protected:
            std::string m_owner;
            std::string m_opponent;
            int m_game_number;
    };
}

#endif
