#include "IntegrationSettingsPage.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QSettings>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

#include <shlobj.h>
#include <wrl/client.h>

#include "ContextMenuItemDialog.h"
#include "Core/AbstractServerIconCache.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"

namespace {
const QString REGISTRY_PATH = QStringLiteral("HKEY_CURRENT_USER\\Software\\Uptooda");
const QString CLASS_ID = QStringLiteral("{4B4E72F9-F01C-422E-BF65-98D12861743C}");

QString ShellExtensionPath() {
    SYSTEM_INFO info { };
    GetNativeSystemInfo(&info);
    const bool is64Bit = info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64
        || info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64;
    return QCoreApplication::applicationDirPath()
        + (is64Bit ? QStringLiteral("/ExplorerIntegration64.dll") : QStringLiteral("/ExplorerIntegration.dll"));
}

bool UpdateSendTo(bool enabled) {
    PWSTR folder = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_SendTo, 0, nullptr, &folder))) {
        return false;
    }
    const QString path = QDir(QString::fromWCharArray(folder)).filePath(QStringLiteral("Uptooda.lnk"));
    CoTaskMemFree(folder);
    if (!enabled) {
        return !QFileInfo::exists(path) || QFile::remove(path);
    }
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool success = false;
    {
        Microsoft::WRL::ComPtr<IShellLinkW> link;
        if (SUCCEEDED(
                CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(link.GetAddressOf())))) {
            const std::wstring exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
            const std::wstring directory
                = QDir::toNativeSeparators(QCoreApplication::applicationDirPath()).toStdWString();
            Microsoft::WRL::ComPtr<IPersistFile> file;
            success = SUCCEEDED(link.As(&file)) && SUCCEEDED(link->SetPath(exe.c_str()))
                && SUCCEEDED(link->SetArguments(L"/fromcontextmenu /upload"))
                && SUCCEEDED(link->SetWorkingDirectory(directory.c_str()))
                && SUCCEEDED(link->SetIconLocation(exe.c_str(), 0))
                && SUCCEEDED(file->Save(path.toStdWString().c_str(), TRUE));
        }
    }
    if (SUCCEEDED(initialized)) {
        CoUninitialize();
    }
    return success;
}
}

IntegrationSettingsPage::IntegrationSettingsPage(QtGuiSettings* settings, UploadEngineManager* manager,
                                                 QWidget* parent) :
    SettingsPage(parent), settings_(settings), manager_(manager) {
    auto* layout = new QVBoxLayout(this);
    auto* integration = new QGroupBox(tr("Windows Explorer integration"), this);
    auto* options = new QVBoxLayout(integration);
    enabled_ = new QCheckBox(tr("Shell context menu integration"), integration);
    videos_ = new QCheckBox(tr("Add item to video files' context menu"), integration);
    cascaded_ = new QCheckBox(tr("Cascaded context menu"), integration);
    quick_ = new QCheckBox(tr("Immediately begin uploading to the server"), integration);
    sendTo_ = new QCheckBox(tr("Integration in menu \"Send to\""), integration);
    for (auto* checkbox : { enabled_, videos_, cascaded_, quick_, sendTo_ }) {
        options->addWidget(checkbox);
    }
    layout->addWidget(integration);
    auto* label = new QLabel(tr("Context menu custom items:"), this);
    layout->addWidget(label);
    items_ = new QListWidget(this);
    items_->setProperty("class", "listbox");
    items_->setSpacing(3);
    auto* listLayout = new QHBoxLayout;
    listLayout->setSpacing(8);
    listLayout->addWidget(items_, 1);
    auto* toolbar = new QVBoxLayout;
    toolbar->setSpacing(6);
    auto addButton = [this, toolbar](const QString& icon, const QString& text) {
        auto* button = new QToolButton(this);
        button->setIcon(QIcon(QStringLiteral(":/res/menu-") + icon + QStringLiteral(".svg")));
        button->setIconSize(QSize(20, 20));
        button->setToolTip(text);
        button->setAccessibleName(text);
        button->setFixedSize(36, 34);
        toolbar->addWidget(button);
        return button;
    };
    auto* add = addButton(QStringLiteral("add"), tr("Add Item"));
    edit_ = addButton(QStringLiteral("edit"), tr("Edit Item"));
    remove_ = addButton(QStringLiteral("remove"), tr("Remove Item"));
    up_ = addButton(QStringLiteral("up"), tr("Move Up"));
    down_ = addButton(QStringLiteral("down"), tr("Move Down"));
    toolbar->addStretch();
    listLayout->addLayout(toolbar);
    layout->addLayout(listLayout, 1);
    const bool available = QFileInfo::exists(ShellExtensionPath());
    enabled_->setEnabled(available);
    if (!available) {
        auto* warning = new QLabel(tr("The Explorer shell extension was not found next to the application."), this);
        warning->setWordWrap(true);
        layout->addWidget(warning);
    }
    connect(enabled_, &QCheckBox::toggled, this, [this, label, add](bool checked) {
        videos_->setEnabled(checked);
        cascaded_->setEnabled(checked);
        label->setEnabled(checked);
        items_->setEnabled(checked);
        add->setEnabled(checked);
        updateButtons();
    });
    connect(items_, &QListWidget::currentRowChanged, this, &IntegrationSettingsPage::updateButtons);
    connect(items_, &QListWidget::itemDoubleClicked, this, [this] { editItem(false); });
    connect(add, &QToolButton::clicked, this, [this] { editItem(true); });
    connect(edit_, &QToolButton::clicked, this, [this] { editItem(false); });
    connect(remove_, &QToolButton::clicked, this, [this] {
        changed_ = true;
        delete items_->takeItem(items_->currentRow());
        updateButtons();
    });
    connect(up_, &QToolButton::clicked, this, [this] { moveItem(-1); });
    connect(down_, &QToolButton::clicked, this, [this] { moveItem(1); });
    load();
    for (auto* checkbox : { enabled_, videos_, cascaded_, quick_, sendTo_ }) {
        connect(checkbox, &QCheckBox::toggled, this, [this] { changed_ = true; });
    }
    const bool checked = enabled_->isChecked();
    videos_->setEnabled(checked);
    cascaded_->setEnabled(checked);
    label->setEnabled(checked);
    items_->setEnabled(checked);
    add->setEnabled(checked);
    updateButtons();
}

void IntegrationSettingsPage::load() {
    QSettings options(REGISTRY_PATH, QSettings::NativeFormat);
    enabled_->setChecked(options.value(QStringLiteral("ExplorerContextMenu"), settings_->ExplorerContextMenu).toBool());
    videos_->setChecked(
        options.value(QStringLiteral("ExplorerVideoContextMenu"), settings_->ExplorerVideoContextMenu).toBool());
    cascaded_->setChecked(
        options.value(QStringLiteral("ExplorerCascadedMenu"), settings_->ExplorerCascadedMenu).toBool());
    sendTo_->setChecked(settings_->SendToContextMenu);
    quick_->setChecked(settings_->QuickUpload);
    groups_ = settings_->ServerProfileGroups;
    items_->clear();
    QSettings registry(REGISTRY_PATH + QStringLiteral("\\ContextMenuItems"), QSettings::NativeFormat);
    auto keys = registry.childGroups();
    keys.sort();
    for (const QString& key : keys) {
        registry.beginGroup(key);
        QString id = registry.value(QStringLiteral("ServerProfileGroupId")).toString();
        if (id.isEmpty()) {
            id = key;
            const auto legacy = settings_->ServerProfiles.find(key);
            if (legacy != settings_->ServerProfiles.end()) {
                groups_.emplace(id, ServerProfileGroup(legacy->second));
            }
        }
        auto* item = new QListWidgetItem(registry.value(QStringLiteral("Name")).toString(), items_);
        item->setData(Qt::UserRole, id);
        item->setData(Qt::UserRole + 1, registry.value(QStringLiteral("AutoTitle"), false).toBool());
        if (groups_.find(id) == groups_.end()) {
            item->setToolTip(tr("The server group is missing. Edit or remove this item."));
            item->setForeground(QColor(QStringLiteral("#c44e58")));
        }
        registry.endGroup();
    }
}

bool IntegrationSettingsPage::validate(QString& error) const {
    if (changed_ && enabled_->isChecked() && !QFileInfo::exists(ShellExtensionPath())) {
        error = tr("The Explorer shell extension was not found next to the application.");
        return false;
    }
    return true;
}

void IntegrationSettingsPage::editItem(bool create) {
    auto* item = create ? nullptr : items_->currentItem();
    if (!create && !item) {
        return;
    }
    const QString id = item ? item->data(Qt::UserRole).toString() : QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto found = groups_.find(id);
    ContextMenuItemDialog dialog(manager_, item && !item->data(Qt::UserRole + 1).toBool() ? item->text() : QString(),
                                 found == groups_.end() ? ServerProfileGroup() : found->second, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (!item) {
        item = new QListWidgetItem(items_);
    }
    item->setText(dialog.menuTitle());
    item->setData(Qt::UserRole + 1, dialog.usesDefaultTitle());
    item->setData(Qt::UserRole, id);
    item->setForeground(QBrush());
    item->setToolTip({ });
    groups_[id] = dialog.serverProfileGroup();
    changed_ = true;
    items_->setCurrentItem(item);
    updateButtons();
}

void IntegrationSettingsPage::moveItem(int offset) {
    const int row = items_->currentRow();
    if (row < 0 || row + offset < 0 || row + offset >= items_->count()) {
        return;
    }
    auto* item = items_->takeItem(row);
    changed_ = true;
    items_->insertItem(row + offset, item);
    items_->setCurrentItem(item);
}

void IntegrationSettingsPage::updateButtons() {
    const int row = items_->currentRow();
    const bool selected = enabled_->isChecked() && row >= 0;
    edit_->setEnabled(selected);
    remove_->setEnabled(selected);
    up_->setEnabled(selected && row > 0);
    down_->setEnabled(selected && row + 1 < items_->count());
}

QString IntegrationSettingsPage::applyError() const { return error_; }

void IntegrationSettingsPage::apply() {
    if (!changed_) {
        return;
    }
    settings_->ServerProfileGroups = groups_;
    settings_->ExplorerContextMenu = enabled_->isChecked();
    settings_->ExplorerVideoContextMenu = videos_->isChecked();
    settings_->ExplorerCascadedMenu = cascaded_->isChecked();
    settings_->SendToContextMenu = sendTo_->isChecked();
    settings_->QuickUpload = quick_->isChecked();
}

void IntegrationSettingsPage::afterSave() {
    error_.clear();
    if (!changed_) {
        return;
    }
    if (!UpdateSendTo(sendTo_->isChecked())) {
        error_ = tr("Unable to update the Send to shortcut.");
        return;
    }
    // Register the existing shell extension for the current user, without elevation.
    QSettings classes(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes"), QSettings::NativeFormat);
    if (enabled_->isChecked()) {
        const QString inproc = QStringLiteral("CLSID/") + CLASS_ID + QStringLiteral("/InprocServer32/");
        classes.setValue(inproc + QStringLiteral("."), QDir::toNativeSeparators(ShellExtensionPath()));
        classes.setValue(inproc + QStringLiteral("ThreadingModel"), QStringLiteral("Apartment"));
        for (const QString& type : { QStringLiteral("*"), QStringLiteral("Folder") }) {
            classes.setValue(type + QStringLiteral("/shellex/ContextMenuHandlers/Uptooda/."), CLASS_ID);
        }
    } else {
        for (const QString& type : { QStringLiteral("*"), QStringLiteral("Folder") }) {
            classes.remove(type + QStringLiteral("/shellex/ContextMenuHandlers/Uptooda"));
        }
    }
    classes.sync();
    QSettings registry(REGISTRY_PATH, QSettings::NativeFormat);
    registry.setValue(QStringLiteral("ExplorerContextMenu"), static_cast<int>(enabled_->isChecked()));
    registry.setValue(QStringLiteral("ExplorerVideoContextMenu"), static_cast<int>(videos_->isChecked()));
    registry.setValue(QStringLiteral("ExplorerCascadedMenu"), static_cast<int>(cascaded_->isChecked()));
    registry.setValue(QStringLiteral("ApplicationPath"),
                      QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    registry.setValue(QStringLiteral("DataPath"), QString::fromStdString(AppRuntimeInfo::instance()->dataDirectory()));
    registry.beginGroup(QStringLiteral("ContextMenuItems"));
    registry.remove(QString());
    for (int i = 0; i < items_->count(); ++i) {
        auto* item = items_->item(i);
        const QString id = item->data(Qt::UserRole).toString();
        registry.beginGroup(QStringLiteral("%1_%2").arg(i, 6, 10, QLatin1Char('0')).arg(id));
        registry.setValue(QStringLiteral("Name"), item->text());
        registry.setValue(QStringLiteral("AutoTitle"), static_cast<int>(item->data(Qt::UserRole + 1).toBool()));
        registry.setValue(QStringLiteral("ServerProfileGroupId"), id);
        const auto found = groups_.find(id);
        if (found != groups_.end() && !found->second.isEmpty()) {
            auto group = found->second;
            auto* cache = ServiceLocator::instance()->serverIconCache();
            if (cache) {
                registry.setValue(
                    QStringLiteral("Icon"),
                    QString::fromStdString(cache->getIconNameForServer(group.getByIndex(0).serverName(), false)));
            }
        }
        registry.endGroup();
    }
    registry.endGroup();
    registry.sync();
    if (classes.status() != QSettings::NoError || registry.status() != QSettings::NoError) {
        error_ = tr("Unable to save Windows Explorer integration settings to the registry.");
        return;
    }
    changed_ = false;
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}
