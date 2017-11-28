#ifndef FILELOADER_H
#define FILELOADER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QPair>

//copypaste, fix

class FileLoader
{
public:
    FileLoader(QString);
    ~FileLoader();

private:
    QFile *file;
    QTextStream *stream;
};

#endif // FILELOADER_H
