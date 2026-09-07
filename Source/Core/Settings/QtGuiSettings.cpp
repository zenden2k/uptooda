#include "QtGuiSettings.h"

QtGuiSettings::QtGuiSettings() : CommonGuiSettings() {
    BindToManager();
#ifdef _WIN32
    auto& general = mgr_["General"];
    general.n_bind(ExplorerContextMenu);
    general.n_bind(ExplorerVideoContextMenu);
    general.n_bind(ExplorerCascadedMenu);
    general.n_bind(SendToContextMenu);
    general.n_bind(QuickUpload);
#endif
}
