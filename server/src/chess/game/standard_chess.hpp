#ifndef CHESS_GAME_STANDARD_CHESS_VERSION
#define CHESS_GAME_STANDARD_CHESS_VERSION 0.1

#include <chess/game.hpp>

#include <boost/property_tree/ptree.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#define _WP 1
#define _WN 2
#define _WB 3
#define _WR 4
#define _WQ 5
#define _WK 6

#define _BP -1
#define _BN -2
#define _BB -3
#define _BR -4
#define _BQ -5
#define _BK -6

#define _EM 0

namespace chess {
    namespace games {
        class standard_chess : public chess::game {
            public:
                standard_chess() {
                }

                ~standard_chess() {
                }

                virtual bool setup(const boost::property_tree::ptree& pt) {
                    /* Todo: add checks for each of these. */
                    m_white = pt.get<std::string>("white");
                    m_game_number = pt.get<int>("game-id");
                    m_black = pt.get<std::string>("black");
                    m_starting_time = pt.get<int>("starting-time");
                    if ( pt.count("increment") ) {
                        m_increment = pt.get<int>("increment");
                    }
                    if ( pt.count("delay") ) {
                        m_delay = pt.get<int>("delay");
                    }
                    m_counter = 1;
                    m_white_turn = true;

                    //m_board = (int**) new int[8][8];

                    for ( int i = 0 ; i < 8 ; i ++ ) {
                        for ( int k = 0 ; k < 8 ; k ++ ) {
                            m_board[i][k] = _EM;
                        }
                    }

                    m_board[0][0] = m_board[0][7] = _WR;
                    m_board[0][1] = m_board[0][6] = _WN;
                    m_board[0][2] = m_board[0][5] = _WB;
                    m_board[0][3] = _WQ;
                    m_board[0][4] = _WK;

                    m_board[7][0] = m_board[7][7] = _BR;
                    m_board[7][1] = m_board[7][6] = _BN;
                    m_board[7][2] = m_board[7][5] = _BB;
                    m_board[7][3] = _BQ;
                    m_board[7][4] = _BK;

                    for ( int i = 0 ; i < 8 ; i ++ ) {
                        m_board[1][i] = _WP;
                        m_board[6][i] = _BP;
                    }
                }

                /* Placeholder output */

                virtual std::string status(const std::string &player,const std::string &style) {
                    if ( ! style.compare("12") ) {
                        return _status_style_12(player);
                    }
                    return "";
                }

                int white_time_remains() {
                    return m_starting_time * 60 * 1000;
                }

                int black_time_remains() {
                    return m_starting_time * 60 * 1000;
                }

                const std::string black() {
                    return m_black;
                }
                
                const std::string white() {
                    return m_white;
                }

                int move_number() {
                    return m_counter;
                }

                bool white_turn() {
                    return m_white_turn;
                }

                char piece_as_char(int p) {
                    char retval = '-';
                    switch(p) {
                        case _WP: retval = 'P'; break;
                        case _WN: retval = 'N'; break;
                        case _WB: retval = 'B'; break;
                        case _WQ: retval = 'Q'; break;
                        case _WK: retval = 'K'; break;
                        case _WR: retval = 'R'; break;
                        case _BP: retval = 'p'; break;
                        case _BN: retval = 'n'; break;
                        case _BB: retval = 'b'; break;
                        case _BQ: retval = 'q'; break;
                        case _BK: retval = 'k'; break;
                        case _BR: retval = 'r'; break;
                    }
                    return retval;
                }

                virtual bool test_move(const std::string &player,const std::string &move) {
                    // First, we need to translate to normal translation.
                    bool retval = false;
                    /* Should really use lower here (TODO) */
                    if ( ! move.compare("o-o") || ! move.compare("O-O") ) {
                        // castle!
                        int row = white_turn() ? 0 : 7 ;
                        int mult = white_turn() ? 1 : -1 ;
                        if ( m_board[row][7] == _WR*mult && m_board[row][4] == _WK*mult &&
                                m_board[row][5] == _EM && m_board[row][6] == _EM ) {
                            m_board[row][5] = _WR*mult;
                            m_board[row][6] = _WK*mult;
                            m_board[row][4] = m_board[row][7] = 0;
                            retval = true;
                        }
                    } else if ( ! move.compare("o-o-o") || ! move.compare("O-O-O") ) {
                        int row = white_turn() ? 0 : 7;
                        int mult = white_turn() ? 1 : -1 ;
                        if ( m_board[row][0] == _WR*mult && m_board[row][4] == _WK*mult &&
                                m_board[row][1] == _EM && m_board[row][2] == _EM && m_board[row][3] == _EM ) {
                            m_board[row][3] = _WR*mult;
                            m_board[row][2] = _WK*mult;
                            m_board[row][0] = m_board[row][1] = m_board[row][4] = 0;
                            retval = true;
                        }
                    } else {
                        int first_row = -1;
                        int final_row = -1;
                        int first_col = -1;
                        int final_col = -1;
                        first_row = (int)move[0] - (int)'a';
                        final_row = (int)move[2] - (int)'a';
                        first_col = (int)move[1] - (int)'1';
                        final_col = (int)move[3] - (int)'1';
                        /*std::cout 
                            << first_row << " "
                            << first_col << " "
                            << final_row << " "
                            << final_col << " "
                            << std::endl;*/
                        if ( first_row >= 0 && first_row < 8 &&
                                final_row >= 0 && final_row < 8 &&
                                first_col >= 0 && first_col < 8 &&
                                final_col >= 0 && final_col < 8 &&
                                (( white_turn() && !player.compare(white()) ) || ( !white_turn() && !player.compare(black()) ) ) ) {
                            int piece = m_board[first_col][first_row];
                            //std::cout << piece << " " << piece_as_char(piece) << std::endl;
                            if ( piece && ( ( white_turn() && piece > 0 ) || ( ! white_turn() && piece < 0 ) ) ) {
                                m_board[final_col][final_row] = m_board[first_col][first_row];
                                m_board[first_col][first_row] = _EM;
                                retval = true;
                                m_move_list.push_back(move);
                            }
                        }
                    }
                    if ( retval ) {
                        this->m_white_turn = ! this->m_white_turn;
                        if ( this->m_white_turn ) {
                            m_counter++;
                        }
                    }
                    return retval;
                }

                std::string move_list() {
                    std::vector<std::string>::iterator it = m_move_list.begin();
                    std::string retval = "";
                    int i = 0;
                    for ( ; it != m_move_list.end() ; it++ ) {
                        if ( i > 0 ) {
                            retval += " ";
                        }
                        i++;
                        retval += (*it);
                    }
                    return retval;
                }

                int starting_time() {
                    return m_starting_time;
                }
                int increment() {
                    return m_increment;
                }

                std::string get_attribute(std::string key) {
                    std::string retval;
                    if ( ! key.compare("black") ) {
                        retval = black();
                    } else if ( ! key.compare("white") ) {
                        retval = white();
                    }
                    if ( ! retval.length() ) {
                        retval = game::get_attribute(key);
                    }
                    return retval;
                }

            protected:

                std::string m_white;
                std::string m_black;
                std::vector<std::string> m_move_list;
                int m_starting_time;
                int m_increment;
                int m_delay;
                int m_counter;
                bool m_white_turn;

                int m_board[8][8];

                std::string _status_style_12(const std::string player) {
                    std::ostringstream stm;
                    stm << "<12> ";
                    for ( int i = 7 ; i >= 0 ; i-- ) {
                        for ( int k = 0 ; k < 8 ; k++ ) {
                            stm << piece_as_char(m_board[i][k]);
                        }
                        stm << " ";
                    }
                    if ( white_turn() ) {
                        stm << "W";
                    } else {
                        stm << "B";
                    }
                    // double pawn...
                    stm << " " << -1;
                    // white short castle
                    stm << " " << 1;
                    // white long castle
                    stm << " " << 1;
                    // black short castle
                    stm << " " << 1;
                    // black long castle
                    stm << " " << 1;
                    // draw moves
                    stm << " " << 0;
                    // game number
                    stm << " " << m_game_number;
                    stm << " " << m_white << " " << m_black;
                    if ( ( ! player.compare(m_white) && white_turn() ) || ( ! player.compare(m_black) && ! white_turn()  ) ) {
                        stm << " " << 1; 
                    } else if ( ! player.compare(m_white) || ! player.compare(m_black) ) {
                        stm << " " << -1; 
                    } else {
                        stm << " " << 0;
                    }
                    stm << " " << m_starting_time * 60;
                    stm << " " << m_increment;
                    // white material 
                    stm << " " << 39;
                    // black material 
                    stm << " " << 39;
                    stm << " " << white_time_remains() << " " << black_time_remains();
                    stm << " " << move_number();
                    stm << " none";
                    stm << " (0:00.000)";
                    stm << " none";
                    if ( ! player.compare(m_black) ) {
                        stm << " 1";
                    } else {
                        stm << " 0";
                    }
                    return stm.str();
                }
        };
    }
}

#endif
