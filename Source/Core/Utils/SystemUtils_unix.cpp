#ifndef IU_CORE_UTILS_SYSTEMUTILS_WIN_H
#define IU_CORE_UTILS_SYSTEMUTILS_WIN_H

#include "SystemUtils.h"

#include <string>
#include <limits.h>

namespace IuCoreUtils
{

bool IsOs64Bit() {
    return sizeof(void *) * CHAR_BIT == 64;
}

std::string GetOsName() {
    // TODO:
    return "Unix";
}

std::string GetOsVersion() {
    // TODO: 
    std::string res;
    
    return res;
}

}

#endif