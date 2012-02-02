#include <boost/program_options.hpp>
#include <chess/server/options.hpp>
#include <chess/server/logger.hpp>
#include <boost/lexical_cast.hpp>

int main (int argc,char** argv) {
    boost::program_options::options_description desc("Allowed Options");
    desc.add_options()
        ("port",boost::program_options::value<int>(),"Port to open for primary connections.")
        ("min-port-range",boost::program_options::value<int>(),"Ports to use for children connections.")
        ("max-port-range",boost::program_options::value<int>(),"Ports to use for children connections.")
        ("log-directory",boost::program_options::value<std::string>(),"Logging directory.")
    ;

    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc,argv,desc)
        ,vm);
    boost::program_options::notify(vm);

    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    boost::gregorian::date today = now.date();

    int errors = 0;
    chess::server::logger *logger;

    chess::server::options server_options;

    if ( vm.count("log-directory") ) {
        server_options.log_directory(vm["log-directory"].as<std::string>());
    }

    if ( server_options.log_directory().length() > 0 ) {
        logger = new chess::server::logger(server_options.log_directory());
    } else {
        logger = new chess::server::logger();
    }

    logger->log("Started logger");

    if ( vm.count("port") ) {
        server_options.port(vm["port"].as<int>());
        logger->log("port retrieved " + boost::lexical_cast<std::string>(server_options.port()));
    } else {
        logger->error("Missing required option 'port'");
        errors++;
    }

    if ( vm.count("min-port-range") ) {
        server_options.min_port_range(vm["min-port-range"].as<int>());
        logger->log("Minimum port range set to " + boost::lexical_cast<std::string>(server_options.min_port_range()));
    } else {
        logger->error("Missing required option 'min-port-range'");
        errors++;
    }

    if ( vm.count("max-port-range") ) {
        server_options.max_port_range(vm["max-port-range"].as<int>());
        logger->log("Maximum port range set to " + boost::lexical_cast<std::string>(server_options.max_port_range()));
    } else {
        logger->error("Missing required option 'max-port-range'");
        errors++;
    }

    return errors;
}
