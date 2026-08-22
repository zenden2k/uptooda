#ifndef QIMAGEUPLOADER_GUI_VIRTUALFILEDROP_H
#define QIMAGEUPLOADER_GUI_VIRTUALFILEDROP_H

#include <QStringList>

class QMimeData;

namespace VirtualFileDrop {

void installConverter();
bool hasFiles(const QMimeData* mimeData);
QStringList fileNames(const QMimeData* mimeData);
QStringList materializeFiles(const QMimeData* mimeData);

} // namespace VirtualFileDrop

#endif
