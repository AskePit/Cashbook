#ifndef INNODEDIALOG_H
#define INNODEDIALOG_H

#include <QDialog>
#include <QModelIndex>

class QAbstractItemModel;

namespace Ui {
class InNodeDialog;
}

namespace cashbook {

class InNodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InNodeDialog(QAbstractItemModel &model, QWidget *parent = 0);
    ~InNodeDialog();

    QModelIndex getIndex();

private:
    Ui::InNodeDialog *ui;
    QAbstractItemModel &m_model;
};

} // namespace cashbook

#endif // INNODEDIALOG_H
