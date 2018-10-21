#ifndef BOOKKEEPING_WIDGETS_H
#define BOOKKEEPING_WIDGETS_H

#include "models.h"
#include <QTreeView>
#include <QMouseEvent>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>

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
class PopupTree;

template <class T>
class NodeButton : public QPushButton
{
public:
    NodeButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setStyleSheet("padding:0 0 0 2; text-align:left");
    }

    NodeButton(QAbstractItemModel &model, QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setStyleSheet("padding:0 0 0 2; text-align:left");
        setModel(model);
    }

    ~NodeButton() {
        /*if(m_tree) {
            m_tree->deleteLater();
        }*/
    }

    void setModel(QAbstractItemModel &model) {
        connect(this, &QPushButton::clicked, [this, &model]() {
            this->setState(NodeButtonState::Expanded);
            QTreeView *view = new PopupTree<T>(model, this, this);
            UNUSED(view);
        });
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
const Node<Category> *NodeButton<Category>::treeNode() const;

template<>
const Node<Wallet> *NodeButton<Wallet>::treeNode() const;

template<>
QModelIndex NodeButton<Category>::nodeIndex();

template<>
QModelIndex NodeButton<Wallet>::nodeIndex();

template <class T>
class PopupTree : public QTreeView
{
public:
    PopupTree(QAbstractItemModel &model, NodeButton<T> *button, QWidget *parent = nullptr)
        : QTreeView(parent)
        , m_button(button)
    {
        setModel(&model);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::Tool);

        setStyleSheet("QTreeView::item { height: 18px;}");
        setAlternatingRowColors(true);
        expandToDepth(0);
        header()->hide();
        for(int i = CategoriesColumn::Name+1; i<CategoriesColumn::Count; ++i) {
            hideColumn(i);
        }
        button->setTree(this);
        move(button->mapToGlobal(QPoint(0, 0)));
        show();
        activateWindow();
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void focusOutEvent(QFocusEvent *event);

    // do not forward scroll to parents in case of nonscrolling state
    void wheelEvent(QWheelEvent* event) {
        bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();
        bool atTop = verticalScrollBar()->value() == verticalScrollBar()->minimum();

        bool scrollDown = event->delta() < 0;
        bool scrollUp = !scrollDown;

        bool noWay = (atBottom && scrollDown) || (atTop && scrollUp);

        if(noWay) {
            event->accept();
        } else {
            QTreeView::wheelEvent(event);
        }
    }

private:
    NodeButton<T> *m_button;
    bool m_gonnaDestroy {false};

    void chooseValue(QEvent *event){
        Q_UNUSED(event);

        if(m_gonnaDestroy) {
            return;
        }

        m_button->fetchNode();
        m_gonnaDestroy = true;
        selfDestroy();
    }

    void selfDestroy() {
        close();
        m_button->setState(NodeButtonState::Folded);
        deleteLater();
    }
};

template <>
void PopupTree<Category>::mouseDoubleClickEvent(QMouseEvent *event);

template <>
void PopupTree<Category>::focusOutEvent(QFocusEvent *event);

template <>
void PopupTree<Wallet>::mouseDoubleClickEvent(QMouseEvent *event);

template <>
void PopupTree<Wallet>::focusOutEvent(QFocusEvent *event);

} // namespace cashbook

#endif // BOOKKEEPING_WIDGETS_H
