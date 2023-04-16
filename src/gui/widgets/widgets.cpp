#include "widgets.h"

namespace cashbook {

bool PopupTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceRow);

    //qDebug() << "filterAcceptsRow";

    auto *model = sourceModel();

    QModelIndex currIndex = model->index(sourceRow, 0, sourceParent);

    if (model->data(currIndex).toString().compare(m_filterString, Qt::CaseInsensitive) == 0) {
        return true;
    }

    for(int r = 0; r<model->rowCount(currIndex); ++r) {
        QModelIndex childIndex = model->index(r, 0, currIndex);
        if (model->data(childIndex).toString().compare(m_filterString, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

template <>
void PopupTree<Category, CategoriesModel>::mouseDoubleClickEvent(QMouseEvent *event) {
    chooseValue(event);
}

template <>
void PopupTree<Category, CategoriesModel>::focusOutEvent(QFocusEvent *event) {
    //chooseValue(event);
}

template <>
void PopupTree<Wallet, WalletsModel>::mouseDoubleClickEvent(QMouseEvent *event) {
    WalletsModel *model = getSourceModel();
    if(!model) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const Node<Wallet> *node = model->getItem(getCurrentSourceIndex());
    if(node->isLeaf()) {
        chooseValue(event);
    }
}

template <>
void PopupTree<Wallet, WalletsModel>::focusOutEvent(QFocusEvent *event) {
    WalletsModel *model = getSourceModel();
    if(!model) {
        selfDestroy();
        return;
    }

    QModelIndex index = getCurrentSourceIndex();
    if(!index.isValid()) {
        selfDestroy();
        return;
    }

    const Node<Wallet> *node = model->getItem(index);
    if(!node) {
        selfDestroy();
        return;
    }

    if(node->isLeaf()) {
        chooseValue(event);
        return;
    } else {
        selfDestroy();
        return;
    }
}

} // namespace cashbook
