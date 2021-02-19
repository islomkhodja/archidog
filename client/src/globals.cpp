#include "globals.h"

// globals
string file;
string dirClient;
string user;
string command;
string hostIp;
string port;

bool parse_args(int argc, char *argv[]) {
    po::options_description desc("Allowed options");

    desc.add_options()
            ("help,h", "print usage message")
            ("ip, ip", po::value(&hostIp), "IP addres")
            ("port,p", po::value(&port), "port")
            ("dir,d", po::value(&dirClient), "the path to the folder where the files are saved")
            ("command,c", po::value(&command), "command")
            ("username,u", po::value(&user),"username")
            ("file,f", po::value(&file), "file");

    // Parse argc and argv
    po::variables_map vm;
    store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return false;
    }

    if (!vm.count("port")) {
        cout << desc << endl;
        return false;
    }
    port = vm["port"].as<string>();


    user = vm["username"].as<string>();
    if (!vm.count("username")) {
        cout << desc << endl;
        return false;
    }

    command = vm["command"].as<string>();
    if (!vm.count("command")) {
        cout << desc << endl;
        return false;
    }

    file = vm["file"].as<string>();
    if (!vm.count("file")) {
        cout << desc << endl;
        return false;
    }

    dirClient= vm["dir"].as<string>();
    if (!vm.count("dir")) {
        cout << desc << endl;
        return false;
    }

    hostIp = vm["ip"].as<string>();
    if (!vm.count("ip")) {
        cout << desc << endl;
        return false;
    }

    return true;
}
