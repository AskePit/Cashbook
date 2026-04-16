#pragma once

#include "bookkeeping/models.h"

#include <QItemDelegate>
#include <QFont>

namespace cashbook
{

class DataModels;

class BriefDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    BriefDelegate(QObject* parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QFont m_headerFont;
    QFont m_commonFont;
    QFont m_normalFont;
};

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
