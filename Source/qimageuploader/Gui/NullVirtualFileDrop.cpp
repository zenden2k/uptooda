#include "VirtualFileDrop.h"

#include <QMimeData>

namespace VirtualFileDrop {

void installConverter() { }

bool hasFiles(const QMimeData*) { return false; }

QStringList fileNames(const QMimeData*) { return { }; }

QStringList materializeFiles(const QMimeData*) { return { }; }

} // namespace VirtualFileDrop
