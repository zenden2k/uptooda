#include "Helpers.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QRegularExpression>

namespace Helpers {

namespace {

QString GenerateRandomString(int length) {
    static constexpr QLatin1StringView RANDOM_CHARACTERS(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    QString result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += RANDOM_CHARACTERS.at(QRandomGenerator::global()->bounded(RANDOM_CHARACTERS.size()));
    }
    return result;
}

void ReplaceRandomMacros(QString& value) {
    static const QRegularExpression RANDOM_MACRO(QStringLiteral(R"(%random\((\d+)\)%?)"));

    qsizetype offset = 0;
    while (true) {
        const QRegularExpressionMatch match = RANDOM_MACRO.match(value, offset);
        if (!match.hasMatch()) {
            return;
        }

        bool validLength = false;
        const int length = match.capturedView(1).toInt(&validLength);
        if (!validLength) {
            offset = match.capturedEnd();
            continue;
        }

        const QString randomString = GenerateRandomString(length);
        value.replace(match.capturedStart(), match.capturedLength(), randomString);
        offset = match.capturedStart() + randomString.size();
    }
}

} // namespace

QString GenerateFileNameFromTemplate(const QString& templateString, int index, const QSize& size,
                                     const QString& originalName, const QString& objectType) {
    QString result = templateString;
    const QDateTime now = QDateTime::currentDateTime();
    const QFileInfo originalFileInfo(originalName);
    const QByteArray uniqueValue = QByteArray::number(now.toMSecsSinceEpoch()) + ':'
        + QByteArray::number(QRandomGenerator::global()->generate64());
    const QString md5 = QString::fromLatin1(QCryptographicHash::hash(uniqueValue, QCryptographicHash::Md5).toHex());

    result.replace(QStringLiteral("%md5%"), md5);
    result.replace(QStringLiteral("%uid%"), md5.mid(5, 6));
    ReplaceRandomMacros(result);
    result.replace(QStringLiteral("%cx%"), QString::number(size.width()));
    result.replace(QStringLiteral("%cy%"), QString::number(size.height()));
    result.replace(QStringLiteral("%width%"), QString::number(size.width()));
    result.replace(QStringLiteral("%height%"), QString::number(size.height()));
    result.replace(QStringLiteral("%y%"), now.toString(QStringLiteral("yyyy")));
    result.replace(QStringLiteral("%m%"), now.toString(QStringLiteral("MM")));
    result.replace(QStringLiteral("%d%"), now.toString(QStringLiteral("dd")));
    result.replace(QStringLiteral("%h%"), now.toString(QStringLiteral("hh")));
    result.replace(QStringLiteral("%n%"), now.toString(QStringLiteral("mm")));
    result.replace(QStringLiteral("%s%"), now.toString(QStringLiteral("ss")));
    result.replace(QStringLiteral("%i%"), QStringLiteral("%1").arg(index, 3, 10, QLatin1Char('0')));
    result.replace(QStringLiteral("%fe%"), originalFileInfo.fileName());
    result.replace(QStringLiteral("%f%"), originalFileInfo.completeBaseName());
    result.replace(QStringLiteral("%ext%"), originalFileInfo.suffix());
    result.replace(QStringLiteral("%type%"), objectType);

    // Older filename templates use macros without a closing percent sign.
    result.replace(QStringLiteral("%md5"), md5);
    result.replace(QStringLiteral("%uid"), md5.mid(5, 6));
    result.replace(QStringLiteral("%width"), QString::number(size.width()));
    result.replace(QStringLiteral("%height"), QString::number(size.height()));
    result.replace(QStringLiteral("%cx"), QString::number(size.width()));
    result.replace(QStringLiteral("%cy"), QString::number(size.height()));
    result.replace(QStringLiteral("%fe"), originalFileInfo.fileName());
    result.replace(QStringLiteral("%ext"), originalFileInfo.suffix());
    result.replace(QStringLiteral("%type"), objectType);
    result.replace(QStringLiteral("%f"), originalFileInfo.completeBaseName());
    result.replace(QStringLiteral("%y"), now.toString(QStringLiteral("yyyy")));
    result.replace(QStringLiteral("%m"), now.toString(QStringLiteral("MM")));
    result.replace(QStringLiteral("%d"), now.toString(QStringLiteral("dd")));
    result.replace(QStringLiteral("%h"), now.toString(QStringLiteral("hh")));
    result.replace(QStringLiteral("%n"), now.toString(QStringLiteral("mm")));
    result.replace(QStringLiteral("%s"), now.toString(QStringLiteral("ss")));
    result.replace(QStringLiteral("%i"), QStringLiteral("%1").arg(index, 3, 10, QLatin1Char('0')));
    return result;
}

QString MakeUniqueFileName(const QString& fileName) {
    if (!QFileInfo::exists(fileName)) {
        return fileName;
    }

    const QFileInfo fileInfo(fileName);
    const QString suffix = fileInfo.suffix();
    const QString extension = suffix.isEmpty() ? QString() : QLatin1Char('.') + suffix;
    const QString baseName = fileInfo.completeBaseName();
    const QDir directory = fileInfo.absoluteDir();
    for (int number = 2;; ++number) {
        const QString candidate
            = directory.filePath(QStringLiteral("%1 (%2)%3").arg(baseName).arg(number).arg(extension));
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
}

} // namespace Helpers
