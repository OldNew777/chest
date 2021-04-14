#include "promotiondialog.h"
#include "ui_promotiondialog.h"

PromotionDialog::PromotionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PromotionDialog)
{
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, [=]{
        emit this->setPromotion(ui->comboBox->currentIndex() + 2);
    });
}

PromotionDialog::~PromotionDialog()
{
    delete ui;
}
