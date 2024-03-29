#ifndef BOOKKEEPING_WIDGETS_H
#define BOOKKEEPING_WIDGETS_H

#include "bookkeeping/models.h"
#include <QTreeView>
#include <QMouseEvent>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>
#include <QDoubleSpinBox>

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

template <class T, class Model>
class PopupTree;

template <class T, class Model>
class NodeButton : public QPushButton
{
public:
    NodeButton(QWidget *parent = nullptr)
        : QPushButton(parent)
        , m_nodeSetCallback(nullptr)
    {
        setStyleSheet("padding:0 0 0 2; text-align:left");
    }

    NodeButton(QAbstractItemModel &model, QWidget *parent = nullptr)
        : QPushButton(parent)
        , m_nodeSetCallback(nullptr)
    {
        setStyleSheet("padding:0 0 0 2; text-align:left");
        setModel(model);
    }

    ~NodeButton() {
        /*if(m_tree) {
            m_tree->deleteLater();
        }*/
    }

    void setModel(QAbstractItemModel &model, bool setParent = false) {
        disconnect(m_modelConnection);

        this->setTree(nullptr);
        this->setNode(nullptr);

        m_modelConnection = connect(this, &QPushButton::clicked, [this, &model, setParent]() {


            this->setState(NodeButtonState::Expanded);
            QTreeView *view = new PopupTree<T, Model>(model, this, setParent ? this : nullptr);
            Q_UNUSED(view);
        });
    }

    void setState(NodeButtonState s) {
        m_state = s;
    }

    NodeButtonState state() {
        return m_state;
    }

    void setTree(QTreeView *tree) {
        m_tree = tree;
    }

    void setNode(const Node<T> *node) {
        m_node = node;
        setText(pathToShortString(node));
        if (m_nodeSetCallback != nullptr) {
            m_nodeSetCallback(m_node);
        }
    }

    const Node<T> *node() const {
        return m_node;
    }

    void setUpdateCallback(std::function<void(const Node<T>*)> cb) {
        m_nodeSetCallback = cb;
    }

private:
    NodeButtonState m_state {NodeButtonState::Folded};
    const Node<T> *m_node {nullptr};
    QTreeView *m_tree {nullptr};
    QMetaObject::Connection m_modelConnection;

    std::function<void(const Node<T>*)> m_nodeSetCallback {nullptr};
};

class PopupTreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PopupTreeProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}
};

template <class T, class Model>
class PopupTree : public QTreeView
{
public:
    PopupTree(QAbstractItemModel &model, NodeButton<T, Model> *button, QWidget *parent = nullptr)
        : QTreeView(parent)
        , m_button(button)
    {
        PopupTreeProxyModel* proxy = new PopupTreeProxyModel(parent);
        proxy->setSourceModel(&model);

        setModel(proxy);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::Tool);

        setStyleSheet("QTreeView::item { height: 18px;}");
        setAlternatingRowColors(true);
        header()->hide();
        for(int i = WalletColumn::Name+1; i<WalletColumn::Count; ++i) {
            hideColumn(i);
        }
        button->setTree(this);

        const Node<T> *node = button->node();
        setCurrentIndex(node ? getProxyIndex(qobject_cast<Model*>(&model)->itemIndex(node)) : QModelIndex());
        move(button->mapToGlobal(QPoint(0, 0)));
        show();
        activateWindow();
        setFocus();
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void focusOutEvent(QFocusEvent *event);

    // do not forward scroll to parents in case of nonscrolling state
    void wheelEvent(QWheelEvent* event) {
        const bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();
        const bool atTop = verticalScrollBar()->value() == verticalScrollBar()->minimum();

        const QPoint delta = event->angleDelta();

        const bool scrollDown = delta.y() < 0;
        const bool scrollUp = delta.y() > 0;

        const bool noWay = (atBottom && scrollDown) || (atTop && scrollUp);

        if(noWay) {
            event->accept();
        } else {
            QTreeView::wheelEvent(event);
        }
    }

private:
    NodeButton<T, Model> *m_button;
    bool m_gonnaDestroy {false};

    void chooseValue(QEvent *event){
        Q_UNUSED(event)

        if(m_gonnaDestroy) {
            return;
        }

        Model* model = getSourceModel();
        const Node<T>* node = model->getItem(getCurrentSourceIndex());
        m_button->setNode(node);
        m_button->setFocus();
        m_button->setTree(nullptr);

        m_gonnaDestroy = true;
        selfDestroy();
    }

    void selfDestroy() {
        close();
        m_button->setState(NodeButtonState::Folded);
        deleteLater();
    }

    Model* getSourceModel() const {
        return qobject_cast<Model*>(getProxyModel()->sourceModel());
    }

    PopupTreeProxyModel* getProxyModel() const {
        return qobject_cast<PopupTreeProxyModel*>(model());
    }

    QModelIndex getSourceIndex(const QModelIndex& index) const {
        return getProxyModel()->mapToSource(index);
    }

    QModelIndex getProxyIndex(const QModelIndex& index) const {
        return getProxyModel()->mapFromSource(index);
    }

    QModelIndex getCurrentSourceIndex() const {
        return getSourceIndex(currentIndex());
    }
};

template <>
void PopupTree<Category, CategoriesModel>::mouseDoubleClickEvent(QMouseEvent *event);

template <>
void PopupTree<Category, CategoriesModel>::focusOutEvent(QFocusEvent *event);

template <>
void PopupTree<Wallet, WalletsModel>::mouseDoubleClickEvent(QMouseEvent *event);

template <>
void PopupTree<Wallet, WalletsModel>::focusOutEvent(QFocusEvent *event);

using CategoryNodeButton = NodeButton<Category, CategoriesModel>;
using WalletNodeButton = NodeButton<Wallet, WalletsModel>;

class RelaxedDoubleSpinBox : public QDoubleSpinBox {
    Q_OBJECT

public:
    explicit RelaxedDoubleSpinBox(QWidget* parent =0) : QDoubleSpinBox(parent)
    {
        setMinimum(-1000000000);
        setMaximum(1000000000);
    }

    QValidator::State validate(QString & text, int & pos) const override
    {
        QString s = QString(text).replace(".", ",");
        return QDoubleSpinBox::validate(s,pos);
    }

    double valueFromText(const QString& text) const override
    {
        QString s = QString(text).replace(",", ".");
        return s.toDouble();
    }
};

} // namespace cashbook

#endif // BOOKKEEPING_WIDGETS_H
