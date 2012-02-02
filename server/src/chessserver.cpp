#include <chess/server/options.hpp>
#include <chess/server/logger.hpp>

int main (int argc,char** argv) {
    chess::server::logger *logger;
    chess::server::options server_options;
    server_options.load_arguments(argc,argv,&logger);

    return server_options.option_errors();
}
