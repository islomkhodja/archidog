#include "globals.h"
#include "client.h"

string sendFileWithCommand(tcp::socket& socket, const string& user, const string &file, const string& command) {
    boost::filesystem::path p(dirClient + "/" + file);
    ifstream source_file(dirClient + "/" + file, ios_base::binary | ios_base::ate);
    if (!source_file)
    {
        std::cout << "failed to open " << file << std::endl;
        return "ERROR";
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

    if (command == "zip" || command == "unzip") {
        boost::asio::streambuf response;
        boost::system::error_code error2;
        boost::asio::read(socket, response, boost::asio::transfer_all(), error2);
        std::istream ss(&response);
        std::string ok;
        std::string name;
        ss >> ok; ss >> name;
        std::cout << "message: " << ok << " " << name << std::endl;
        if (ok == "Error") {
            return ok;
        }

        return name;
    }

    if (command == "zip-and-get") {
        return file + ".sev";
    } else if (command == "unzip-and-get") {
        size_t lastOf = file.find_last_of(".");
        string path = file.substr(0, lastOf);
        return path;
    }
}

bool getFileWithCommand(tcp::socket& socket, const string& user, const string &file) {
    std::array<char, 1024> buf{};
    boost::asio::streambuf request_buf;
    boost::system::error_code ignore_error_read_until;
    boost::asio::read_until(socket, request_buf, "\n\n", ignore_error_read_until);
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

    file_path = dirClient + "/" + file_path;

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

    return true;
}

string zip(const string& user, const string &file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(hostIp, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);

    if (error) {
        cout << "failed to connect" << endl;
        return __TIME__;
    }

    cout << "connected to " << hostIp << ":" << port << endl;
    std::string name;
    name = sendFileWithCommand(socket, user, file, string("zip"));

    ioContext.stop();
    return name;
}

string unzip(const string& user, const string &file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(hostIp, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);

    if (error) {
        cout << "failed to connect" << endl;
        return __TIME__;
    }

    cout << "connected to " << hostIp << ":" << port << endl;
    std::string name;
    name = sendFileWithCommand(socket, user, file, string("unzip"));

    ioContext.stop();
    return name;
}

bool get(const string& user, const string& file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(hostIp, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);
    boost::filesystem::path p(file);

    boost::asio::streambuf request;
    std::ostream commandStream(&request);
    commandStream << "get" << " " << user << " " << p.filename().string() << "\n\n";
    boost::asio::write(socket, request);

    if (getFileWithCommand(socket, user, file)) {
        cout << "Ok file is saved" << endl;
        return true;
    }

    return false;
}

bool zip_and_get(string& user, string& file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(hostIp, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);

    string archived_file = sendFileWithCommand(socket, user, file, string("zip-and-get"));
    if (archived_file == "Error") {
        cout << archived_file << endl;
        ioContext.stop();
        return false;
    }

    if (getFileWithCommand(socket, user, archived_file)) {
        ioContext.stop();
        return true;
    }

    ioContext.stop();
    return false;
}

bool unzip_and_get(string& user, string& file) {
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    tcp::resolver::query query(hostIp, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    tcp::socket socket(ioContext);
    boost::system::error_code error = boost::asio::error::host_not_found;
    boost::asio::connect(socket, endpoint_iterator, error);

    string ziped_file = sendFileWithCommand(socket, user, file, string("unzip-and-get"));
    if (ziped_file == "Error") {
        cout << ziped_file << endl;
        ioContext.stop();
        return false;
    }

    cout << "ziped_file: " << ziped_file << endl;

    if (getFileWithCommand(socket, user, ziped_file)) {
        ioContext.stop();
        return true;
    }

    ioContext.stop();
    return false;
}