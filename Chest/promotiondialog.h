#ifndef PROMOTIONDIALOG_H
#define PROMOTIONDIALOG_H

#include <QDialog>

namespace Ui {
class PromotionDialog;
}

class PromotionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PromotionDialog(QWidget *parent = nullptr);
    ~PromotionDialog();

private:
    Ui::PromotionDialog *ui;

signals:
    void setPromotion(int k);
};

#endif // PROMOTIONDIALOG_H
