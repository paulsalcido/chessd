#include <chess/server/options.hpp>
#include <chess/server/logger.hpp>
#include <chess/server/primary.hpp>

int main (int argc,char** argv) {
    chess::server::logger *logger;
    chess::server::options server_options;
    // This will also create the logger as necessary.
    server_options.load_arguments(argc,argv,&logger);

    chess::server::primary *primary_server;
    primary_server = new chess::server::primary(
        &server_options
        ,logger);

    primary_server->main_connect();
    primary_server->start_listen();

    delete primary_server;
    primary_server = NULL;

    delete logger;
    logger = NULL;

    return server_options.option_errors();
}
