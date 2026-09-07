#pragma once

#include <QDialog>
#include "Core/Upload/ServerProfileGroup.h"

class QLineEdit;
class MultiServerSelectorWidget;
class UploadEngineManager;

class ContextMenuItemDialog final : public QDialog {
    Q_OBJECT
public:
    ContextMenuItemDialog(UploadEngineManager* manager, const QString& title, ServerProfileGroup group,
                          QWidget* parent = nullptr);
    QString menuTitle() const;
    bool usesDefaultTitle() const;
    ServerProfileGroup serverProfileGroup() const;

private:
    void accept() override;
    void updateDefaultTitle();
    QLineEdit* title_;
    MultiServerSelectorWidget* servers_;
};
