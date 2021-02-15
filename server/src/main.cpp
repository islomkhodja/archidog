#include <iostream>
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>

#include "ArchiveServer.h"


int main()
{
    setlocale(LC_ALL, "Russian");
    try {
        size_t threadNum = 8;
        boost::asio::io_context ioContext;
        boost::asio::executor_work_guard<decltype(ioContext.get_executor())> work { ioContext.get_executor() };
        boost::thread_group threadPool;
        for (int i = 0; i < threadNum; i++) {
            threadPool.create_thread(
                    boost::bind(&boost::asio::io_service::run, &ioContext)
            );
        }

        ArchiveServer server(ioContext, 9898, "../server_files");

//        ioContext.run();
        threadPool.join_all();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}