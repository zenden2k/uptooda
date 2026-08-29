#ifndef HELPERS_H
#define HELPERS_H

#include <QSize>
#include <QString>

namespace Helpers {

QString GenerateFileNameFromTemplate(const QString& templateString, int index, const QSize& size,
                                     const QString& originalName, const QString& objectType);
QString MakeUniqueFileName(const QString& fileName);

} // namespace Helpers

#endif // HELPERS_H
