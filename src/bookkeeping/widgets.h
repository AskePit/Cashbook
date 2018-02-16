#ifndef BOOKKEEPING_WIDGETS_H
#define BOOKKEEPING_WIDGETS_H

#include "models.h"
#include <QTreeView>
#include <QMouseEvent>
#include <QPushButton>
#include <QHeaderView>

namespace cashbook
{

/*
 * There are different focus behaviour for NodeButton depending on if PopupTree
 * is shown or not.
 */
enum class NodeButtonState {
    Folded,  // just button
    Expanded // button is under PopupTree
};

template <class T>
class NodeButton : public QPushButton
{
public:
    NodeButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setStyleSheet("padding:0 0 0 2; text-align:left");
    }

    void setState(NodeButtonState s) {
        m_state = s;
    }

    NodeButtonState state() {
        return m_state;
    }

    void setNode(const Node<T> *node) {
        m_node = node;
        setText(pathToShortString(node));
    }

    void fetchNode() {
        const auto *node = treeNode();
        setNode(node);
        setFocus();
        m_tree = nullptr;
    }

    void setTree(QTreeView *tree) {
        m_tree = tree;
        m_tree->setCurrentIndex(nodeIndex());
    }

    const Node<T> *node() const {
        return m_node;
    }

private:
    NodeButtonState m_state {NodeButtonState::Folded};
    const Node<T> *m_node {nullptr};
    QTreeView *m_tree {nullptr};

    const Node<T> *treeNode() const;
    QModelIndex nodeIndex();
};

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

template <class T>
class PopupTree : public QTreeView
{
public:
    PopupTree(QAbstractItemModel *model, NodeButton<T> *button, QWidget *parent = nullptr)
        : QTreeView(parent)
        , m_button(button)
    {
        setModel(model);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setStyleSheet("QTreeView::item { height: 18px;}");
        expandToDepth(0);
        header()->hide();
        for(int i = CategoriesColumn::Name+1; i<CategoriesColumn::Count; ++i) {
            hideColumn(i);
        }
        button->setTree(this);
        move(button->pos());
        show();
        setFocus();
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void focusOutEvent(QFocusEvent *event);

private:
    NodeButton<T> *m_button;

    void chooseValue(QEvent *event){
        Q_UNUSED(event);

        m_button->fetchNode();
        selfDestroy();
    }

    void selfDestroy() {
        close();
        m_button->setState(NodeButtonState::Folded);
        deleteLater();
    }
};

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

#endif // BOOKKEEPING_WIDGETS_H
