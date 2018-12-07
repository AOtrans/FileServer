#include <QCoreApplication>
#include "http/httpserver.h"
#include <QLockFile>
#include "common.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //make sure singal instance in runtime
    QLockFile lockFile(SINGAL_LOCKER_PATH);

    if (!lockFile.tryLock(100))
    {
        qDebug() << "service already running";
        return 1;
    }

    HttpServer server;
    if(!server.listen(58890))
    {
        qDebug()<<"open service false！";
        exit(-1);
    }
    qDebug()<<"open service success！";
    return a.exec();
}
