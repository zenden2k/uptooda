#pragma once

#include <QDialog>

#include <memory>
#include <string>

class QAbstractItemView;
class QButtonGroup;
class QLineEdit;
class QListView;
class QTableView;
class QToolButton;
class ServerTableModel;

class ServerListPopup : public QDialog {
    Q_OBJECT

public:
    explicit ServerListPopup(int serversMask, const std::string& selectedServer, QWidget* parent = nullptr);
    ~ServerListPopup() override;

    std::string selectedServer() const;
    int showPopup(const QRect& anchorRect);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void applyFilter();
    void acceptCurrent();
    void showOptionsMenu();
    void showServerContextMenu(const QPoint& position);

private:
    int selectedTypeMask() const;
    QAbstractItemView* activeView() const;
    void selectServer(const std::string& serverName);
    void setIconMode(bool enabled);

    int serversMask_;
    std::string selectedServer_;
    std::unique_ptr<ServerTableModel> model_;
    QLineEdit* searchEdit_;
    QTableView* tableView_;
    QListView* iconView_;
    QButtonGroup* typeButtonGroup_;
    QToolButton* optionsButton_;
};
