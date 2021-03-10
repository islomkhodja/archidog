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
                             if (verbose_flag) std::cout << "read_commands(): already read" << std::endl;
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
            std::cout << "checkUserActive(): many requests" << std::endl;
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
            std::cout << "checkUserActive(): user has reached the limit file" << std::endl;
            return false;
        }

        UserMap[m_user].lastActiveTime = currentTime;
    }

    return true;
}

void Session::commandRouter(size_t t_bytesTransferred) {
    if (verbose_flag) std::cout << __FUNCTION__ << "(" << t_bytesTransferred << ")"
                                << ", in_avail = " << m_request_buf_read.in_avail() << ", size = "
                                << m_request_buf_read.size() << ", max_size = " << m_request_buf_read.max_size() << "." << std::endl;
    if (verbose_flag) {
        boost::asio::streambuf::const_buffers_type bufs = m_request_buf_read.data();
        std::string line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + t_bytesTransferred);
        std::cout << "line: " << line << std::endl;
    }
    std::istream requestStream(&m_request_buf_read);
    requestStream >> m_command;
    if (m_command.empty()) {
        sendError("COMMAND_IS_EMPTY", __LINE__, "commandRouter");
        return;
    }

    requestStream >> m_user;
    if (m_user.empty()) {
        sendError("USERNAME_IS_EMPTY", __LINE__, "commandRouter");
        return;
    }

    if (!checkUserActive()) {
        sendError("USER_REACHED_LIMIT", __LINE__, "commandRouter");
        return;
    }


    if (m_command == "get") {
        requestStream >> m_fileName;
        if (m_fileName.empty()) {
            sendError("FILENAME_IS_EMPTY", __LINE__, "commandRouter");
            return;
        }

        getCommandHandler(m_fileName);
        return;
    }


    if (m_command == "zip" || m_command == "zip-and-get") {
        requestStream >> m_fileName;
        requestStream >> m_fileSize;
        if (m_fileName.empty() || !m_fileSize) {
            sendError("ZIP_WRONG_ARGS", __LINE__, "commandRouter");
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
            if (verbose_flag) std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
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
            sendError("UNZIP_WRONG_ARGS", __LINE__, "commandRouter");
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
            if (verbose_flag) std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            m_outputFile.write(m_read_buf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            m_outputFile.close();
            return unZipLogic();
        }

        unZipCommandHandler(m_command);
        return;
    }

    sendError("UNKNOWN_COMMAND", __LINE__, "commandRouter");
}

void Session::zipLogic() {
    if (verbose_flag) std::cout << "zipLogic(): Received file: " << m_fileName << std::endl;
    std::string compressedFileName = getFileName(m_fileName, "zip");
    std::string tempFileName = m_fileName + ".tmp";

    compress(tempFileName, compressedFileName);
    remove(tempFileName);
    if (m_command == "zip") {
        std::string ok("OK " + compressedFileName);
        if (verbose_flag) std::cout << ok << std::endl;
        writeToClient(ok);
    } else if (m_command  == "zip-and-get"){
        getCommandHandler(compressedFileName);
    }
}

void Session::unZipLogic() {
    if (verbose_flag) std::cout << "unZipLogic(): Received file: " << m_fileName << std::endl;
    std::string deCompressedFileName = getFileName(m_fileName, "unzip");
    std::string tempFileName = m_fileName + ".tmp";
    decompress(tempFileName, deCompressedFileName);
    remove(tempFileName);
    if (m_command == "unzip") {
        std::string ok("OK " + deCompressedFileName);
        if (verbose_flag) std::cout << "unZipLogic(): " << ok << std::endl;
        writeToClient(ok);
    } else if (m_command  == "unzip-and-get"){
        getCommandHandler(deCompressedFileName);
    }
}

void Session::zipCommandHandler(const std::string& command) {
    auto self = shared_from_this();
    if (verbose_flag) std::cout << "zipCommandHandler(): read wait" << std::endl;
    m_socket.async_read_some(boost::asio::buffer(m_read_buf.data(), m_read_buf.size()),
                             boost::asio::bind_executor(my_strand, [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     if (verbose_flag) std::cout << "zipCommandHandler(): recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         m_outputFile.write(m_read_buf.data(), static_cast<std::streamsize>(bytes));

                                         if (verbose_flag) std::cout << "zipCommandHandler() " << " recv " << m_outputFile.tellp() << " bytes" << std::endl;

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
                                     if (verbose_flag) std::cout << "unZipCommandHandler(): recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         if (verbose_flag) m_outputFile.write(m_read_buf.data(), static_cast<std::streamsize>(bytes));

                                         if (verbose_flag) std::cout << "unZipCommandHandler(): " << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

                                         if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
                                            m_outputFile.close();
                                            return unZipLogic();
                                         }
                                     }

                                     unZipCommandHandler(command);
                                 } else {
                                     handleError("unZipCommandHandler", ec);
                                 }
                             }));
}

void Session::createFile(std::string const &fileName) {
    m_outputFile.open(fileName, std::ios_base::binary);
    if (!m_outputFile) {
        std::cout << "createFile()" << ": Failed to create: " << m_fileName << std::endl;;
        return;
    }
}

void Session::remove(std::string& fileName) {
    const char *c = fileName.c_str();
    if( std::remove( c ) != 0 )
        perror( "remove(): Error deleting file" );
    else
        if (verbose_flag) puts( "remove(): File successfully deleted" );
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
                                     handleError("writeToClient()", ec);
                                 }
                             }));
}

void Session::sendError(const std::string &errorMessage, int line, const std::string& at) {
    std::string error = "\nError " + errorMessage;
    std::cout << at << "(" << line << "): " << error << std::endl;
    writeToClient(error);
    return;
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
    boost::filesystem::path filePath;
    if (find_file(fileName, filePath)) {
        openFile(fileName); // open the file
        // and prepare m_request for request to client

        writeBuffer(m_request_buf_send); // send to client;
    } else {
        sendError("FILE_NOT_FOUND", __LINE__, "getCommandHandler()");
        return;
    }
}


void Session::sendFile(boost::system::error_code t_ec) {
    if (!t_ec) {
        if (m_sourceFile) {
            m_sourceFile.read(m_send_buf.data(), m_send_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "sendFile(): Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            if (verbose_flag) {
                std::stringstream ss;
                ss << "sendFile(): Send " << m_sourceFile.gcount() << " bytes, total: "
                   << m_sourceFile.tellg() << " bytes";
                std::cout << ss.str() << std::endl;
            }
            auto buf = boost::asio::buffer(m_send_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        } else {
            if (verbose_flag) std::cout << "sendFile(): end!" << std:: endl;
            m_sourceFile.close();
        }
    } else {
        std::cout << "sendFile(): Error: " << t_ec.message();
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
    requestStream << "Ready " << m_fileName << " " << fileSize << "\n\n";
    if (verbose_flag) std::cout << "Request size: " << m_request_buf_send.size() << std::endl;
}

bool Session::find_file(const std::string & file_name, // search for this name,
                         boost::filesystem::path & path_found )            // placing path here if found
{
    auto currentPath = boost::filesystem::current_path();
    if (verbose_flag) std::cout << "dir_path: " << currentPath.c_str() << " " << file_name << std::endl;

    if ( !exists(currentPath) ) return false;
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for ( boost::filesystem::directory_iterator itr(currentPath);
          itr != end_itr;
          ++itr )
    {
        if (verbose_flag) std::cout << "find:" << itr->path().filename() << std::endl;

        if ( itr->path().filename() == file_name ) // see below
        {
            path_found = itr->path();
            return true;
        }
    }
    return false;
}

