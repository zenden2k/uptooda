#ifndef QIMAGEUPLOADER_GUI_CONTROLS_MULTISERVERSELECTORWIDGET_H
#define QIMAGEUPLOADER_GUI_CONTROLS_MULTISERVERSELECTORWIDGET_H

#include <QGroupBox>
#include <QString>
#include <vector>

#include "Core/Upload/ServerProfileGroup.h"

class QMouseEvent;
class QPushButton;
class QScrollArea;
class QToolButton;
class QVBoxLayout;
class ServerSelectorWidget;
class UploadEngineManager;

class MultiServerSelectorWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit MultiServerSelectorWidget(UploadEngineManager* uploadEngineManager, QWidget* parent = nullptr);

    void setTitle(const QString& title);
    QString baseTitle() const;
    void setServerProfileGroup(ServerProfileGroup serverProfileGroup);
    ServerProfileGroup serverProfileGroup() const;
    void setServersMask(int mask);
    void updateServerList();
    void fillServerIcons();
    bool validate(QString* firstInvalidServerName = nullptr, bool focusFirstInvalid = true);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    struct SelectorRow {
        QWidget* container = nullptr;
        ServerSelectorWidget* selector = nullptr;
        QToolButton* deleteButton = nullptr;
    };

    void addSelector(const ServerProfile& serverProfile = ServerProfile());
    void removeSelector(QWidget* container);
    void updateDeleteButtons();
    void updateTitle();
    void updateStyle();
    void setValidationError(SelectorRow& row, bool hasError);
    void updateHeight();
    void setExpanded(bool expanded);

    UploadEngineManager* uploadEngineManager_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
    QWidget* selectorContainer_ = nullptr;
    QVBoxLayout* selectorLayout_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QString baseTitle_;
    std::vector<SelectorRow> rows_;
    int serversMask_ = 0xffff;
    bool expanded_ = false;
    bool iconsLoaded_ = false;
};

#endif
