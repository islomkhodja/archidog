#ifndef BOOST_ARCHIVE_SERVER_H
#define BOOST_ARCHIVE_SERVER_H

#include <iostream>
#include <array>
#include <fstream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "Session.h"

/*!
 * \class ArchiveServer
 *
 * \brief Класс архивного сервера. Наш класс может выполнять 2 работы:
 * 1. Если рабочий каталог не создан, будет создан
 * 2. Запускает Сервер и выполняет работу по приему клиентов.
 *
 * \author Sevara Jumaeva
 */
class ArchiveServer {
public:
    using TcpSocket = boost::asio::ip::tcp::socket;
    using TcpAcceptor = boost::asio::ip::tcp::acceptor;
    using IoContext = boost::asio::io_context;

    /*!
     * Конструктор класса, принимает
     *
     * @param t_ioСontext
     * @param ip
     * @param t_port
     * @param t_workDirectory
     *
     *  инициализирует m_acceptor, m_socket
     */
    ArchiveServer(IoContext& t_ioСontext, boost::asio::ip::address ip, short t_port, std::string const& t_workDirectory);

private:
    /*!
     * \brief метод который создает каталог если еще не создано
     */
    void createWorkDirectory();

    /*! \brief асинхроно выполняет работу по приему клиентов (запускается рекурсивно)
     *
     *  Используем метод async_accept приемника (m_acceptor)
     *  и передадим ему сокет в качестве аргумента.
     *  Если клиент подключился успешно
     *  то создадим для него экземпляр класса Session и сразу же запустим его.
     *  Потом снова повторяем всю эту логику для нового клиента (через рекурсию)
     */
    void doAccept();

    /// сокет
    TcpSocket m_socket;

    /// acceptor — приемник который принимает клиентские подключения.
    TcpAcceptor m_acceptor;

    /// рабочий каталог
    std::string m_workDirectory;

    /*!
     * Boost.Asio использует io_context для общения с сервисом ввода/вывода операционной системы.
     * в классе будем хранить
     */
    IoContext& m_ioContext;
};

#endif //BOOST_ARCHIVE_SERVER_H
