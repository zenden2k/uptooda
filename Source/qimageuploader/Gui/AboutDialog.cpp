#include "AboutDialog.h"

#include "ui_AboutDialog.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/BasicConstants.h"

#include <QPushButton>

AboutDialog::AboutDialog(QWidget *parent): 
    QDialog(parent),
    ui(new Ui::AboutDialog) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->programNameLabel->setText(APP_NAME_A + QStringLiteral(" (Qt GUI)"));

    auto version = AppRuntimeInfo::instance()->GetAppVersion();

    ui->versionLabel->setText(QString::fromStdString(version->FullVersion) + " build " + QString::number(version->Build));
    QPushButton * btn = new QPushButton(tr("About Qt"), this);
    connect(btn, &QPushButton::clicked, qApp, &QApplication::aboutQt);

    ui->buttonBox->addButton(btn, QDialogButtonBox::ActionRole);
}

AboutDialog::~AboutDialog() {
    
}
