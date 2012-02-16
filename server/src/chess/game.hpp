#ifndef CHESS_GAME_VERSION
#define CHESS_GAME_VERSION 0.1

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>

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
            int game_number(int val) {
                return m_game_number = val;
            }
            int game_number() {
                return m_game_number;
            }
            virtual std::string get_attribute(std::string key) {
                std::string retval;
                if ( ! key.compare("game-id") ) {
                    retval = boost::lexical_cast<std::string>(m_game_number);
                }
                return retval;
            }

            /* test_move
             * Arguments: player name, move text
             * Returns true if good move.
             */
            virtual bool test_move(const std::string&,const std::string&)=0; 

            /* setup
             * Accepts a property tree that can be handled by the game
             * definition.
             */
            virtual bool setup(const boost::property_tree::ptree&)=0;

            /* status
             * Arguments: player name, style name
             * prints out the status of the game, based on the 
             * arguments
             */
            virtual std::string status(const std::string&,const std::string&)=0;
        protected:
            std::string m_owner;
            std::string m_opponent;
            int m_game_number;
    };
}

#endif
