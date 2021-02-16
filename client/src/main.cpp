#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;
string host;
string port;



string zip(string& user, string& file);
string unzip(string& user, string& file);
bool get(string& user, string& file);
bool zip_and_get(string& user, string& file);
bool unzip_and_get(string& user, string& file);


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
        string username = "islom";
        string archived_file = zip( username, fileName);
        cout << "archived_file: " << archived_file << endl;

    } catch (exception& e) {
        cout << "some error happened" << endl;
        cout << e.what() << endl;
    }
}

string zip(string &user, string &file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(host, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);

    if (error) {
        cout << "failed to connect" << endl;
        return __TIME__;
    }

    enum { MessageSize = 1024 };
    array<char, MessageSize> buf{};
    cout << "connected to " << host << ":" << port << endl;
    ifstream source_file(file, ios_base::binary | ios_base::ate);
    if (!source_file)
    {
        std::cout << "failed to open " << file << std::endl;
        return __TIME__;
    }

    // get length of file
    source_file.seekg(0, source_file.end);
    auto file_size = source_file.tellg();
    source_file.seekg(0, source_file.beg);

    // first send file name and file size to server
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "zip" << " " << user << " " << "salom.txt" << " " << file_size << "\n\n";
    boost::asio::write(socket, request);

    // then send file
    std::cout << "start sending file content.\n";


    while (source_file) {
        source_file.read(buf.data(), buf.size());
        if (source_file.fail() && !source_file.eof()) {
            auto msg = "Failed while reading file";
            std::cout << msg;
            throw std::fstream::failure(msg);
        }

        auto m_buf = boost::asio::buffer(buf.data(), static_cast<size_t>(source_file.gcount()));
        boost::system::error_code error3;
        boost::asio::write(socket, m_buf, error3);
    }

    cout << "file stream sent" << endl;

    boost::asio::streambuf response;
    boost::system::error_code error2;
    boost::asio::read(socket, response, boost::asio::transfer_all(), error2);
    std::istream ss(&response);
    std::string ok;
    std::string name;
    ss >> ok; ss >> name;
    std::cout << "message: " << ok << " " << name << std::endl;
    ioContext.stop();
    return name;
}
