#include "fileloader.h"
#include <QDebug>

FileLoader::FileLoader(QString path){
    /*
    file = new QFile(path);
    file->open(QIODevice::ReadOnly);
    stream = new QTextStream(file);
    bool flag = false;
    if(path.endsWith("txt", Qt::CaseInsensitive)){
        while(!stream->atEnd()) {
            QString line = stream->readLine();
            if(line.length() > 0 && flag){
                QStringList args = line.split("\t");

            }else{
                if(line.length() == 0){
                    flag = true;
                }
            }
        }
    }
    file->close();
    */
}

FileLoader::~FileLoader(){

}
