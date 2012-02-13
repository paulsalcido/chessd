#ifndef CHESS_PTREE_VERSION
#define CHESS_PTREE_VERSION 0.1

#include <sstream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace chess {
    namespace ptree {
        std::string ptree_as_string(const boost::property_tree::ptree &pt) {
            std::ostringstream stm;
            boost::property_tree::json_parser::write_json(stm,pt);
            return stm.str();
        }

        void fill_ptree (const std::string &str,boost::property_tree::ptree &pt) {
            std::istringstream stm(str);
            boost::property_tree::json_parser::read_json(stm,pt);
        }

        std::string action_as_send_string (const boost::property_tree::ptree &pt) {
            std::ostringstream stm;
            stm << "\n" << pt.get<std::string>("player") << " " << pt.get<std::string>("action") << "s: "
                << pt.get<std::string>("message") << "\n";
            return stm.str();
        }
    }
}

#endif
