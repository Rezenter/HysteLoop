#include "fileloader.h"
#include <QDebug>

//copypaste, fix

FileLoader::FileLoader(QString path){
    file = new QFile(path);
    file->open(QIODevice::ReadOnly);
    stream = new QTextStream(file);
    bool flag = false;
    if(path.endsWith("txt", Qt::CaseInsensitive)){
        while(!stream->atEnd()) {
            QString line = stream->readLine();
            if(line.length() > 0 && flag){
                QStringList args = line.split("\t");
                if(args.length() < 10){

                }else{


                }
            }else{
                if(line.length() == 0){
                    flag = true;
                }
            }
        }
    }
    file->close();
}

FileLoader::~FileLoader(){
    delete stream;
    file->close();
    delete file;
}
