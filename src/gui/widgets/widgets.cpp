#include "widgets.h"

namespace cashbook {

template <>
void PopupTree<Category, CategoriesModel>::mouseDoubleClickEvent(QMouseEvent *event) {
    chooseValue(event);
}

template <>
void PopupTree<Category, CategoriesModel>::focusOutEvent(QFocusEvent *event) {
    chooseValue(event);
}

template <>
void PopupTree<Wallet, WalletsModel>::mouseDoubleClickEvent(QMouseEvent *event) {
    WalletsModel *model = getSourceModel();
    if(!model) {
        QTreeView::mouseDoubleClickEvent(event);
        return;
    }

    const auto *node = model->getItem(getModelIndex(currentIndex()));
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

    QModelIndex index = getModelIndex(currentIndex());
    if(!index.isValid()) {
        selfDestroy();
        return;
    }

    const auto *node = model->getItem(index);
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
