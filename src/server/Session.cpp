#include "globals.h"
#include "Session.h"
#include "../common/rle.h"

std::map<std::string, UserSettings> UserMap;

Session::Session(TcpSocket t_socket, boost::asio::io_context& io, boost::filesystem::path m_workDirectory)
    :   socket(std::move(t_socket)),
        workDirectory(m_workDirectory),
        ioContext(io),
        myStrand(ioContext) {
}


void Session::readCommands() {
    auto self = shared_from_this();
    async_read_until(socket, requestBufRead, "\n\n",
                     boost::asio::bind_executor(myStrand, [this, self](boost::system::error_code ec, size_t bytes) {
                         if (!ec) {
                             if (verbose_flag) std::cout << "read_commands(): already read" << std::endl;
                             commandRouter(bytes);
                         }

                         else
                             handleError(__FUNCTION__, ec);
                     }));
}

bool Session::checkUserActive() {
    if (UserMap.find(user) == UserMap.end()) {
        UserMap[user] = { 1, time(nullptr), 0 };
    } else {
        std::time_t currentTime = time(nullptr);
        double diff = difftime(currentTime, UserMap[user].lastActiveTime);
        if (diff > 60) {
            UserMap[user].userReqCount = 1;
        } else {
            UserMap[user].userReqCount += 1;
        };

        if (UserMap[user].userReqCount > reqsPerMin) {
            std::cout << "checkUserActive(): many requests" << std::endl;
            return false;
        }

        if (command  == "zip"
        || command == "zip-and-get"
        || command == "unzip-and-get"
        || command == "unzip")
        {
            UserMap[user].userMaxFile += 1;
        }


        if (UserMap[user].userMaxFile > maxNFiles) {
            std::cout << "checkUserActive(): user has reached the limit file" << std::endl;
            return false;
        }

        UserMap[user].lastActiveTime = currentTime;
    }

    return true;
}

void Session::commandRouter(size_t t_bytesTransferred) {
    if (verbose_flag) std::cout << __FUNCTION__ << "(" << t_bytesTransferred << ")"
                                << ", in_avail = " << requestBufRead.in_avail() << ", size = "
                                << requestBufRead.size() << ", max_size = " << requestBufRead.max_size() << "." << std::endl;
    if (verbose_flag) {
        boost::asio::streambuf::const_buffers_type bufs = requestBufRead.data();
        std::string line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + t_bytesTransferred);
        std::cout << "line: " << line << std::endl;
    }
    std::istream requestStream(&requestBufRead);
    requestStream >> command;
    if (command.empty()) {
        sendError("COMMAND_IS_EMPTY", __LINE__, "commandRouter");
        return;
    }

    requestStream >> user;
    if (user.empty()) {
        sendError("USERNAME_IS_EMPTY", __LINE__, "commandRouter");
        return;
    }

    if (!checkUserActive()) {
        sendError("USER_REACHED_LIMIT", __LINE__, "commandRouter");
        return;
    }


    if (command == "get") {
        requestStream >> fileName;
        if (fileName.empty()) {
            sendError("FILENAME_IS_EMPTY", __LINE__, "commandRouter");
            return;
        }

        getCommandHandler(fileName);
        return;
    }


    if (command == "zip" || command == "zip-and-get") {
        requestStream >> fileName;
        requestStream >> fileSize;
        if (fileName.empty() || !fileSize) {
            sendError("ZIP_WRONG_ARGS", __LINE__, "commandRouter");
            return;
        }



        // убирать 2 /n
        requestStream.read(readBuf.data(), 2);

        auto pos = fileName.find_last_of('\\');
        if (pos != std::string::npos)
            fileName = fileName.substr(pos + 1);

        createFile(fileName + ".tmp");

        // записать лишние байты в файл
        do {
            requestStream.read(readBuf.data(), readBuf.size());
            if (verbose_flag) std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            outputFile.write(readBuf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        if (outputFile.tellp() >= static_cast<std::streamsize>(fileSize)) {
            outputFile.close();
            return zipLogic();
        }

        zipCommandHandler(command);
        return;
    }

    if (command == "unzip" || command == "unzip-and-get") {
        requestStream >> fileName;
        requestStream >> fileSize;
        if (fileName.empty() || !fileSize) {
            sendError("UNZIP_WRONG_ARGS", __LINE__, "commandRouter");
            return;
        }

        // убирать 2 /n
        requestStream.read(readBuf.data(), 2);

        auto pos = fileName.find_last_of('\\');
        if (pos != std::string::npos)
            fileName = fileName.substr(pos + 1);

        std::string tempFile = fileName + ".tmp";
        createFile(tempFile);

        // записать лишние байты в файл
        do {
            requestStream.read(readBuf.data(), readBuf.size());
            if (verbose_flag) std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            outputFile.write(readBuf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        if (outputFile.tellp() >= static_cast<std::streamsize>(fileSize)) {
            outputFile.close();
            return unZipLogic();
        }

        unZipCommandHandler(command);
        return;
    }

    sendError("UNKNOWN_COMMAND", __LINE__, "commandRouter");
}

void Session::zipLogic() {
    if (verbose_flag) std::cout << "zipLogic(): Received file: " << fileName << std::endl;
    std::string compressedFileName = getFileName(fileName, "zip");
    std::string tempFileName = fileName + ".tmp";

    compress(tempFileName, compressedFileName);
    remove(tempFileName);
    if (command == "zip") {
        std::string ok("OK " + compressedFileName);
        if (verbose_flag) std::cout << ok << std::endl;
        writeToClient(ok);
    } else if (command  == "zip-and-get"){
        getCommandHandler(compressedFileName);
    }
}

void Session::unZipLogic() {
    if (verbose_flag) std::cout << "unZipLogic(): Received file: " << fileName << std::endl;
    std::string deCompressedFileName = getFileName(fileName, "unzip");
    std::string tempFileName = fileName + ".tmp";
    decompress(tempFileName, deCompressedFileName);
    remove(tempFileName);
    if (command == "unzip") {
        std::string ok("OK " + deCompressedFileName);
        if (verbose_flag) std::cout << "unZipLogic(): " << ok << std::endl;
        writeToClient(ok);
    } else if (command  == "unzip-and-get"){
        getCommandHandler(deCompressedFileName);
    }
}

void Session::zipCommandHandler(const std::string& command) {
    auto self = shared_from_this();
    if (verbose_flag) std::cout << "zipCommandHandler(): read wait" << std::endl;
    socket.async_read_some(boost::asio::buffer(readBuf.data(), readBuf.size()),
                             boost::asio::bind_executor(myStrand, [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     if (verbose_flag) std::cout << "zipCommandHandler(): recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         outputFile.write(readBuf.data(), static_cast<std::streamsize>(bytes));

                                         if (verbose_flag) std::cout << "zipCommandHandler() " << " recv " << outputFile.tellp() << " bytes" << std::endl;

                                         if (outputFile.tellp() >= static_cast<std::streamsize>(fileSize)) {
                                             outputFile.close();
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
    socket.async_read_some(boost::asio::buffer(readBuf.data(), readBuf.size()),
                             boost::asio::bind_executor(myStrand, [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     if (verbose_flag) std::cout << "unZipCommandHandler(): recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         if (verbose_flag) outputFile.write(readBuf.data(), static_cast<std::streamsize>(bytes));

                                         if (verbose_flag) std::cout << "unZipCommandHandler(): " << " recv " << outputFile.tellp() << " bytes" << std::endl;;

                                         if (outputFile.tellp() >= static_cast<std::streamsize>(fileSize)) {
                                            outputFile.close();
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
    outputFile.open(fileName, std::ios_base::binary);
    if (!outputFile) {
        std::cout << "createFile()" << ": Failed to create: " << fileName << std::endl;;
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
    boost::asio::async_write(socket, boost::asio::buffer(message),
                             boost::asio::bind_executor(myStrand, [this, self](boost::system::error_code ec, std::size_t)
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
    if (findFile(fileName, filePath)) {
        openFile(fileName); // open the file
        // and prepare m_request for request to client

        writeBuffer(requestBufSend); // send to client;
    } else {
        sendError("FILE_NOT_FOUND", __LINE__, "getCommandHandler()");
        return;
    }
}


void Session::sendFile(boost::system::error_code t_ec) {
    if (!t_ec) {
        if (sourceFile) {
            sourceFile.read(sendBuf.data(), sendBuf.size());
            if (sourceFile.fail() && !sourceFile.eof()) {
                auto msg = "sendFile(): Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            if (verbose_flag) {
                std::stringstream ss;
                ss << "sendFile(): Send " << sourceFile.gcount() << " bytes, total: "
                   << sourceFile.tellg() << " bytes";
                std::cout << ss.str() << std::endl;
            }
            auto buf = boost::asio::buffer(sendBuf.data(), static_cast<size_t>(sourceFile.gcount()));
            writeBuffer(buf);
        } else {
            if (verbose_flag) std::cout << "sendFile(): end!" << std:: endl;
            sourceFile.close();
        }
    } else {
        std::cout << "sendFile(): Error: " << t_ec.message();
    }
}

void Session::openFile(std::string const& t_path)
{
    sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path);

    // получить размер файла
    sourceFile.seekg(0, sourceFile.end);
    auto fileSize = sourceFile.tellg();
    sourceFile.seekg(0, sourceFile.beg);


    std::ostream requestStream(&requestBufSend);
    boost::filesystem::path p(t_path);
    fileName = p.filename().string();
    requestStream << "Ready " << fileName << " " << fileSize << "\n\n";
    if (verbose_flag) std::cout << "Request size: " << requestBufSend.size() << std::endl;
}

bool Session::findFile(const std::string & file_name, // search for this name,
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

