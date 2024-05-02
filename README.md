# archive-server

Это асинхронный архив-сервер с пулом потоков.


![DALL·E 2024-05-02 23 07 14 - A logo for an archive server project featuring a dog  The logo should be simple and modern, focusing on a stylized, abstract representation of a dog  ](https://github.com/islomkhodja/archidog/assets/4748226/d16169ff-10ed-4d54-b0c5-a2815e7c63b5)

## Описание

Сервер будет архивировать поток, получаемый от клиента и хранит некоторые файлы:

## Install
https://www.ics.uci.edu/~pattis/common/handouts/mingweclipse/mingw.html
https://gist.github.com/sim642/29caef3cc8afaa273ce6
https://stackoverflow.com/questions/36519453/setup-boost-in-clion

## Команды

+ "**zip** `<name> <bytes_size>`" - далее клиент шлёт байтовый поток, сервер его архивирует и сохраняет файл у себя.

+ "**get** `<name>`" – клиент требует заархивированный файл с именем,

+ "**zip-and-get** `<name> <bytes_size>`" – сервер одновременно принимает, архивирует и отсылает сжатые данные обратно клиенту.

+ "**unzip** `<name> <byte_size>`" - далее клиент шлёт байтовый поток, сервер его разархивирует и сохраняет файл у себя.

+ "**unzip-and-get** `<name> <bytes_size>`" – сервер одновременно принимает, разархивирует и отсылает сжатые данные обратно клиенту.
### Конфигурация сервера
Запуск ./server.exe
  
| Аргументы | Описание |
| --- | --- |
| \-d \-\-dir=\<path\> | путь до папки сервера, где сохранять файлы \(future\) |
| \-i \-\-ip=\<IP\> | IP адрес, на котором работает listener сервер |
| \-p \-\-port=\<uint\> | порт, на котором работает listener сервера |
| \-n \-\-maxnfiles=\<uint\> | максимальное число файлов, что клиент может сохранить |
| \-r \-\-reqspermin=\<uint\> | максимальное количество обращений на архивацию от 1 клиента в 1 минуту |
| \-w \-\-workers=\<uint\> | число потоков |
| \-v \-\-verbose | печатать в stdout подробно производимые операции сервером \(stderr\), без этого флага печатать только ошибки |
| \-h \-\-help | распечатать в консоль аргументы, как использовать программу |
