#include "ImageConverterPrivateBase.h"

#include "Core/Utils/CoreUtils.h"
#include "Core/3rdpart/pcreplusplus.h"
#include "Core/Utils/StringUtils.h"
#include "Core/3rdpart/parser.h"

ImageConverterPrivateBase::ImageConverterPrivateBase()
{
    generateThumb_ = false;
    thumbnailTemplate_ = nullptr;
    processingEnabled_ = true;
}

uint32_t ImageConverterPrivateBase::EvaluateColor(const std::string& expr)
{
    uint32_t color = 0;
    color = EvaluateExpression(expr);
    color = ((color << 8) >> 8) | ((255 - (color >> 24)) << 24);
    return color;
}

int ImageConverterPrivateBase::EvaluateExpression(const std::string& expr)
{
    std::string processedExpr = ReplaceVars(expr);
    return static_cast<int>(EvaluateSimpleExpression(processedExpr));
}

int64_t ImageConverterPrivateBase::EvaluateSimpleExpression(const std::string& expr) const
{
    TParser parser;
    int64_t res = 0;
    try
    {
        parser.Compile(expr.c_str());
        res = static_cast<int64_t>(parser.Evaluate());
    }
    catch (const TParserError& err)
    {
        LOG(ERROR) << err.what();
    }
    return res;
}

std::string ImageConverterPrivateBase::ReplaceVars(const std::string& expr)
{
    std::string Result = expr;

    pcrepp::Pcre reg(R"(\$\(([A-z0-9_]*?)\))", "imc");

    int pos = 0;
    while (pos < expr.length())
    {
        if (reg.search(expr, pos))
        {
            pos = reg.get_match_end() + 1;
            std::string vv = reg[1];
            std::string value = m_Vars[vv];

            Result = IuStringUtils::Replace(Result, "$(" + vv + ")", value);
        }
        else
            break;
    }

    // Result  = IuStringUtils::Replace(Result,std::string("#"), "0x");
    {
        pcrepp::Pcre reg2("\\#([0-9A-Fa-f]+)", "imc");
        std::string str = (Result);
        pos = 0;
        while (pos < str.length())
        {
            if (reg2.search(str, pos))
            {
                pos = reg2.get_match_end() + 1;
                std::string vv = reg2[1];
                unsigned int res = strtoul(vv.c_str(), 0, 16);

                Result = IuStringUtils::Replace(Result, "#" + vv, std::to_string(res));
            }
            else
                break;
        }
    }
    return Result;
}
