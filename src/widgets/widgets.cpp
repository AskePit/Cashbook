#include "widgets.h"

namespace cashbook {

template<>
const Node<Category> *NodeButton<Category>::treeNode() const {
    if(!m_tree) {
        return nullptr;
    }

    CategoriesModel *model = dynamic_cast<CategoriesModel *>(m_tree->model());
    return model->getItem(m_tree->currentIndex());
}

template<>
const Node<Wallet> *NodeButton<Wallet>::treeNode() const {
    if(!m_tree) {
        return nullptr;
    }

    WalletsModel *model = dynamic_cast<WalletsModel *>(m_tree->model());
    return model->getItem(m_tree->currentIndex());
}

template<>
QModelIndex NodeButton<Category>::nodeIndex() {
    if(m_node) {
        CategoriesModel *model = dynamic_cast<CategoriesModel *>(m_tree->model());
        return model->itemIndex(m_node);
    } else {
        return QModelIndex();
    }
}

template<>
QModelIndex NodeButton<Wallet>::nodeIndex() {
    if(m_node) {
        WalletsModel *model = dynamic_cast<WalletsModel *>(m_tree->model());
        return model->itemIndex(m_node);
    } else {
        return QModelIndex();
    }
}

template <>
void PopupTree<Category>::mouseDoubleClickEvent(QMouseEvent *event) {
    chooseValue(event);
}

template <>
void PopupTree<Category>::focusOutEvent(QFocusEvent *event) {
    chooseValue(event);
}

template <>
void PopupTree<Wallet>::mouseDoubleClickEvent(QMouseEvent *event) {
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
void PopupTree<Wallet>::focusOutEvent(QFocusEvent *event) {
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
