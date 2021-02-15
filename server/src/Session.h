
#ifndef ARCHIVE_SERVER_SESSION_H
#define ARCHIVE_SERVER_SESSION_H

#include <iostream>
#include <array>
#include <fstream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

class Session
        : public std::enable_shared_from_this<Session>
{
public:
    using TcpSocket = boost::asio::ip::tcp::socket;

    Session(TcpSocket t_socket);

    void start()
    {
        handle_read();
    }

private:
    void handle_read();
    void commandRouter(size_t t_bytesTransferred);
    std::string getFileName(std::string const &fileName, std::string const type);
    void createFile(std::string const &fileName);
    void doReadFileContent(size_t t_bytesTransferred);
    void handleError(std::string const& t_functionName, boost::system::error_code const& t_ec);
    float compress(std::basic_string<char> fname, const std::basic_string<char> cname);
    void decompress(std::basic_string<char> fname, const std::basic_string<char> uname);
    void sendFile(boost::system::error_code ec);


    TcpSocket m_socket;
    enum { MaxLength = 40960 };
    std::array<char, MaxLength> m_buf;
    enum { MessageSize = 1024 };
    std::array<char, MessageSize> s_buf;
    boost::asio::streambuf m_requestBuf_;
    std::ofstream m_outputFile;
    std::ifstream m_sourceFile;
    size_t m_fileSize;
    std::string m_fileName;
    boost::asio::streambuf m_request;
    std::string m_command;

    void writeToClient(std::string &message);

    void remove(std::string &fileName);

    bool
    find_file(const boost::filesystem::path &dir_path, const std::string &file_name,
              boost::filesystem::path &path_found);

    void openFile(const std::string &t_path);
    template<class Buffer>
    void writeBuffer(Buffer &t_buffer, const bool once = false);

    void zipCommandHandler(const std::string &command);
    void getCommandHandler(const std::string &fileName);

    void unZipCommandHandler(const std::string &command);
};

template<class Buffer>
void Session::writeBuffer(Buffer& t_buffer, const bool once)
{
    auto self(shared_from_this());
    boost::asio::async_write(m_socket,
                             t_buffer,
                             [this, self, once](boost::system::error_code ec, size_t length)
                             {
                                 if (!ec && !once) {
                                     std::cout << "ok отправили " << length << " байтов. Если что еще раз отправим" << std::endl;
                                     sendFile(ec);
                                 }
                             });
}


#endif //ARCHIVE_SERVER_SESSION_H
