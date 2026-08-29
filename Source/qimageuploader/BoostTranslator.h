#ifndef UPTOODA_BOOSTTRANSLATOR_H
#define UPTOODA_BOOSTTRANSLATOR_H

// BoostTranslator.h
#pragma once
#include <QTranslator>
#include <boost/locale.hpp>
#include <locale>

class BoostTranslator : public QTranslator
{
public:
    explicit BoostTranslator(QObject *parent = nullptr);

    bool isEmpty() const override {
        return false;
    }

    QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const override;

private:
    std::locale m_locale;
};

#endif //UPTOODA_BOOSTTRANSLATOR_H