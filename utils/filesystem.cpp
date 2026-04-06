#include "filesystem.h"
#include <QImageReader>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace aske {

bool isBinary(QFile &f)
{
    if(f.size() == 0) {
        return false;
    }

    char c;
    while(1) {
        f.getChar(&c);

        if(c == 0) {
            f.seek(0);
            return true;
        }

        if(f.atEnd()) {
            break;
        }
    }

    f.seek(0);
    return false;
}

void createFile(const QString &fileName)
{
    QFile f(fileName);
    if(!f.open(QIODevice::WriteOnly)) return;
    f.close();
}

QString readFile(const QString &fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open" << fileName << "file.";
        return QString();
    }

    return QString(file.readAll());
}

bool copyFileForced(const QString &from, const QString &to)
{
    if (QFileInfo(from) == QFileInfo(to)) {
        return true;
    }

    if (QFile::exists(to)) {
        QFile::remove(to);
    }
    return QFile::copy(from, to);
}

bool copyRecursively(const QString &srcDir, const QString &dstDir)
{
    QFileInfo srcFileInfo(srcDir);
    if (srcFileInfo.isDir()) {

        QDir targetDir;
        if (!targetDir.mkpath(dstDir)) {
            return false;
        }
        QDir sourceDir(srcDir);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for(const QString &fileName : std::as_const(fileNames)) {
            const QString postfix = "/" + fileName;
            const QString newSrc = srcDir + postfix;
            const QString newDst = dstDir + postfix;
            if (!copyRecursively(newSrc, newDst)) {
                return false;
            }
        }
    } else {
        if (!copyFileForced(srcDir, dstDir)) {
            return false;
        }
    }
    return true;
}

bool isPicture(const QString &fileName)
{
    QImageReader reader(fileName);
    return reader.format() != QByteArray();
}

static inline bool isNth(int val, int n) {
    return val % n == n - 1;
}

QString binaryToText(const QByteArray &data, bool caps)
{
    Q_UNUSED(caps);
    QByteArray raw = data.toHex();
    QString res;
    for(int i = 0; i<raw.size(); ++i) {
        res += raw[i];
        if(isNth(i, 2)) {
            res += ' ';
        }

        if(isNth(i, 32)) {
            res += '\n';
        } else if(isNth(i, 16)) {
            res += "| ";
        }
    }

    return res;
}

} // namespace aske
