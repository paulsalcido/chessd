#ifndef CHESS_GAME_STANDARD_CHESS_VERSION
#define CHESS_GAME_STANDARD_CHESS_VERSION 0.1

#include <chess/game.hpp>

#include <string>
#include <sstream>
#include <iostream>

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

                bool setup(
                        std::string white,
                        std::string black,
                        int game_number,
                        int starting_time,
                        int increment,
                        int delay = 0
                    ) {
                    m_white = white;
                    m_game_number = game_number;
                    m_black = black;
                    m_starting_time = starting_time;
                    m_increment = increment;
                    m_delay = delay;

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

                std::string status_style_12(std::string player) {
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
                        stm << " " << -2;
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

                int white_time_remains() {
                    return m_starting_time * 60 * 1000;
                }

                int black_time_remains() {
                    return m_starting_time * 60 * 1000;
                }

                int move_number() {
                    return 1;
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

                bool test_move(std::string move) {
                    // First, we need to translate to normal translation.
                    bool retval = false;
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
                            final_col >= 0 && final_col < 8 ) {
                        int piece = m_board[first_col][first_row];
                        //std::cout << piece << " " << piece_as_char(piece) << std::endl;
                        if ( piece && ( ( white_turn() && piece > 0 ) || ( ! white_turn() && piece < 0 ) ) ) {
                            m_board[final_col][final_row] = m_board[first_col][first_row];
                            m_board[first_col][first_row] = _EM;
                            retval = true;
                            this->m_white_turn = ! this->m_white_turn;
                        }
                    }
                    return retval;
                }

            protected:

                std::string m_white;
                std::string m_black;
                int m_starting_time;
                int m_increment;
                int m_delay;
                bool m_white_turn;
                int m_game_number;

                int m_board[8][8];
        };
    }
}

#endif
