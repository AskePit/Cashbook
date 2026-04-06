/*! @file
 *
 * Qt filesystem helper functions.
 *
 */

#ifndef ASKELIB_STD_FS_H
#define ASKELIB_STD_FS_H

#include <QFile>

namespace aske {

/*! Determines if file `f` contains binary (not readable text) data.
 *
 * It is assumed that `f` is already opened and has 0 position before this
 * function execution.
 * This function will set `f`'s position back at 0 at the end of execution.
 */
bool isBinary(QFile &f);

/*! Create empty file with name `fileName`. */
void createFile(const QString &fileName);

/*! Read file to string. */
QString readFile(const QString &fileName);

/*! Copies file with overwrite. */
bool copyFileForced(const QString &from, const QString &to);

/*! Recursively copies data from `srcDir` folder to `dstDir` folder. */
bool copyRecursively(const QString &srcDir, const QString &dstDir);

/*! Determines if file is an image file. */
bool isPicture(const QString &fileName);

/*! Returns pretty formatted, readable representation of binary data. */
QString binaryToText(const QByteArray &data, bool caps = true);

} // namespace aske

#endif //ASKELIB_STD_FS_H
