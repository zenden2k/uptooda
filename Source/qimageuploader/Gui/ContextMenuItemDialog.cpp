#include "ContextMenuItemDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

#include "controls/MultiServerSelectorWidget.h"

ContextMenuItemDialog::ContextMenuItemDialog(UploadEngineManager* manager, const QString& title,
                                             ServerProfileGroup group, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Context menu item"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(650, 300);
    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel(tr("Menu item title:"), this);
    title_ = new QLineEdit(title, this);
    label->setBuddy(title_);
    layout->addWidget(label);
    layout->addWidget(title_);
    auto* hint = new QLabel(tr("Leave empty to use the server names."), this);
    layout->addWidget(hint);
    servers_ = new MultiServerSelectorWidget(manager, this);
    servers_->setTitle(tr("Servers"));
    servers_->setServersMask(3);
    servers_->setServerProfileGroup(std::move(group));
    servers_->fillServerIcons();
    layout->addWidget(servers_);
    layout->addStretch();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &ContextMenuItemDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ContextMenuItemDialog::reject);
    connect(servers_, &MultiServerSelectorWidget::serverProfileGroupChanged, this,
            &ContextMenuItemDialog::updateDefaultTitle);
    updateDefaultTitle();
}

void ContextMenuItemDialog::updateDefaultTitle() {
    auto group = servers_->serverProfileGroup();
    QString title;
    if (!group.isEmpty()) {
        const QString server = QString::fromStdString(group.getByIndex(0).serverName());
        title = group.getCount() == 1
            ? tr("Upload to %1").arg(server)
            : tr("Upload to %1 and %n more servers", nullptr, static_cast<int>(group.getCount() - 1)).arg(server);
    }
    title_->setPlaceholderText(title);
}

QString ContextMenuItemDialog::menuTitle() const {
    return title_->text().trimmed().isEmpty() ? title_->placeholderText() : title_->text().trimmed();
}

bool ContextMenuItemDialog::usesDefaultTitle() const { return title_->text().trimmed().isEmpty(); }

ServerProfileGroup ContextMenuItemDialog::serverProfileGroup() const { return servers_->serverProfileGroup(); }

void ContextMenuItemDialog::accept() {
    auto group = serverProfileGroup();
    if (!servers_->validate() || group.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Select a server and configure its account."));
        return;
    }
    for (auto& profile : group.getItems()) {
        if (!profile.uploadEngineData()) {
            QMessageBox::warning(this, windowTitle(), tr("The selected server is unavailable."));
            return;
        }
    }
    QDialog::accept();
}
