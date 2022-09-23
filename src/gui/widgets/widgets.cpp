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
    WalletsModel *model = dynamic_cast<WalletsModel *>(this->model());
    if(!model) {
        QTreeView::mouseDoubleClickEvent(event);
        return;
    }

    const auto *node = model->getItem(currentIndex());
    if(node->isLeaf()) {
        chooseValue(event);
    }
}

template <>
void PopupTree<Wallet, WalletsModel>::focusOutEvent(QFocusEvent *event) {
    WalletsModel *model = dynamic_cast<WalletsModel *>(this->model());
    if(!model) {
        selfDestroy();
        return;
    }

    QModelIndex index = currentIndex();
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
