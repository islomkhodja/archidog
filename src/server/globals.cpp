#include "globals.h"

// globals
string dirClient;
boost::asio::ip::address ip;
ushort port;
uint maxNFiles;
uint reqsPerMin;
uint threadsNum;
bool verbose_flag;

bool parse_args(int argc, char *argv[]) {
    po::options_description desc("Allowed options");

    desc.add_options()
            ("help,h", "print usage message")
            ("verbose,v", "verbose mode")
            ("dir,d", po::value(&dirClient), "the path to the server folder where the files are saved")
            ("ip,i", po::value<string>(), "IP address")
            ("port,p", po::value(&port), "port")
            ("maxnfiles,n", po::value(&maxNFiles), "max number of files that the client can save")
            ("reqspermin,r", po::value(&reqsPerMin), "max number of requests for archiving from 1 client per 1 minute")
            ("workers,w", po::value(&threadsNum), "number of threads");

    // Parse argc and argv
    po::variables_map vm;
    store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return false;
    }
    verbose_flag = vm.count("verbose");
    if (!vm.count("port")) {
        cout << desc << endl;
        return false;
    }
    port = max((ushort) 1, vm["port"].as<ushort>());
    if (!vm.count("maxnfiles")) {
        cout << desc << endl;
        return false;
    }
    maxNFiles = max(1u, vm["maxnfiles"].as<uint>());
    if (!vm.count("reqspermin")) {
        cout << desc << endl;
        return false;
    }
    reqsPerMin = max(1u, vm["reqspermin"].as<uint>());
    if (!vm.count("workers")) {
        cout << desc << endl;
        return false;
    }
    threadsNum = max(1u, vm["workers"].as<uint>());
    if (!vm.count("dir")) {
        cout << desc << endl;
        return false;
    }
    dirClient = vm["dir"].as<string>();
    if (!vm.count("ip")) {
        cout << desc << endl;
        return false;
    }
    string ip_str = vm["ip"].as<string>();
    try {
        ip = boost::asio::ip::address::from_string(ip_str);
    }
    catch (const exception& ex) {
        cout << "ip address `"<< ip_str << "` is invalid, use 127.0.0.1 instead" << endl;
        return false;
    }
    return true;
}
