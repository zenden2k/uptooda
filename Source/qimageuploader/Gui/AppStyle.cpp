#include "AppStyle.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QStyledItemDelegate>

namespace {

constexpr qreal MENU_RADIUS = 8.0;
constexpr int MENU_SHADOW_BLUR = 6;
constexpr qreal MENU_SHADOW_LEFT = 4.0;
constexpr qreal MENU_SHADOW_TOP = 3.0;
constexpr qreal MENU_SHADOW_RIGHT = 9.0;
constexpr qreal MENU_SHADOW_BOTTOM = 9.0;
constexpr qreal MENU_SHADOW_OFFSET_X = 2.0;
constexpr qreal MENU_SHADOW_OFFSET_Y = 3.0;
constexpr int MENU_CONTENT_PADDING = 7;
constexpr int COMBO_ITEM_HORIZONTAL_PADDING = 10;
constexpr int COMBO_ITEM_VERTICAL_PADDING = 7;
constexpr int COMBO_ITEM_MINIMUM_HEIGHT = 32;
constexpr int COMBO_MAXIMUM_VISIBLE_ITEMS = 8;

bool isComboBoxPopup(QWidget* widget) { return widget && widget->inherits("QComboBoxPrivateContainer"); }

void configureMenuMargins(QMenu* menu) {
    const QMargins margins(
        qRound(MENU_SHADOW_LEFT) + MENU_CONTENT_PADDING, qRound(MENU_SHADOW_TOP) + MENU_CONTENT_PADDING,
        qRound(MENU_SHADOW_RIGHT) + MENU_CONTENT_PADDING, qRound(MENU_SHADOW_BOTTOM) + MENU_CONTENT_PADDING);
    if (menu->contentsMargins() != margins) {
        menu->setContentsMargins(margins);
    }
}

bool isComboBoxScroller(QWidget* widget) { return widget && widget->inherits("QComboBoxPrivateScroller"); }

QComboBox* comboBoxForPopup(QWidget* popup) {
    for (QObject* parent = popup ? popup->parent() : nullptr; parent; parent = parent->parent()) {
        if (auto* comboBox = qobject_cast<QComboBox*>(parent)) {
            return comboBox;
        }
    }
    return nullptr;
}

void positionComboBoxPopup(QWidget* popup) {
    QComboBox* comboBox = comboBoxForPopup(popup);
    if (!comboBox) {
        return;
    }

    const QPoint comboBottomLeft = comboBox->mapToGlobal(QPoint(0, comboBox->height()));
    popup->move(comboBottomLeft - QPoint(qRound(MENU_SHADOW_LEFT), qRound(MENU_SHADOW_TOP)));
}

class RoundedComboBoxItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize result = QStyledItemDelegate::sizeHint(option, index);
        result.rwidth() += COMBO_ITEM_HORIZONTAL_PADDING * 2;
        result.rheight() += COMBO_ITEM_VERTICAL_PADDING * 2;
        result.setHeight(qMax(result.height(), COMBO_ITEM_MINIMUM_HEIGHT));
        return result;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem contentOption(option);
        initStyleOption(&contentOption, index);
        const bool highlighted = contentOption.state.testFlag(QStyle::State_Selected)
            || contentOption.state.testFlag(QStyle::State_MouseOver);

        if (highlighted) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(QStringLiteral("#e5f3ff")));
            painter->drawRoundedRect(QRectF(contentOption.rect).adjusted(2.0, 1.0, -2.0, -1.0), 4.0, 4.0);
            painter->restore();
        }

        contentOption.state &= ~QStyle::State_Selected;
        contentOption.state &= ~QStyle::State_MouseOver;
        contentOption.state &= ~QStyle::State_HasFocus;
        contentOption.backgroundBrush = Qt::NoBrush;
        contentOption.palette.setColor(QPalette::Highlight, Qt::transparent);
        contentOption.palette.setColor(QPalette::HighlightedText, contentOption.palette.color(QPalette::Text));
        contentOption.rect.adjust(COMBO_ITEM_HORIZONTAL_PADDING, COMBO_ITEM_VERTICAL_PADDING,
                                  -COMBO_ITEM_HORIZONTAL_PADDING, -COMBO_ITEM_VERTICAL_PADDING);

        QStyle* style = contentOption.widget ? contentOption.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &contentOption, painter, contentOption.widget);
    }
};

void configureComboBoxView(QAbstractItemView* itemView) {
    if (!itemView || itemView->property("uptoodaComboBoxViewConfigured").toBool()) {
        return;
    }

    itemView->setProperty("uptoodaComboBoxViewConfigured", true);
    itemView->setObjectName(QStringLiteral("comboBoxPopupView"));
    itemView->setItemDelegate(new RoundedComboBoxItemDelegate(itemView));
    itemView->setFrameStyle(QFrame::NoFrame);
    itemView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    itemView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    itemView->verticalScrollBar()->setObjectName(QStringLiteral("comboBoxPopupScrollBar"));
    itemView->setAutoFillBackground(false);
    itemView->viewport()->setAutoFillBackground(false);
    QPalette viewPalette = itemView->palette();
    viewPalette.setColor(QPalette::Base, Qt::transparent);
    viewPalette.setColor(QPalette::Window, Qt::transparent);
    itemView->setPalette(viewPalette);
    itemView->viewport()->setPalette(viewPalette);
}

void configureComboBoxPopupMargins(QWidget* popup) {
    popup->setContentsMargins(8, 7, 13, 13);
    if (popup->layout()) {
        popup->layout()->setContentsMargins(0, 0, 0, 0);
        popup->layout()->setSpacing(0);
    }
}

void adjustComboBoxPopupGeometry(QWidget* popup) {
    if (popup->property("uptoodaAdjustingComboBoxPopup").toBool()) {
        return;
    }
    popup->setProperty("uptoodaAdjustingComboBoxPopup", true);

    QComboBox* comboBox = comboBoxForPopup(popup);
    auto* itemView = popup->findChild<QAbstractItemView*>();
    if (!comboBox || !itemView) {
        popup->setProperty("uptoodaAdjustingComboBoxPopup", false);
        return;
    }

    configureComboBoxView(itemView);
    comboBox->setMaxVisibleItems(COMBO_MAXIMUM_VISIBLE_ITEMS);
    itemView->doItemsLayout();

    const auto children = popup->findChildren<QWidget*>();
    for (QWidget* child : children) {
        if (isComboBoxScroller(child)) {
            child->setFixedHeight(0);
            child->hide();
        }
    }
    itemView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    const int visibleItemCount = qMin(comboBox->count(), COMBO_MAXIMUM_VISIBLE_ITEMS);
    const bool scrollBarExpected = comboBox->count() > COMBO_MAXIMUM_VISIBLE_ITEMS;
    QMargins popupMargins = popup->contentsMargins();
    popupMargins.setRight(qRound(MENU_SHADOW_RIGHT) + (scrollBarExpected ? 0 : 4));
    popup->setContentsMargins(popupMargins);

    int rowsHeight = 0;
    for (int row = 0; row < visibleItemCount; ++row) {
        rowsHeight += qMax(itemView->sizeHintForRow(row), COMBO_ITEM_MINIMUM_HEIGHT);
    }

    const QMargins margins = popup->contentsMargins();
    const int desiredWidth = comboBox->width() + qRound(MENU_SHADOW_LEFT + MENU_SHADOW_RIGHT);
    const int desiredHeight = rowsHeight + margins.top() + margins.bottom();
    if (popup->size() != QSize(desiredWidth, desiredHeight)) {
        popup->resize(desiredWidth, desiredHeight);
    }
    if (popup->layout()) {
        popup->layout()->activate();
    }

    positionComboBoxPopup(popup);
    popup->setProperty("uptoodaAdjustingComboBoxPopup", false);
}

class RoundedMenuEventFilter final : public QObject {
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (auto* comboBox = qobject_cast<QComboBox*>(watched)) {
            if (event->type() == QEvent::Polish) {
                comboBox->setMaxVisibleItems(COMBO_MAXIMUM_VISIBLE_ITEMS);
                QAbstractItemView* itemView = comboBox->view();
                configureComboBoxView(itemView);

                for (QWidget* parent = itemView->parentWidget(); parent; parent = parent->parentWidget()) {
                    if (isComboBoxPopup(parent)) {
                        configureComboBoxPopupMargins(parent);
                        break;
                    }
                }
            }
            return QObject::eventFilter(watched, event);
        }

        auto* popup = qobject_cast<QWidget*>(watched);
        if (isComboBoxScroller(popup)) {
            if (event->type() == QEvent::Polish) {
                popup->setFixedHeight(0);
                popup->hide();
            } else if (event->type() == QEvent::Show) {
                popup->hide();
                return true;
            }
            return QObject::eventFilter(watched, event);
        }

        auto* menu = qobject_cast<QMenu*>(popup);
        const bool comboBoxPopup = isComboBoxPopup(popup);
        if (!menu && !comboBoxPopup) {
            return QObject::eventFilter(watched, event);
        }

        if (event->type() == QEvent::Polish) {
            popup->setWindowFlags(popup->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
            popup->setAttribute(Qt::WA_TranslucentBackground);
            popup->setAttribute(Qt::WA_NoSystemBackground);
            popup->setAttribute(Qt::WA_StyledBackground, false);
            popup->setAutoFillBackground(false);
            QPalette palette = popup->palette();
            palette.setColor(QPalette::Window, Qt::transparent);
            popup->setPalette(palette);

            if (menu) {
                configureMenuMargins(menu);
            } else if (comboBoxPopup) {
                configureComboBoxPopupMargins(popup);
                if (auto* frame = qobject_cast<QFrame*>(popup)) {
                    frame->setFrameStyle(QFrame::NoFrame);
                }
                if (auto* itemView = popup->findChild<QAbstractItemView*>()) {
                    configureComboBoxView(itemView);
                }
            }
        } else if (event->type() == QEvent::Show || event->type() == QEvent::Resize) {
            if (menu) {
                configureMenuMargins(menu);
            } else if (comboBoxPopup) {
                adjustComboBoxPopupGeometry(popup);
            }
        } else if (event->type() == QEvent::Paint) {
            QPainter painter(popup);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(popup->rect(), Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.setPen(Qt::NoPen);

            const QRectF panelRect
                = QRectF(popup->rect())
                      .adjusted(MENU_SHADOW_LEFT, MENU_SHADOW_TOP, -MENU_SHADOW_RIGHT, -MENU_SHADOW_BOTTOM);
            for (int spread = MENU_SHADOW_BLUR; spread > 0; --spread) {
                const QRectF shadowRect = panelRect.translated(MENU_SHADOW_OFFSET_X, MENU_SHADOW_OFFSET_Y)
                                              .adjusted(-spread, -spread, spread, spread);
                painter.setBrush(QColor(26, 38, 50, 3 + (MENU_SHADOW_BLUR + 1 - spread) * 2));
                painter.drawRoundedRect(shadowRect, MENU_RADIUS + spread, MENU_RADIUS + spread);
            }

            painter.setBrush(QColor(QStringLiteral("#ffffff")));
            painter.setPen(QPen(QColor(QStringLiteral("#cfd8e2")), 1.0));
            painter.drawRoundedRect(panelRect.adjusted(0.5, 0.5, -0.5, -0.5), MENU_RADIUS, MENU_RADIUS);

            if (comboBoxPopup) {
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

} // namespace

void ApplyApplicationStyle(QApplication& application) {
    application.installEventFilter(new RoundedMenuEventFilter(&application));

    QFile styleFile(QStringLiteral(":/styles/Application.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
}
