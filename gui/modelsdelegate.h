#pragma once

#include "bookkeeping/models.h"

#include <QItemDelegate>

namespace cashbook
{

class DataModels;

class ModelsDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ModelsDelegate(DataModels &dataModels, QObject* parent = nullptr);
    ~ModelsDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    virtual bool eventFilter(QObject *object, QEvent *event) override;

private:
    DataModels &m_data;
};

} // namespace cashbook
