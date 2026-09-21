//
// Created by user on 30.06.2026.
//

#ifndef UPTOODA_IFAVORITESERVERS_H
#define UPTOODA_IFAVORITESERVERS_H

#include <string>

class IFavoriteServers {
public:
    virtual ~IFavoriteServers() = default;
    virtual bool isServerFavorite(const std::string& server) = 0;
    virtual bool isServerBlacklisted(const std::string& server) = 0;
};

#endif //UPTOODA_IFAVORITESERVERS_H