#include <iostream>
#include <boost/program_options.hpp>

int main (int argc,char** argv) {
    std::cout << "Hello World." << std::endl;
    boost::program_options::options_description desc("Allowed Options");
    desc.add_options()
        ("port",boost::program_options::value<int>(),"Port to open for primary connections.")
        ("min-port-range",boost::program_options::value<int>(),"Ports to use for children connections.")
        ("max-port-range",boost::program_options::value<int>(),"Ports to use for children connections.")
    ;

    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc,argv,desc)
        ,vm);
    boost::program_options::notify(vm);

    if ( vm.count("port") ) {
        std::cout << "Port set to " << vm["port"].as<int>() << "." << std::endl;
    }

    return 0;
}
