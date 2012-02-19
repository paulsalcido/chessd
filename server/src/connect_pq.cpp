#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>

#include <chess/storage/base.hpp>
#include <chess/storage/postgres.hpp>

int main(int argc, char** argv) {
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("port",boost::program_options::value<int>(),"postgres port.")
        ("host",boost::program_options::value<std::string>(),"postgres host.")
        ("user",boost::program_options::value<std::string>(),"postgres user.")
        ("pass",boost::program_options::value<std::string>(),"postgres pass.")
    ;
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc,argv,desc),vm);
    boost::program_options::notify(vm);
    std::map<std::string,std::string> login_options;
    int errors = 0;
    if ( ! vm.count("host") ) {
        std::cerr << "host required." << std::endl;
        errors++;
    } else {
        login_options["host"] = vm["host"].as<std::string>();
    }
    if ( ! vm.count("user") ) {
        std::cerr << "user required." << std::endl;
        errors++;
    } else {
        login_options["user"] = vm["user"].as<std::string>();
    }
    if ( ! vm.count("pass") ) {
        std::cerr << "pass required." << std::endl;
        errors++;
    } else {
        login_options["password"] = vm["pass"].as<std::string>();
    }
    if ( ! vm.count("port") ) {
        std::cerr << "port required." << std::endl;
        errors++;
    } else {
        login_options["port"] = boost::lexical_cast<std::string>(vm["port"].as<int>());
    }
    chess::storage::base* conn = new chess::storage::postgres();
    if ( conn->connect(login_options) ) {
        std::cout << "great!" << std::endl;
        conn->disconnect();
    } else {
        std::cout << "poo!" << std::endl;
    }
    return 0;
    //PGconn* conn;
    //PGresult* res;
    /*std::string conninfo = "host=" + vm["host"].as<std::string>() 
        + " user=" + vm["user"].as<std::string>()
        + " password=" + vm["pass"].as<std::string>()
        + " port=" + boost::lexical_cast<std::string>(vm["port"].as<int>());*/
    //conn = PQconnectdb(conninfo.c_str());
    /*const char *keys[5];
    const char *values[5];
    keys[0] = "host";
    keys[1] = "password";
    keys[2] = "port";
    keys[3] = "user";
    keys[4] = NULL;
    values[0] = vm["host"].as<std::string>().c_str();
    values[1] = vm["pass"].as<std::string>().c_str();
    values[2] = boost::lexical_cast<std::string>(vm["port"].as<int>()).c_str();
    values[3] = vm["user"].as<std::string>().c_str();
    values[4] = NULL;
    conn = PQconnectdbParams(keys,values,0);
    if ( PQstatus(conn) != CONNECTION_OK ) {
        std::cerr << PQerrorMessage(conn) << std::endl;
        exit(1);
    }
    const char *arguments[0];
    arguments[0] = "a";
    res = PQexecParams(conn,"select * from test1 where id = $1",1,NULL,arguments,NULL,NULL,0);
    std::cout << PQntuples(res) << ", " << PQnfields(res) << std::endl;
    PQclear(res);
    PQfinish(conn);
    exit(0);*/
}
