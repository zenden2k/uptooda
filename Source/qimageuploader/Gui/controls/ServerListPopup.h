#pragma once

#include <QDialog>

#include <memory>
#include <string>

class QAbstractItemView;
class QButtonGroup;
class QLineEdit;
class QListView;
class QMouseEvent;
class QResizeEvent;
class QTableView;
class QToolButton;
class ServerTableModel;

class ServerListPopup : public QDialog {
    Q_OBJECT

public:
    explicit ServerListPopup(int serversMask, const std::string& selectedServer, QWidget* parent = nullptr);
    ~ServerListPopup() override;

    std::string selectedServer() const;
    void showPopup(const QRect& anchorRect);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void applyFilter();
    void acceptCurrent();
    void showOptionsMenu();
    void showServerContextMenu(const QPoint& position);

private:
    int selectedTypeMask() const;
    QAbstractItemView* activeView() const;
    Qt::Edges resizeEdges(const QPointF& position) const;
    void selectServer(const std::string& serverName);
    void setIconMode(bool enabled);
    void updateResizeCursor(Qt::Edges edges);

    int serversMask_;
    std::string selectedServer_;
    std::unique_ptr<ServerTableModel> model_;
    QLineEdit* searchEdit_;
    QTableView* tableView_;
    QListView* iconView_;
    QButtonGroup* typeButtonGroup_;
    QToolButton* optionsButton_;
};
