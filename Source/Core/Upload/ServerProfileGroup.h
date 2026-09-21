#ifndef IU_CORE_UPLOAD_SERVERPROFILEGROUP_H_
#define IU_CORE_UPLOAD_SERVERPROFILEGROUP_H_

#pragma once 

#include "ServerProfile.h"

class ServerProfileGroup {
public:
    ServerProfileGroup();
    ServerProfileGroup(const ServerProfile& profile);
    void addItem(const ServerProfile& profile);

    /**
     * @throws std::out_of_range
     */
    ServerProfile& getByIndex(size_t index);
    std::vector<ServerProfile>& getItems();
    std::size_t getCount() const;
    bool isEmpty() const;
    /*operator ServerProfile& () {
        return getByIndex(0);
    }*/
private:
    std::vector<ServerProfile> items_;
};
#endif
