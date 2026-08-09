#include "ServerProfileGroup.h"

ServerProfileGroup::ServerProfileGroup() {
}

ServerProfileGroup::ServerProfileGroup(const ServerProfile& profile) {
    addItem(profile);
}

void ServerProfileGroup::addItem(const ServerProfile& profile) {
    items_.push_back(profile);
}

std::vector<ServerProfile>& ServerProfileGroup::getItems() {
    return items_;
}

std::size_t ServerProfileGroup::getCount() const {
    return items_.size();
}

bool ServerProfileGroup::isEmpty() const {
    return items_.empty();
}

ServerProfile& ServerProfileGroup::getByIndex(size_t index) {
    if (items_.empty()) {
        addItem(ServerProfile {});
    }
    return items_.at(index);
}
