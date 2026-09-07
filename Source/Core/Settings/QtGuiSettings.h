#ifndef IU_CORE_SETTINGS_QTGUISETTINGS_H
#define IU_CORE_SETTINGS_QTGUISETTINGS_H

#pragma once
#include "CommonGuiSettings.h"

class QtGuiSettings : public CommonGuiSettings {
public:
    QtGuiSettings();
#ifdef _WIN32
    bool ExplorerContextMenu = false;
    bool ExplorerVideoContextMenu = true;
    bool ExplorerCascadedMenu = true;
    bool SendToContextMenu = false;
    bool QuickUpload = false;
#endif
};

#endif
