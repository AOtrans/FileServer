#include "httpserver.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpconnection.h"
#include "iostream"
#include <QPixmap>
#include <QTcpServer>
#include <QBuffer>
#include <QFile>
#include<QXmlStreamReader>
#include<QXmlStreamWriter>
#include<QDateTime>
#include<QDir>
#include<QFileInfo>
#include<QFileInfoList>
#include"common.h"

QHash<int, QString> STATUS_CODES;

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
    , m_tcpServer(0)
{
#define STATUS_CODE(num, reason) STATUS_CODES.insert(num, reason);
    // {{{
    STATUS_CODE(100, "Continue")
            STATUS_CODE(101, "Switching Protocols")
            STATUS_CODE(102, "Processing")                 // RFC 2518) obsoleted by RFC 4918
            STATUS_CODE(200, "OK")
            STATUS_CODE(201, "Created")
            STATUS_CODE(202, "Accepted")
            STATUS_CODE(203, "Non-Authoritative Information")
            STATUS_CODE(204, "No Content")
            STATUS_CODE(205, "Reset Content")
            STATUS_CODE(206, "Partial Content")
            STATUS_CODE(207, "Multi-Status")               // RFC 4918
            STATUS_CODE(300, "Multiple Choices")
            STATUS_CODE(301, "Moved Permanently")
            STATUS_CODE(302, "Moved Temporarily")
            STATUS_CODE(303, "See Other")
            STATUS_CODE(304, "Not Modified")
            STATUS_CODE(305, "Use Proxy")
            STATUS_CODE(307, "Temporary Redirect")
            STATUS_CODE(400, "Bad Request")
            STATUS_CODE(401, "Unauthorized")
            STATUS_CODE(402, "Payment Required")
            STATUS_CODE(403, "Forbidden")
            STATUS_CODE(404, "Not Found")
            STATUS_CODE(405, "Method Not Allowed")
            STATUS_CODE(406, "Not Acceptable")
            STATUS_CODE(407, "Proxy Authentication Required")
            STATUS_CODE(408, "Request Time-out")
            STATUS_CODE(409, "Conflict")
            STATUS_CODE(410, "Gone")
            STATUS_CODE(411, "Length Required")
            STATUS_CODE(412, "Precondition Failed")
            STATUS_CODE(413, "Request Entity Too Large")
            STATUS_CODE(414, "Request-URI Too Large")
            STATUS_CODE(415, "Unsupported Media Type")
            STATUS_CODE(416, "Requested Range Not Satisfiable")
            STATUS_CODE(417, "Expectation Failed")
            STATUS_CODE(418, "I\"m a teapot")              // RFC 2324
            STATUS_CODE(422, "Unprocessable Entity")       // RFC 4918
            STATUS_CODE(423, "Locked")                     // RFC 4918
            STATUS_CODE(424, "Failed Dependency")          // RFC 4918
            STATUS_CODE(425, "Unordered Collection")       // RFC 4918
            STATUS_CODE(426, "Upgrade Required")           // RFC 2817
            STATUS_CODE(500, "Internal Server Error")
            STATUS_CODE(501, "Not Implemented")
            STATUS_CODE(502, "Bad Gateway")
            STATUS_CODE(503, "Service Unavailable")
            STATUS_CODE(504, "Gateway Time-out")
            STATUS_CODE(505, "HTTP Version not supported")
            STATUS_CODE(506, "Variant Also Negotiates")    // RFC 2295
            STATUS_CODE(507, "Insufficient Storage")       // RFC 4918
            STATUS_CODE(509, "Bandwidth Limit Exceeded")
            STATUS_CODE(510, "Not Extended")                // RFC 2774
            // }}}
}

HttpServer::~HttpServer()
{
}

void HttpServer::newConnection()
{
    Q_ASSERT(m_tcpServer);
    while(m_tcpServer->hasPendingConnections()) {
        HttpConnection *connection = new HttpConnection(m_tcpServer->nextPendingConnection(), this);
        // connect(connection, SIGNAL(newRequest(HttpRequest*, HttpResponse*)),
        //   this, SIGNAL(newRequest(HttpRequest*, HttpResponse*)));
        connect(connection, SIGNAL(newRequest(HttpRequest*, HttpResponse*)),
                this, SLOT(onRequest(HttpRequest*, HttpResponse*)));
    }
}

bool HttpServer::listen(const QHostAddress &address, quint16 port)
{
    m_tcpServer = new QTcpServer;

    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    return m_tcpServer->listen(address, port);
}

bool HttpServer::listen(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

QMap<QString, QString> analysisVars(const QStringList vars)
{
    QMap<QString, QString> varMap;
    foreach (QString var, vars) {
        QStringList temp = var.split("=");
        if(temp.size()>1)
            varMap.insert(temp[0].toUtf8(), temp[1].toUtf8());
    }

    return varMap;
}

bool analysisDevices(QMap<QString, QString> &deviceMap, QString xmlFilePath)
{
    qDebug() << "start analysis:" << xmlFilePath;
    QFile file(xmlFilePath);

    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "open file failed";
        return false;
    }

    QXmlStreamReader reader(&file);

    reader.readNext();

    while(!reader.atEnd())
    {
        if(reader.isStartElement()&&reader.name() == "Device")
        {
            QXmlStreamAttributes &&attributes = reader.attributes();
            deviceMap.insert(attributes.value("name").toString(), attributes.value("serial").toString());
        }

        reader.readNext();
    }

    file.close();
    return true;
}

QString mkIndexHTML(QMap<QString, QString> &deviceMap)
{
    QString html = "<!DOCTYPE html> \n\
    <html lang='cn'> \n\
    <head> \n\
        <meta charset='UTF-8'> \n\
        <title>Image Server</title> \n\
    </head> \n\
    <body> \n\
<span style='width:100%;text-align:center;display:block;font-size:30px;margin:10px'>报警报表查询</span> \n\
    <table id='tab' border='1px' cellspacing='0px' style='margin: auto;'> \n\
      <thead> \n\
        <tr> \n\
          <th>地点</th> \n\
          <th>起始时间</th> \n\
          <th>结束时间</th> \n\
          <th>筛选</th> \n\
          <th></th> \n\
        </tr> \n\
      </thead> \n\
      <tbody> \n";


    int i = 0;
    foreach (QString key, deviceMap.keys()) {
        QString content = QString("<tr>\n<td name='device' id='%1'>%2</td> \n\
            <td><input type='datetime-local' name='start_time'/></td> \n\
            <td><input type='datetime-local' name='end_time'/></td> \n\
            <td align='center'><input type='checkbox' name='check' checked='checked'></td> \n\
            <td><input type='button' value='查询' onclick='check(%3)' /></td>\n</tr> \n").arg(deviceMap[key]).arg(key).arg(i++);

        html += content;
    }

    html+="</tbody> \n\
    </table> \n\
    <script type='text/javascript'> \n\
            Date.prototype.Format = function (fmt) { \n\
                var o = { \n\
                    'M+': this.getMonth() + 1, \n\
                    'd+': this.getDate(), \n\
                    'h+': this.getHours(), \n\
                    'm+': this.getMinutes(), \n\
                    's+': this.getSeconds(), \n\
                    'q+': Math.floor((this.getMonth() + 3) / 3), \n\
                    'S': this.getMilliseconds() \n\
                }; \n\
                if (/(y+)/.test(fmt)) fmt = fmt.replace(RegExp.$1, (this.getFullYear() + '').substr(4 - RegExp.$1.length)); \n\
                for (var k in o) \n\
                if (new RegExp('(' + k + ')').test(fmt)) fmt = fmt.replace(RegExp.$1, (RegExp.$1.length == 1) ? (o[k]) : (('00' + o[k]).substr(('' + o[k]).length))); \n\
                return fmt; \n\
            } \n\
            var stdate = new Date().Format('yyyy-MM-ddT') + '00:00'; \n\
            var eddate = new Date().Format('yyyy-MM-ddThh:mm'); \n\
            var st = document.getElementsByName('start_time'); \n\
            var et = document.getElementsByName('end_time'); \n\
            for (var j=0;j<st.length;j++) \n\
            { \n\
            st[j].value=stdate \n\
            et[j].value=eddate \n\
            } \n\
      function check(i) { \n\
        var t1 = document.getElementsByName('start_time'); \n\
        var t2 = document.getElementsByName('end_time'); \n\
        var t3 = document.getElementsByName('device'); \n\
        var t4 = document.getElementsByName('check'); \n\
        var target = 'Search?location='+t3[i].id+'&start_time='+t1[i].value+'&end_time='+t2[i].value+'&checked='+t4[i].checked;\n\
        window.location.href = target; \n\
      } \n\
    </script> \n\
    </body> \n\
    </html>";

    return html;
}

QString mkSearchHTML(QMap<QString, QString> &varMap, QString rootPath, QString deviceName)
{
    QStringList temp =  varMap["start_time"].split("T")[0].split("-");
    QDate start_date(temp.at(0).toInt(), temp.at(1).toInt(), temp.at(2).toInt());

    temp = varMap["start_time"].split("T")[1].split(":");
    QTime start_time(temp.at(0).toInt(), temp.at(1).toInt(), 0);

    temp = varMap["end_time"].split("T")[0].split("-");
    QDate end_date(temp.at(0).toInt(), temp.at(1).toInt(), temp.at(2).toInt());

    temp = varMap["end_time"].split("T")[1].split(":");
    QTime end_time(temp.at(0).toInt(), temp.at(1).toInt(), 59);


    QDateTime start_datetime(start_date, start_time);
    QDateTime end_datetime(end_date, end_time);

    int st = start_datetime.toTime_t();
    int ed = end_datetime.toTime_t();




    QString html = QString("<!DOCTYPE html> \n\
    <html lang='cn'> \n\
    <head> \n\
        <meta charset='UTF-8'> \n\
        <title>Image Server</title> \n\
    </head> \n\
    <style> \n\
    td \n\
    { \n\
     white-space: nowrap; \n\
    } \n\
    </style> \n\
    <body> \n\
    <span style='width:100%;text-align:center;display:block;font-size:30px;margin:10px'>%1 %2 to %3</span>").arg(deviceName).arg(varMap["start_time"].replace("T", " ")).arg(varMap["end_time"].replace("T", " "));
            QDir dir(rootPath);

            int i =0;
            foreach (QFileInfo info, dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot)) {
                QStringList dir_date = info.fileName().split("-");
                QDate current_date(dir_date.at(0).toInt(), dir_date.at(1).toInt(), dir_date.at(2).toInt());

                if(current_date>=start_date && current_date<=end_date)
                {
                    QDir subdir(info.filePath());
                    foreach (QFileInfo subinfo, subdir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot)) {
                        if(subinfo.fileName() == varMap["location"])
                        {
                            QDir imgdir(subinfo.filePath() + "/Cameral1");
                            QStringList filter;
                            filter << "*.gif";
                            foreach (QFileInfo imginfo, imgdir.entryInfoList(filter,QDir::Files)) {
                                int cur_time_t = imginfo.fileName().split("_")[2].toInt();
                                QString record = imginfo.fileName().mid(0,imginfo.fileName().lastIndexOf(".gif"));
                                record = record.mid(record.indexOf("_")+1);

                                if(varMap["checked"] == "true" && imginfo.fileName().split("_")[0] == NEW_RECORD)
                                {
                                    continue;
                                }

                                if(cur_time_t>=st && cur_time_t<=ed)
                                {
                                    if(i == 0)
                                    {
                                        html += "<table style='margin: auto;'> \n\
                                                <td valign='top'> \n\
                                                <table border='1px' cellspacing='0px'> \n\
                                                <thead> \n\
                                                  <tr> \n\
                                                    <th>地点</th> \n\
                                                    <th>时间</th> \n\
                                                    <th>图片</th> \n\
                                                  </tr> \n\
                                                </thead> \n\
                                                <tbody> \n";
                                    }
                                    else if(i%60 == 0)
                                    {
                                        html += "</tabody> \n\
                                                </table> \n\
                                                </td> \n\
                                                </table> \n\
                                                <table style='margin: auto;'> \n\
                                                <td valign='top'> \n\
                                                <table border='1px' cellspacing='0px'> \n\
                                                <thead> \n\
                                                <tr> \n\
                                                  <th>地点</th> \n\
                                                  <th>时间</th> \n\
                                                  <th>图片</th> \n\
                                                </tr> \n\
                                                </thead> \n\
                                                <tbody> \n";
                                    }
                                    else if(i%10 == 0)
                                    {
                                        html += "</tabody> \n\
                                                </table> \n\
                                                </td> \n\
                                                <td valign='top'> \n\
                                                <table border='1px' cellspacing='0px'> \n\
                                                <thead> \n\
                                                <tr> \n\
                                                  <th>地点</th> \n\
                                                  <th>时间</th> \n\
                                                  <th>图片</th> \n\
                                                </tr> \n\
                                                </thead> \n\
                                                <tbody> \n";
                                    }


                                    QString content = QString("<tr name='line'> \n\
                                            <td name='location' attr='%1'>%2</td> \n\
                                            <td name='date' attr='%3'>%4</td> \n\
                                            <td name='record' attr='%5'><input name='bt' type='button' value='查看' onclick='check(%6)' /></td> \n\
                                          </tr>\n").arg(subinfo.fileName()).arg(deviceName).arg(info.fileName()).arg(QDateTime::fromTime_t(cur_time_t).toString("yyyy-MM-dd hh:mm:ss")).arg(record).arg(i);
                                    html += content;

                                    i++;
                                }
                            }

                        }
                    }
                }
            }
    if(i == 0)
    {
        html += "<span style='width:100%;text-align:center;display:block;font-size:25px;margin:10px'>NOT FOUND ANY RECORD</span>\n";
    }

    html += "</tabody> \n\
            </table> \n\
            </td> \n\
         </table> \n\
         <div id='imgdiv' style='padding: 5px 5px 5px 5px; display: none; z-index:10; position:absolute;top:0px; left:0px;background-color:#C0C0C0'> \n\
         <img id='moimg'> \n\
         <span id='close' style='vertical-align:top; font-size:20px'>×<\span> \n\
         </div> \n\
         <script type='text/javascript'> \n\
          function check(i) { \n\
            var t1 = document.getElementsByName('location'); \n\
            var t2 = document.getElementsByName('date'); \n\
            var t3 = document.getElementsByName('record'); \n\
            var t4 = document.getElementsByName('img'); \n\
            var t5 = document.getElementsByName('line'); \n\
            var t6 = document.getElementsByName('bt'); \n\
            for (var j=0;j<t5.length;j++) \n\
            { \n\
            t5[j].setAttribute('bgcolor','#FFFFFF') \n\
            } \n\
            t5[i].setAttribute('bgcolor','#00CCFF') \n\
            var moimg=document.getElementById('moimg') \n\
            var imgdiv=document.getElementById('imgdiv') \n\
            var X= t6[i].getBoundingClientRect().right + 5 + document.documentElement.scrollLeft; \n\
            var Y = t6[i].getBoundingClientRect().top + document.documentElement.scrollTop;  \n\
            imgdiv.style.left=X + 'px' \n\
            imgdiv.style.top=Y + 'px'\n\
            var target = 'Image?date='+t2[i].getAttribute('attr')+'&location='+t1[i].getAttribute('attr')+'&record='+t3[i].getAttribute('attr'); \n\
            moimg.setAttribute('src',target); \n\
            moimg.setAttribute('title',t3[i].getAttribute('attr')); \n\
            imgdiv.style.display='block' \n\
            document.getElementById('close').onclick = function(){imgdiv.style.display='none'} \n\
          } \n\
    </script> \n\
    </body> \n\
    </html>";

    return html;
}

void HttpServer::onRequest(HttpRequest* req, HttpResponse* resp)
{
    QString path = req->path();
    if(path == "/favicon.ico")
        return;

    qDebug() << "---http in---";
    qDebug() << "path:" << path;
    QStringList split = path.split("?");
    QString tag = split[0];
    QMap<QString, QString> varMap;
    QString rootPath = PATH_PREFIX;
    QString xmlFilePath = XML_PATH;

    if(split.length()>1)
        varMap = analysisVars(path.split("?")[1].split("&"));

    if(tag == "/Index")//http://127.0.0.1:58890/Index
    {
        QMap<QString, QString> deviceMap;
        bool flag = analysisDevices(deviceMap, xmlFilePath);

        if(flag)
        {
            QString reply = mkIndexHTML(deviceMap);
            resp->setHeader("Content-Type", "text/html");
            resp->setStatus(200);
            resp->end(reply.toLocal8Bit());
        }
        else
        {
            QString reply  = "<html><head><title>Image Server</title></head><body><h1>xml not found</h1></body></html>";
            resp->setHeader("Content-Type", "text/html");
            resp->setStatus(200);
            resp->end(reply.toLocal8Bit());
        }
    }
    else if(tag == "/Search")
    {
        QMap<QString, QString> deviceMap;
        bool flag = analysisDevices(deviceMap, xmlFilePath);

        if(flag)
        {
            QString deviceName = deviceMap.key(varMap["location"]);
            QString reply = mkSearchHTML(varMap, rootPath, deviceName);
            resp->setHeader("Content-Type", "text/html");
            resp->setStatus(200);
            resp->end(reply.toLocal8Bit());
        }
        else
        {
            QString reply  = "<html><head><title>Image Server</title></head><body><h1>xml not found</h1></body></html>";
            resp->setHeader("Content-Type", "text/html");
            resp->setStatus(200);
            resp->end(reply.toLocal8Bit());
        }
    }
    else if(tag == "/Image")//http://127.0.0.1:58890/Image?date=2018-11-19&location=DS-2PT7D20IW-DE20170902AACH828556878&record=1_1542594638_1ab68ee0-eba3-11e8-857b-6045cb7fb916
    {
        QString imagePath = QString("%1/%2/%3/Cameral1/newrecord_%4.gif").arg(rootPath).arg(varMap["date"]).arg(varMap["location"]).arg(varMap["record"]);

        QFile img(imagePath);

        if(!img.exists())
        {
            imagePath = QString("%1/%2/%3/Cameral1/watched_%4.gif").arg(rootPath).arg(varMap["date"]).arg(varMap["location"]).arg(varMap["record"]);
            qDebug() << imagePath;
            img.setFileName(imagePath);
        }

        if(!img.exists())
        {
            QString reply  = "<html><head><title>Image Server</title></head><body><h1>404 no record</h1></body></html>";
            resp->setHeader("Content-Type", "text/html");
            resp->setStatus(200);
            resp->end(reply.toLocal8Bit());
        }
        else
        {
            img.open(QIODevice::ReadWrite);

            QByteArray ba = img.readAll();

            resp->setHeader("Content-Type", "image/gif");
            resp->setStatus(200);
            resp->end(ba);
        }
    }
    else
    {
        QString reply  = "<html><head><title>Image Server</title></head><body><h1>404 page not found</h1></body></html>";
        resp->setHeader("Content-Type", "text/html");
        resp->setStatus(200);
        resp->end(reply.toLocal8Bit());
    }

    return;
}
