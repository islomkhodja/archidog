#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;
string host;
string port;



string sendFileWithCommand(const string& user, const string &file, const string& command);
bool get(const string& user, const string& file);
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
        string username = "sevara";
//        string archived_file = sendFileWithCommand( username, fileName, string("zip"));
//        cout << "archived_file: " << archived_file << endl;
//        string decompressed_file = sendFileWithCommand( username, string("../client_files/salom.txt.sev"), string("unzip"));
//        cout << "decompressed_file: " << decompressed_file << endl;
        get(username, string("salom.txt"));
    } catch (exception& e) {
        cout << "some error happened" << endl;
        cout << e.what() << endl;
    }
}

string sendFileWithCommand(const string& user, const string &file, const string& command) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(host, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);
    boost::filesystem::path p(file);

    if (error) {
        cout << "failed to connect" << endl;
        return __TIME__;
    }


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

    enum { MessageSize = 1024 };
    array<char, MessageSize> buf{};

    // first send file name and file size to server
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << command << " " << user << " " << p.filename().string() << " " << file_size << "\n\n";
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

        cout << "send byte" << endl;
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

bool get(const string& user, const string& file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(host, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);
    std::array<char, 1024> buf{};
    boost::asio::streambuf request_buf;

    boost::asio::streambuf request;
    std::ostream commandStream(&request);
    commandStream << "get" << " " << user << " " << file << "\n\n";
    boost::asio::write(socket, request);


    boost::asio::read_until(socket, request_buf, "\n\n");
    std::cout<< "request size:" << request_buf.size() << "\n";

    istream requestStream(&request_buf);
    string responseStatus;
    string file_path;
    size_t file_size = 0;

    requestStream >> responseStatus;
    requestStream >> file_path;
    requestStream >> file_size;

    requestStream.read(buf.data(), 2); // eat the "\n\n"

    cout << file_path << " size is " << file_size << endl;

    size_t pos = file_path.find_last_of('\\');
    if (pos!=std::string::npos)
        file_path = file_path.substr(pos+1);

    file_path = "../client_files/" + file_path;

    std::ofstream outputFile(file_path, std::ios_base::binary);
    if (!outputFile)
    {
        std::cout << "failed to open " << file_path << std::endl;
        return __LINE__;
    }

    // write extra bytes to file
    do
    {
        requestStream.read(buf.data(), (std::streamsize) buf.size());
        std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes.\n";
        outputFile.write(buf.data(), requestStream.gcount());
    } while (requestStream.gcount() > 0);

    if (outputFile.tellp() >= static_cast<std::streamsize>(file_size)) {
        cout << "OK file is saved" << endl;
        return true;
    }


    while (true) {
        boost::system::error_code ignore_error;
        size_t bytes = socket.read_some(boost::asio::buffer(buf.data(), buf.size()), ignore_error);
        if (!ignore_error) {
            if (bytes > 0) {
                outputFile.write(buf.data(), static_cast<std::streamsize>(bytes));
                if (outputFile.tellp() >= static_cast<std::streamsize>(file_size)) {
                    break;
                }
            }
        } else {
            cout << "error while loop" << endl;
            return false;
        }

    }

    cout << "OK file is saved" << endl;
    return true;
}

