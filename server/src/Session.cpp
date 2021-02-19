#include "globals.h"
#include "Session.h"
#include "../common/src/rle.h"

std::map<std::string, UserSettings> UserMap;

Session::Session(TcpSocket t_socket, boost::asio::io_context& io, boost::filesystem::path m_workDirectory)
    :   m_socket(std::move(t_socket)),
        m_workDirectory(m_workDirectory),
        ioContext(io),
        my_strand(ioContext) {
}


void Session::read_commands() {
    auto self = shared_from_this();
    async_read_until(m_socket, m_request_buf_read, "\n\n",
                     boost::asio::bind_executor(my_strand, [this, self](boost::system::error_code ec, size_t bytes) {
                         if (!ec) {
                             std::cout << "read_commands(); уже прочитал" << std::endl;
                             commandRouter(bytes);
                         }

                         else
                             handleError(__FUNCTION__, ec);
                     }));
}

bool Session::checkUserActive() {
    if (UserMap.find(m_user) == UserMap.end()) {
        UserMap[m_user] = { 1, time(nullptr), 0 };
    } else {
        std::time_t currentTime = time(nullptr);
        double diff = difftime(currentTime, UserMap[m_user].lastActiveTime);
        if (diff > 60) {
            UserMap[m_user].userReqCount = 1;
        } else {
            UserMap[m_user].userReqCount += 1;
        };

        if (UserMap[m_user].userReqCount > reqsPerMin) {
            std::cout << "many requests" << std::endl;
            return false;
        }

        if (m_command  == "zip"
        || m_command == "zip-and-get"
        || m_command == "unzip-and-get"
        || m_command == "unzip")
        {
            UserMap[m_user].userMaxFile += 1;
        }


        if (UserMap[m_user].userMaxFile > maxNFiles) {
            std::cout << "user has reached the limit file" << std::endl;
            return false;
        }

        UserMap[m_user].lastActiveTime = currentTime;
    }

    return true;
}

void Session::commandRouter(size_t t_bytesTransferred) {
    std::cout << __FUNCTION__ << "(" << t_bytesTransferred << ")"
              << ", in_avail = " << m_request_buf_read.in_avail() << ", size = "
              << m_request_buf_read.size() << ", max_size = " << m_request_buf_read.max_size() << "." << std::endl;
    boost::asio::streambuf::const_buffers_type bufs = m_request_buf_read.data();
    std::string line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + t_bytesTransferred);
    std::cout << "line: " << line << std::endl;
    std::istream requestStream(&m_request_buf_read);
    requestStream >> m_command;
    if (m_command.empty()) {
        std::string error = "\nError COMMAND_IS_EMPTY";
        writeToClient(error);
        return;
    }

    requestStream >> m_user;
    if (m_user.empty()) {
        std::string error = "\nError USERNAME_IS_EMPTY";
        writeToClient(error);
        return;
    }

    if (!checkUserActive()) {
        std::string error = "\nError USER_REACHED_LIMIT";
        writeToClient(error);
        return;
    }


    if (m_command == "get") {
        requestStream >> m_fileName;
        if (m_fileName.empty()) {
            std::string error = "\nError FILENAME_IS_EMPTY";
            writeToClient(error);
            return;
        }

        getCommandHandler(m_fileName);
        return;
    }


    if (m_command == "zip" || m_command == "zip-and-get") {
        requestStream >> m_fileName;
        requestStream >> m_fileSize;
        if (m_fileName.empty() || !m_fileSize) {
            std::string error = "\nError ZIP_WRONG_ARGS";
            writeToClient(error);
            return;
        }



        // убирать 2 /n
        requestStream.read(m_read_buf.data(), 2);

        auto pos = m_fileName.find_last_of('\\');
        if (pos != std::string::npos)
            m_fileName = m_fileName.substr(pos + 1);

        createFile(m_fileName + ".tmp");

        // записать лишние байты в файл
        do {
            requestStream.read(m_read_buf.data(), m_read_buf.size());
            std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            m_outputFile.write(m_read_buf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            m_outputFile.close();
            return zipLogic();
        }

        zipCommandHandler(m_command);
        return;
    }

    if (m_command == "unzip" || m_command == "unzip-and-get") {
        requestStream >> m_fileName;
        requestStream >> m_fileSize;
        if (m_fileName.empty() || !m_fileSize) {
            std::string error = "\nError UNZIP_WRONG_ARGS";
            writeToClient(error);
            return;
        }

        // убирать 2 /n
        requestStream.read(m_read_buf.data(), 2);

        auto pos = m_fileName.find_last_of('\\');
        if (pos != std::string::npos)
            m_fileName = m_fileName.substr(pos + 1);

        std::string tempFile = m_fileName + ".tmp";
        createFile(tempFile);

        // записать лишние байты в файл
        do {
            requestStream.read(m_read_buf.data(), m_read_buf.size());
            std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            m_outputFile.write(m_read_buf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            m_outputFile.close();
            return unZipLogic();
        }

        unZipCommandHandler(m_command);
        return;
    }

    std::string error = "\nError UNKNOWN_COMMAND";
    writeToClient(error);
}

void Session::zipLogic() {
    std::cout << "Received file: " << m_fileName << std::endl;
    std::string compressedFileName = getFileName(m_fileName, "zip");
    std::string tempFile = m_fileName + ".tmp";
    compress(tempFile, compressedFileName);
    remove(tempFile);
    if (m_command == "zip") {
        std::string ok("OK " + compressedFileName);
        std::cout << ok << std::endl;
        writeToClient(ok);
    } else if (m_command  == "zip-and-get"){
        getCommandHandler(compressedFileName);
    }
}

void Session::unZipLogic() {
    std::cout << "Received file: " << m_fileName << std::endl;
    std::string deCompressedFileName = getFileName(m_fileName, "unzip");
    std::string tempFile = m_fileName + ".tmp";
    decompress(tempFile, deCompressedFileName);
    remove(tempFile);
    if (m_command == "unzip") {
        std::string ok("OK " + deCompressedFileName);
        std::cout << ok << std::endl;
        writeToClient(ok);
    } else if (m_command  == "unzip-and-get"){
        getCommandHandler(deCompressedFileName);
    }
}

void Session::zipCommandHandler(const std::string& command) {
    auto self = shared_from_this();
    std::cout << "read wait" << std::endl;
    m_socket.async_read_some(boost::asio::buffer(m_read_buf.data(), m_read_buf.size()),
                             boost::asio::bind_executor(my_strand, [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     std::cout << "recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         m_outputFile.write(m_read_buf.data(), static_cast<std::streamsize>(bytes));

                                         std::cout << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

                                         if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
                                             m_outputFile.close();
                                             return zipLogic();
                                         }
                                     }

                                     zipCommandHandler(command);
                                 } else {
                                     handleError(__FUNCTION__, ec);
                                 }
                             }));
}

void Session::unZipCommandHandler(const std::string& command) {
    auto self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_read_buf.data(), m_read_buf.size()),
                             boost::asio::bind_executor(my_strand, [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     std::cout << "recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         m_outputFile.write(m_read_buf.data(), static_cast<std::streamsize>(bytes));

                                         std::cout << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

                                         if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
                                            m_outputFile.close();
                                            return unZipLogic();
                                         }
                                     }

                                     unZipCommandHandler(command);
                                 } else {
                                     handleError(__FUNCTION__, ec);
                                 }
                             }));
}

void Session::createFile(std::string const &fileName) {
    m_outputFile.open(fileName, std::ios_base::binary);
    if (!m_outputFile) {
        std::cout << __LINE__ << ": Failed to create: " << m_fileName << std::endl;;
        return;
    }
}

void Session::remove(std::string& fileName) {
    const char *c = fileName.c_str();
    if( std::remove( c ) != 0 )
        perror( "Error deleting file" );
    else
        puts( "File successfully deleted" );
}

void Session::handleError(std::string const &t_functionName, boost::system::error_code const &t_ec) {
    std::cout << __FUNCTION__ << " in " << t_functionName << " due to "
              << t_ec << " " << t_ec.message() << std::endl;
}

void Session::writeToClient(std::string &message) {
    auto self(shared_from_this());
    boost::asio::async_write(m_socket, boost::asio::buffer(message),
                             boost::asio::bind_executor(my_strand, [this, self](boost::system::error_code ec, std::size_t)
                             {
                                 if (!ec)
                                 {
                                 } else {
                                     handleError(__FUNCTION__, ec);
                                 }
                             }));
}

std::string Session::getFileName(const std::string& fileName, const std::string type) {
    std::string path;
    if (type == "zip") {
        path = fileName + ".sev";
    } else if (type == "unzip") {
        size_t lastOf = fileName.find_last_of(".");
        path = fileName.substr(0, lastOf);
    }

    return path;
}

void Session::getCommandHandler(const std::string& fileName) {
    std::filesystem::path filePath;
    if (find_file(fileName, filePath)) {
        openFile(fileName); // open the file
        // and prepare m_request for request to client

        writeBuffer(m_request_buf_send); // send to client;
    } else {
        std::string error = "\nError FILE_NOT_FOUND";
        writeToClient(error);
    }
}


void Session::sendFile(boost::system::error_code t_ec) {
    if (!t_ec) {
        if (m_sourceFile) {
            m_sourceFile.read(m_send_buf.data(), m_send_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            std::stringstream ss;
            ss << "Send " << m_sourceFile.gcount() << " bytes, total: "
               << m_sourceFile.tellg() << " bytes";
            std::cout << ss.str() << std::endl;
            auto buf = boost::asio::buffer(m_send_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        } else {
            std::cout << "конец!" << std:: endl;
            m_sourceFile.close();
//            boost::asio::streambuf rmessage;
//            std::ostream response(&rmessage);
//            response << "\n\n OK" << " " << m_fileName;
//            writeBuffer(rmessage, true);
        }
    } else {
        std::cout << "Error: " << t_ec.message();
    }
}

void Session::openFile(std::string const& t_path)
{
    m_sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (m_sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path);

    // получить размер файла
    m_sourceFile.seekg(0, m_sourceFile.end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, m_sourceFile.beg);


    std::ostream requestStream(&m_request_buf_send);
    boost::filesystem::path p(t_path);
    m_fileName = p.filename().string();
    //requestStream << p.filename().string() << "\n" << fileSize << "\n\n";
    requestStream << "Ready " << m_fileName << " " << fileSize << "\n\n";
    std::cout << "Request size: " << m_request_buf_send.size() << std::endl;
}

bool Session::find_file(const std::string & file_name, // search for this name,
                         std::filesystem::path & path_found )            // placing path here if found
{
    auto currentPath = std::filesystem::current_path();
    std::cout << "dir_path: " << currentPath.c_str() << " " << file_name << std::endl;

    if ( !exists(currentPath) ) return false;
    std::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for ( std::filesystem::directory_iterator itr(currentPath);
          itr != end_itr;
          ++itr )
    {
        std::cout << "f:" << itr->path().filename() << std::endl;

        if ( itr->path().filename() == file_name ) // see below
        {
            path_found = itr->path();
            return true;
        }
    }
    return false;
}

