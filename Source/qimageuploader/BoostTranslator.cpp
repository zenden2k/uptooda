#include "BoostTranslator.h"

BoostTranslator::BoostTranslator(QObject* parent): QTranslator(parent) {
}

QString BoostTranslator::translate(const char* context, const char* sourceText, const char* disambiguation, int n) const {
    using boost::locale::translate;
    std::string result;
    try {
        if (n >= 0) {
            result = (disambiguation && *disambiguation)
                ? translate(disambiguation, sourceText, sourceText, n)
                : translate(sourceText, sourceText, n);
        } else if (disambiguation && *disambiguation) {
            result = translate(disambiguation, sourceText);
        } else {
            result = translate(sourceText);
        }
    } catch (const std::exception &) {
        return {};
    }
    return QString::fromUtf8(result.c_str());
}
