#include "client.h"

string host;
string port;

// TODO: use boost::program_options
int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <server-address> <file path>" << std::endl;
        std::cerr << "sample: " << argv[0] << " 127.0.0.1:1234 c:\\tmp\\a.txt" << std::endl;
        return __LINE__;
    }

    try {
        string hostPort = argv[1];
        string fileName = argv[2];

        size_t pos = hostPort.find(':');
        if (pos == string::npos) {
            return __LINE__;
        }
        port = hostPort.substr(pos+1);
        host = hostPort.substr(0, pos);
        string username = "sevara";

        get(username, fileName);
    } catch (exception& e) {
        cout << "some error happened" << endl;
        cout << e.what() << endl;
    }
}

