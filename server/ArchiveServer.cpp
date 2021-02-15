#include <iostream>

#include <boost/asio/read_until.hpp>
#include <boost/filesystem.hpp>

#include "ArchiveServer.h"

Session::Session(TcpSocket t_socket)
        : m_socket(std::move(t_socket)) {
}


void Session::handle_read() {
    auto self = shared_from_this();
    async_read_until(m_socket, m_requestBuf_, "\n",
                     [this, self](boost::system::error_code ec, size_t bytes) {
                         if (!ec) {
                             std::cout << "handle_read(); уже прочитал" << std::endl;
                             commandRouter(bytes);
                         }

                         else
                             handleError(__FUNCTION__, ec);
                     });
}

bool Session::find_file( const boost::filesystem::path & dir_path,         // in this directory,
                         const std::string & file_name, // search for this name,
                         boost::filesystem::path & path_found )            // placing path here if found
{
    if ( !exists( dir_path ) ) return false;
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for ( boost::filesystem::directory_iterator itr( dir_path );
          itr != end_itr;
          ++itr )
    {
        if ( is_directory(itr->status()) )
        {
            if ( find_file( itr->path(), file_name, path_found ) ) return true;
        }
        else if ( itr->path().filename() == file_name ) // see below
        {
            path_found = itr->path();
            return true;
        }
    }
    return false;
}


void Session::commandRouter(size_t t_bytesTransferred) {
    std::cout << __FUNCTION__ << "(" << t_bytesTransferred << ")"
              << ", in_avail = " << m_requestBuf_.in_avail() << ", size = "
              << m_requestBuf_.size() << ", max_size = " << m_requestBuf_.max_size() << "." << std::endl;
    boost::asio::streambuf::const_buffers_type bufs = m_requestBuf_.data();
    std::string line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + t_bytesTransferred);
    std::cout << "line: " << line << std::endl;
    std::istream requestStream(&m_requestBuf_);
    requestStream >> m_command;


    if (m_command == "get") {
        requestStream >> m_fileName;
        getCommandHandler(m_fileName);
        return;
    }


    if (m_command == "zip" || m_command == "zip-and-get") {
        requestStream >> m_fileName;
        requestStream >> m_fileSize;

        // убирать 2 /n
        requestStream.read(m_buf.data(), 2);

        auto pos = m_fileName.find_last_of('\\');
        if (pos != std::string::npos)
            m_fileName = m_fileName.substr(pos + 1);

        createFile(m_fileName);

        // записать лишние байты в файл
        do {
            requestStream.read(m_buf.data(), m_buf.size());
            std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            m_outputFile.write(m_buf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);
        zipCommandHandler(m_command);
        return;
    }

    if (m_command == "unzip" || m_command == "unzip-and-get") {
        requestStream >> m_fileName;
        requestStream >> m_fileSize;

        // убирать 2 /n
        requestStream.read(m_buf.data(), 2);

        auto pos = m_fileName.find_last_of('\\');
        if (pos != std::string::npos)
            m_fileName = m_fileName.substr(pos + 1);

        createFile(m_fileName);

        // записать лишние байты в файл
        do {
            requestStream.read(m_buf.data(), m_buf.size());
            std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
            m_outputFile.write(m_buf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        unZipCommandHandler(m_command);
        return;
    }

    boost::asio::streambuf rmessage;
    std::ostream response(&rmessage);
    response << "\nERROR" << " " << "UNKNOWN_COMMAND";
    writeBuffer(rmessage, true);
}

void Session::sendFile(boost::system::error_code t_ec) {
    if (!t_ec) {
        if (m_sourceFile) {
            m_sourceFile.read(s_buf.data(), s_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            std::stringstream ss;
            ss << "Send " << m_sourceFile.gcount() << " bytes, total: "
               << m_sourceFile.tellg() << " bytes";
            std::cout << ss.str() << std::endl;
            auto buf = boost::asio::buffer(s_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        } else {
            std::cout << "конец!" << std:: endl;
            boost::asio::streambuf rmessage;
            std::ostream response(&rmessage);
            response << "\n READY" << " " << m_fileName;
            writeBuffer(rmessage, true);
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


    std::ostream requestStream(&m_request);
    boost::filesystem::path p(t_path);
    m_fileName = p.filename().string();
    //requestStream << p.filename().string() << "\n" << fileSize << "\n\n";
    requestStream << m_fileName << " " << fileSize << "\n\n";
    std::cout << "Request size: " << m_request.size() << std::endl;
}

void Session::zipCommandHandler(const std::string& command) {
    auto self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                             [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     std::cout << "recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(bytes));

                                         std::cout << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

                                         if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
                                             std::cout << "Received file: " << m_fileName << std::endl;
                                             std::string compressedFileName = getFileName(m_fileName, "zip");
                                             compress(m_fileName, compressedFileName);
                                             remove(m_fileName);
                                             if (command == "zip") {
                                                 std::string ok("OK " + compressedFileName);
                                                 writeToClient(ok);
                                             } else if (command  == "zip-and-get"){
                                                 getCommandHandler(compressedFileName);
                                             }
                                             return;
                                         }
                                     }

                                     zipCommandHandler(command);
                                 } else {
                                     handleError(__FUNCTION__, ec);
                                 }
                             });
}

void Session::unZipCommandHandler(const std::string& command) {
    auto self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                             [this, self, command](boost::system::error_code ec, size_t bytes) {
                                 if (!ec) {
                                     std::cout << "recv " << bytes << " from response" << std::endl;
                                     if (bytes > 0) {
                                         m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(bytes));

                                         std::cout << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

                                         if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
                                             std::cout << "Received file: " << m_fileName << std::endl;
                                             std::string deCompressedFileName = getFileName(m_fileName, "unzip");
                                             decompress(m_fileName, deCompressedFileName);
                                             remove(m_fileName);
                                             if (command == "unzip") {
                                                 std::string ok("OK " + deCompressedFileName);
                                                 writeToClient(ok);
                                             } else if (command  == "unzip-and-get"){
                                                 getCommandHandler(deCompressedFileName);
                                             }
                                             return;
                                         }
                                     }

                                     unZipCommandHandler(command);
                                 } else {
                                     handleError(__FUNCTION__, ec);
                                 }
                             });
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


void Session::doReadFileContent(size_t t_bytesTransferred) {
    if (t_bytesTransferred > 0) {
        m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(t_bytesTransferred));

        std::cout << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            std::cout << "Received file: " << m_fileName << std::endl;
            return;
        }
    }
    auto self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                             [this, self](boost::system::error_code ec, size_t bytes) {
                                 doReadFileContent(bytes);
                             });
}


void Session::handleError(std::string const &t_functionName, boost::system::error_code const &t_ec) {
    std::cout << __FUNCTION__ << " in " << t_functionName << " due to "
              << t_ec << " " << t_ec.message() << std::endl;
}

void Session::writeToClient(std::string &message) {
    auto self(shared_from_this());
    boost::asio::async_write(m_socket, boost::asio::buffer(message),
                             [this, self](boost::system::error_code ec, std::size_t)
                             {
                                 if (!ec)
                                 {
                                 } else {
                                     handleError(__FUNCTION__, ec);
                                 }
                             });
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

float Session::compress(std::basic_string<char> fname, std::basic_string<char> cname) {
    std::fstream file;            // original file
    std::fstream compressed;      // compressed file
    char character;          // читаемый символ
    char next_character;     // следующий читаемый символ
    int fsize = 0;           // размер файла
    int frequency = 1;       // уникальность символов
    int write_pos = 0;

    file.open(fname, std::ios::in | std::ios::ate | std::ios::binary);
    compressed.open(cname, std::ios::out | std::ios::trunc | std::ios::binary);
    fsize = file.tellg();

    for(int i = 0; i < fsize; i++)
    {
        file.seekg(i, std::ios::beg);      // safety net
        file.read((char*)&character, sizeof(char)); // получить текущую символ
        next_character = file.peek();

        if(next_character != character)
        {
            compressed.seekp(write_pos, std::ios::beg);
            compressed.write((char*)&frequency, sizeof(char));
            compressed.seekp(write_pos + 1, std::ios::beg);
            compressed.write((char*)&character, sizeof(char));
            write_pos += 2;
            frequency = 0;
        }
        frequency++;
    }

    file.close();
    compressed.close();

    return (write_pos / float(fsize));
}

void Session::decompress(std::basic_string<char> fname, const std::basic_string<char> uname) {
    std::fstream file;
    std::fstream ufile;
    char character;
    int frequency = 0;
    int fsize = 0;
    int write_pos = 0;

    file.open(fname, std::ios::ate | std::ios::in | std::ios::binary);
    ufile.open(uname, std::ios::trunc | std::ios::out | std::ios::binary);
    fsize = file.tellg();

    for(int i = 0; i < fsize; i += 2)
    {
        file.seekg(i, std::ios::beg);
        file.read((char*)&frequency, sizeof(char));
        file.seekg(i + 1, std::ios::beg);
        file.read((char*)&character, sizeof(char));

        for(int j = 0; j < frequency; j++)
        {
            ufile.seekp(write_pos, std::ios::beg);
            ufile.write((char*)&character, sizeof(char));
            write_pos++;
        }
    }
    file.close();
    ufile.close();
}

void Session::getCommandHandler(const std::string& fileName) {
    boost::filesystem::path filePath;
    if (find_file(boost::filesystem::path("../new_files"), fileName, filePath)) {
        openFile(fileName);
        writeBuffer(m_request);
    } else {
        boost::asio::streambuf rmessage;
        std::ostream response(&rmessage);
        response << "\nERROR" << " " << "FILE_NOT_FOUND";
        writeBuffer(rmessage, true);
    }
}

ArchiveServer::ArchiveServer(IoService &t_ioService, short t_port, std::string const &t_workingDirectory)
        : m_socket(t_ioService),
          m_acceptor(t_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), t_port)),
          m_workDirectory(t_workingDirectory) {
    std::cout << "Server started\n";

    createWorkDirectory();

    doAccept();
}

void ArchiveServer::doAccept() {
    std::cout << __FUNCTION__ << "пытаемся подключит клиента к серверу" << std::endl;
    m_acceptor.async_accept(m_socket,
                            [this](boost::system::error_code ec) {
                                if (!ec) {
                                    std::cout << "async_accept::" <<  "клиент подключился!" << std::endl;
                                    std::make_shared<Session>(std::move(m_socket))->start();
                                }

                                doAccept();
                            });
}

void ArchiveServer::createWorkDirectory() {
    using namespace boost::filesystem;
    auto currentPath = path(m_workDirectory);

    if (!exists(currentPath) && !create_directory(currentPath)) {
        std::cout << "Не удалось создать рабочую папку: " << m_workDirectory << std::endl;;
    } else {
        std::cout << "папка создан или уже существует: " << m_workDirectory << std::endl;
    }
    current_path(currentPath);
}

