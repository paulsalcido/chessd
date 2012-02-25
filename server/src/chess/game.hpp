#ifndef CHESS_GAME_VERSION
#define CHESS_GAME_VERSION 0.1

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <time.h>

#include <string>
#include <map>

namespace chess {
    /* TODO: This will eventually be a base class only.  For now... */
    class game {
        public:
            game() {
                m_last_time = NULL;
                m_ignore_time = true;
            }
            ~game() {
                for ( std::map<std::string,struct timespec*>::iterator it = m_time_left.begin() ;
                    it != m_time_left.end() ; it ++ ) {
                    delete it->second;
                    m_time_left.erase(it);
                }
                if ( m_last_time != NULL ) {
                    delete m_last_time;
                }
            }

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

            virtual std::string game_name() = 0;

            void setup_times(std::vector<std::string> players,struct timespec &tv) {
                for ( std::vector<std::string>::iterator it = players.begin() ;
                    it != players.end() ; it ++ ) {
                    struct timespec *tmp = new struct timespec;
                    tmp->tv_sec = tv.tv_sec;
                    tmp->tv_nsec = tv.tv_nsec;
                    m_time_left[*it] = tmp;
                }
            }

            bool mark_time(std::string player,bool lose_on_time,int increment,int delay = 0) {
                bool retval = false;
                if ( m_last_time == NULL ) {
                    m_last_time = new struct timespec;
                    /* Using monotonic prevents time skipping */
                    retval = true;
                } else {
                    if ( m_time_left.count(player) ) {
                        struct timespec curtime;
                        struct timespec *duration;
                        clock_gettime(CLOCK_MONOTONIC,&curtime);
                        struct timespec *playertime = m_time_left[player];
                        duration = _duration(m_last_time,&curtime);
                        playertime->tv_sec -= duration->tv_sec;
                        if ( duration->tv_sec >= delay ) {
                            duration->tv_sec -= delay;
                            if ( duration->tv_nsec > playertime->tv_nsec ) {
                                playertime->tv_sec -= 1;
                                playertime->tv_nsec = (playertime->tv_nsec + 1000000000) - duration->tv_nsec;
                            } else {
                                playertime->tv_nsec -= duration->tv_nsec;
                            }
                            playertime->tv_sec += increment;
                        }
                        retval = true;
                    }
                }
                if ( retval ) {
                    clock_gettime(CLOCK_MONOTONIC,m_last_time);
                }
                return retval;
            }

            bool check_time() {
                bool success = true;
                if ( m_controlling_player.length() && m_last_time != NULL && !m_ignore_time) {
                    struct timespec curtime;
                    clock_gettime(CLOCK_MONOTONIC,&curtime);
                    struct timespec *dur = _duration(m_last_time,&curtime);
                    struct timespec *player = m_time_left[m_controlling_player];
                    if ( dur->tv_sec > player->tv_sec || 
                        ( dur->tv_sec == player->tv_sec && dur->tv_nsec > player->tv_nsec ) ) {
                        success = false;
                    }
                }
                return success;
            }
        protected:

            static struct timespec* _duration(const struct timespec* tv1,const struct timespec* tv2) {
                struct timespec *dur = new struct timespec;
                const struct timespec *t1,*t2;
                if ( tv1->tv_sec > tv2->tv_sec || ( tv1->tv_sec == tv2->tv_sec && tv1->tv_nsec > tv2->tv_nsec ) ) {
                    t1 = tv1;
                    t2 = tv2;
                } else {
                    t1 = tv2;
                    t2 = tv1;
                }
                dur->tv_sec = t1->tv_sec - t2->tv_sec;
                if ( t1->tv_nsec < t2->tv_nsec ) {
                    dur->tv_sec -= 1;
                    dur->tv_nsec = (t1->tv_nsec + 1000000000) - t2->tv_nsec;
                } else {
                    dur->tv_nsec = t1->tv_nsec - t2->tv_nsec;
                }
                return dur;
            }

            std::string m_owner;
            std::string m_opponent;
            int m_game_number;
            std::string m_controlling_player;
            std::map<std::string,struct timespec *> m_time_left;
            struct timespec *m_last_time;
            bool m_ignore_time;
    };
}

#endif
