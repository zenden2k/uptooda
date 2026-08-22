#include "VirtualFileDrop.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaType>
#include <QMimeData>
#include <QVariant>

#include <limits>

#include "Core/AppRuntimeInfo.h"

#include <QWindowsMimeConverter>

#include <ShlObj_core.h>
#include <objidl.h>

namespace {

const QString VIRTUAL_FILE_NAMES_MIME_TYPE = QStringLiteral("application/x-uptooda-windows-virtual-file-names");
const QString VIRTUAL_FILE_CONTENTS_MIME_TYPE = QStringLiteral("application/x-uptooda-windows-virtual-file-contents");

template <typename DescriptorGroup>
bool IsValidDescriptorGroup(HGLOBAL handle, const DescriptorGroup* group) {
    if (!handle || !group) {
        return false;
    }
    const SIZE_T headerSize = offsetof(DescriptorGroup, fgd);
    const SIZE_T availableSize = GlobalSize(handle);
    return availableSize >= headerSize && group->cItems <= (availableSize - headerSize) / sizeof(group->fgd[0]);
}

QString SafeFileName(const QString& fileName) {
    const QString result = QFileInfo(QDir::fromNativeSeparators(fileName)).fileName();
    return result.isEmpty() || result == QStringLiteral(".") || result == QStringLiteral("..")
        ? QStringLiteral("phone-file")
        : result;
}

QString UniqueDestination(const QString& fileName) {
    const QDir directory(QString::fromStdString(AppRuntimeInfo::instance()->tempDirectory()));
    const QFileInfo fileInfo(SafeFileName(fileName));
    QString destination = directory.filePath(fileInfo.fileName());
    for (int suffixNumber = 2; QFileInfo::exists(destination); ++suffixNumber) {
        const QString suffix = fileInfo.suffix();
        const QString numberedName = fileInfo.completeBaseName() + QStringLiteral("_%1").arg(suffixNumber)
            + (suffix.isEmpty() ? QString { } : QStringLiteral(".") + suffix);
        destination = directory.filePath(numberedName);
    }
    return destination;
}

bool WriteAll(QFile& destination, const char* data, qint64 size) {
    while (size > 0) {
        const qint64 written = destination.write(data, size);
        if (written <= 0) {
            return false;
        }
        data += written;
        size -= written;
    }
    return true;
}

bool SaveHGlobal(HGLOBAL handle, quint64 descriptorSize, bool hasDescriptorSize, QFile& destination) {
    const SIZE_T availableSize = GlobalSize(handle);
    const quint64 size = hasDescriptorSize ? descriptorSize : availableSize;
    if (size > availableSize || size > static_cast<quint64>(std::numeric_limits<qint64>::max())) {
        return false;
    }
    const auto* data = static_cast<const char*>(GlobalLock(handle));
    if (!data && size != 0) {
        return false;
    }
    const bool result = WriteAll(destination, data, static_cast<qint64>(size));
    if (data) {
        GlobalUnlock(handle);
    }
    return result;
}

bool SaveStream(IStream* stream, QFile& destination) {
    if (!stream) {
        return false;
    }
    LARGE_INTEGER start { };
    stream->Seek(start, STREAM_SEEK_SET, nullptr);
    char buffer[64 * 1024];
    while (true) {
        ULONG bytesRead = 0;
        const HRESULT result = stream->Read(buffer, sizeof(buffer), &bytesRead);
        if (FAILED(result)) {
            return false;
        }
        if (bytesRead && !WriteAll(destination, buffer, bytesRead)) {
            return false;
        }
        if (bytesRead == 0 || result == S_FALSE) {
            return true;
        }
    }
}

template <typename FileDescriptor>
bool SaveFileContents(IDataObject* dataObject, LONG index, const FileDescriptor& descriptor,
                      const QString& descriptorFileName, QString* savedFileName) {
    FORMATETC contentsFormat { static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_FILECONTENTS)), nullptr,
                               DVASPECT_CONTENT, index, TYMED_HGLOBAL | TYMED_ISTREAM };
    STGMEDIUM contents { };
    if (FAILED(dataObject->GetData(&contentsFormat, &contents))) {
        return false;
    }

    const QString destinationName = UniqueDestination(descriptorFileName);
    QFile destination(destinationName);
    bool saved = destination.open(QIODevice::WriteOnly);
    if (saved && contents.tymed == TYMED_HGLOBAL) {
        const quint64 descriptorSize = (static_cast<quint64>(descriptor.nFileSizeHigh) << 32) | descriptor.nFileSizeLow;
        saved = SaveHGlobal(contents.hGlobal, descriptorSize, descriptor.dwFlags & FD_FILESIZE, destination);
    } else if (saved && contents.tymed == TYMED_ISTREAM) {
        saved = SaveStream(contents.pstm, destination);
    } else {
        saved = false;
    }
    destination.close();
    ReleaseStgMedium(&contents);

    if (!saved) {
        destination.remove();
        return false;
    }
    *savedFileName = destinationName;
    return true;
}

template <typename DescriptorGroup, typename FileNameConverter>
QStringList MaterializeDescriptorGroup(IDataObject* dataObject, CLIPFORMAT format, FileNameConverter convertName) {
    FORMATETC descriptorFormat { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM descriptors { };
    if (FAILED(dataObject->GetData(&descriptorFormat, &descriptors))) {
        return { };
    }

    QStringList result;
    const auto* group = static_cast<const DescriptorGroup*>(GlobalLock(descriptors.hGlobal));
    if (IsValidDescriptorGroup(descriptors.hGlobal, group)) {
        for (UINT index = 0; index < group->cItems; ++index) {
            QString savedFileName;
            const auto& descriptor = group->fgd[index];
            if (SaveFileContents(dataObject, static_cast<LONG>(index), descriptor, convertName(descriptor.cFileName),
                                 &savedFileName)) {
                result.append(savedFileName);
            }
        }
    }
    if (group) {
        GlobalUnlock(descriptors.hGlobal);
    }
    ReleaseStgMedium(&descriptors);
    return result;
}

template <typename DescriptorGroup, typename FileNameConverter>
QStringList ReadDescriptorNames(IDataObject* dataObject, CLIPFORMAT format, FileNameConverter convertName) {
    FORMATETC descriptorFormat { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM descriptors { };
    if (FAILED(dataObject->GetData(&descriptorFormat, &descriptors))) {
        return { };
    }

    QStringList result;
    const auto* group = static_cast<const DescriptorGroup*>(GlobalLock(descriptors.hGlobal));
    if (IsValidDescriptorGroup(descriptors.hGlobal, group)) {
        result.reserve(group->cItems);
        for (UINT index = 0; index < group->cItems; ++index) {
            result.append(SafeFileName(convertName(group->fgd[index].cFileName)));
        }
    }
    if (group) {
        GlobalUnlock(descriptors.hGlobal);
    }
    ReleaseStgMedium(&descriptors);
    return result;
}

QByteArray EncodeFileNames(const QStringList& fileNames) {
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << fileNames;
    return encoded;
}

class VirtualFileMimeConverter final : public QWindowsMimeConverter {
public:
    bool canConvertFromMime(const FORMATETC&, const QMimeData*) const override { return false; }
    bool convertFromMime(const FORMATETC&, const QMimeData*, STGMEDIUM*) const override { return false; }
    QList<FORMATETC> formatsForMime(const QString&, const QMimeData*) const override { return { }; }

    bool canConvertToMime(const QString& mimeType, IDataObject* dataObject) const override {
        if ((mimeType != VIRTUAL_FILE_NAMES_MIME_TYPE && mimeType != VIRTUAL_FILE_CONTENTS_MIME_TYPE) || !dataObject) {
            return false;
        }
        FORMATETC format { descriptorFormat(dataObject), nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        return format.cfFormat && SUCCEEDED(dataObject->QueryGetData(&format));
    }

    QVariant convertToMime(const QString& mimeType, IDataObject* dataObject, QMetaType) const override {
        if ((mimeType != VIRTUAL_FILE_NAMES_MIME_TYPE && mimeType != VIRTUAL_FILE_CONTENTS_MIME_TYPE) || !dataObject) {
            return { };
        }
        return EncodeFileNames(mimeType == VIRTUAL_FILE_NAMES_MIME_TYPE ? readFileNames(dataObject)
                                                                        : materializeFiles(dataObject));
    }

    QString mimeForFormat(const FORMATETC& format) const override {
        const CLIPFORMAT unicodeFormat = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
        const CLIPFORMAT ansiFormat = RegisterClipboardFormatA("FileGroupDescriptor");
        return format.tymed & TYMED_HGLOBAL && (format.cfFormat == unicodeFormat || format.cfFormat == ansiFormat)
            ? VIRTUAL_FILE_NAMES_MIME_TYPE
            : QString { };
    }

private:
    static QStringList readFileNames(IDataObject* dataObject) {
        const CLIPFORMAT unicodeFormat = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
        FORMATETC format { unicodeFormat, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (SUCCEEDED(dataObject->QueryGetData(&format))) {
            return ReadDescriptorNames<FILEGROUPDESCRIPTORW>(dataObject, unicodeFormat, [](const wchar_t* name) {
                return QString::fromWCharArray(name, wcsnlen(name, MAX_PATH));
            });
        }
        return ReadDescriptorNames<FILEGROUPDESCRIPTORA>(
            dataObject, RegisterClipboardFormatA("FileGroupDescriptor"),
            [](const char* name) { return QString::fromLocal8Bit(name, strnlen(name, MAX_PATH)); });
    }

    static QStringList materializeFiles(IDataObject* dataObject) {
        const CLIPFORMAT unicodeFormat = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
        FORMATETC format { unicodeFormat, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (SUCCEEDED(dataObject->QueryGetData(&format))) {
            return MaterializeDescriptorGroup<FILEGROUPDESCRIPTORW>(dataObject, unicodeFormat, [](const wchar_t* name) {
                return QString::fromWCharArray(name, wcsnlen(name, MAX_PATH));
            });
        }
        return MaterializeDescriptorGroup<FILEGROUPDESCRIPTORA>(
            dataObject, RegisterClipboardFormatA("FileGroupDescriptor"),
            [](const char* name) { return QString::fromLocal8Bit(name, strnlen(name, MAX_PATH)); });
    }

    static CLIPFORMAT descriptorFormat(IDataObject* dataObject) {
        const CLIPFORMAT unicodeFormat = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
        FORMATETC unicodeDescriptor { unicodeFormat, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (SUCCEEDED(dataObject->QueryGetData(&unicodeDescriptor))) {
            return unicodeFormat;
        }
        return RegisterClipboardFormatA("FileGroupDescriptor");
    }
};

} // namespace
namespace VirtualFileDrop {

void installConverter() { new VirtualFileMimeConverter; }

bool hasFiles(const QMimeData* mimeData) { return mimeData && mimeData->hasFormat(VIRTUAL_FILE_NAMES_MIME_TYPE); }

QStringList DecodeFileNames(const QMimeData* mimeData, const QString& mimeType) {
    if (!mimeData) {
        return { };
    }
    QByteArray encoded = mimeData->data(mimeType);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    QStringList result;
    stream >> result;
    return stream.status() == QDataStream::Ok ? result : QStringList { };
}

QStringList fileNames(const QMimeData* mimeData) { return DecodeFileNames(mimeData, VIRTUAL_FILE_NAMES_MIME_TYPE); }

QStringList materializeFiles(const QMimeData* mimeData) {
    return hasFiles(mimeData) ? DecodeFileNames(mimeData, VIRTUAL_FILE_CONTENTS_MIME_TYPE) : QStringList { };
}

} // namespace VirtualFileDrop
