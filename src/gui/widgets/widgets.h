#ifndef BOOKKEEPING_WIDGETS_H
#define BOOKKEEPING_WIDGETS_H

#include "bookkeeping/models.h"
#include <QTreeView>
#include <QMouseEvent>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTimer>

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
            QWidget *widget = new PopupTree<T, Model>(model, this, setParent ? this : nullptr);
            Q_UNUSED(widget);
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
    PopupTreeProxyModel(QObject* parent = nullptr);

    void setFilterString(const QString& filterString);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

Q_SIGNALS:
    void filtered();
    void filterCanceled();

private:
    void _doFilterWork();
    bool _doFilterIndex(QModelIndex currIndex);
    bool _filterPass(QModelIndex currIndex);
    bool _isFiltered() const;

    QString m_filterString;
    QTimer m_timer;
    QSet<QModelIndex> m_filtereItems;
    QModelIndex m_mostMatched;
};

template <class T, class Model>
class PopupTree : public QWidget
{
public:
    PopupTree(QAbstractItemModel &model, NodeButton<T, Model> *button, QWidget *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    // do not forward scroll to parents in case of nonscrolling state
    void wheelEvent(QWheelEvent* event) override;

private:
    QTreeView* m_treeView {nullptr};
    NodeButton<T, Model>* m_button {nullptr};
    QLineEdit* m_filterLine {nullptr};
    bool m_gonnaDestroy {false};

    QVector<QMetaObject::Connection> m_connections;

    void chooseValue(QEvent *event);
    void selfDestroy();

    Model* getSourceModel() const {
        return qobject_cast<Model*>(getProxyModel()->sourceModel());
    }

    PopupTreeProxyModel* getProxyModel() const {
        return qobject_cast<PopupTreeProxyModel*>(m_treeView->model());
    }

    QModelIndex getSourceIndex(const QModelIndex& index) const {
        return getProxyModel()->mapToSource(index);
    }

    QModelIndex getProxyIndex(const QModelIndex& index) const {
        return getProxyModel()->mapFromSource(index);
    }

    QModelIndex getCurrentSourceIndex() const {
        return getSourceIndex(m_treeView->currentIndex());
    }
};

template <class T, class Model>
PopupTree<T, Model>::PopupTree(QAbstractItemModel &model, NodeButton<T, Model> *button, QWidget *parent /*= nullptr*/)
    : QWidget(parent)
    , m_treeView(new QTreeView(this))
    , m_button(button)
    , m_filterLine(new QLineEdit(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_filterLine, 5);
    layout->addWidget(m_treeView);

    PopupTreeProxyModel* proxy = new PopupTreeProxyModel(this);
    proxy->setSourceModel(&model);

    m_treeView->setModel(proxy);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::Tool);

    m_treeView->setStyleSheet("QTreeView::item { height: 18px;}");
    m_treeView->setAlternatingRowColors(true);
    m_treeView->header()->hide();
    for(int i = WalletColumn::Name+1; i<WalletColumn::Count; ++i) {
        m_treeView->hideColumn(i);
    }
    button->setTree(m_treeView);

    setFixedHeight(500);

    connect(m_treeView, &QTreeView::doubleClicked, [this]() {
        mouseDoubleClickEvent(nullptr);
    });

    const Node<T> *node = button->node();
    m_treeView->setCurrentIndex(node ? getProxyIndex(qobject_cast<Model*>(&model)->itemIndex(node)) : QModelIndex());
    move(button->mapToGlobal(QPoint(0, 0)));
    show();
    activateWindow();
    m_filterLine->setFocus();

    auto conn1 = connect(m_filterLine, &QLineEdit::textEdited, proxy, &PopupTreeProxyModel::setFilterString);
    auto conn2 = connect(static_cast<PopupTreeProxyModel*>(m_treeView->model()), &PopupTreeProxyModel::filtered, [this]() {
        m_treeView->expandAll();
    });
    auto conn3 = connect(static_cast<PopupTreeProxyModel*>(m_treeView->model()), &PopupTreeProxyModel::filterCanceled, [this]() {
        m_treeView->collapseAll();
    });

    m_connections.push_back(conn1);
    m_connections.push_back(conn2);
    m_connections.push_back(conn3);
}

template <>
void PopupTree<Category, CategoriesModel>::mouseDoubleClickEvent(QMouseEvent *event);

template <>
void PopupTree<Category, CategoriesModel>::focusOutEvent(QFocusEvent *event);

template <>
void PopupTree<Wallet, WalletsModel>::mouseDoubleClickEvent(QMouseEvent *event);

template <>
void PopupTree<Wallet, WalletsModel>::focusOutEvent(QFocusEvent *event);

template <class T, class Model>
void PopupTree<T, Model>::keyPressEvent(QKeyEvent *event) {
    // DO NOT WORK for some reason!!

    if (event->key() == Qt::Key_Enter) {
        mouseDoubleClickEvent(nullptr);
    } else {
        QWidget::keyPressEvent(event);
    }
}

template <class T, class Model>
void PopupTree<T, Model>::wheelEvent(QWheelEvent* event) {
    const bool atBottom = m_treeView->verticalScrollBar()->value() == m_treeView->verticalScrollBar()->maximum();
    const bool atTop = m_treeView->verticalScrollBar()->value() == m_treeView->verticalScrollBar()->minimum();

    const QPoint delta = event->angleDelta();

    const bool scrollDown = delta.y() < 0;
    const bool scrollUp = delta.y() > 0;

    const bool noWay = (atBottom && scrollDown) || (atTop && scrollUp);

    if(noWay) {
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

template <class T, class Model>
void PopupTree<T, Model>::chooseValue(QEvent *event){
    Q_UNUSED(event)

    if(m_gonnaDestroy) {
        return;
    }

    Model* model = getSourceModel();
    const Node<T>* node = model->getItem(getCurrentSourceIndex());
    m_button->setNode(node);
    m_button->setFocus();
    m_button->setTree(nullptr);

    selfDestroy();
}

template <class T, class Model>
void PopupTree<T, Model>::selfDestroy() {
    m_gonnaDestroy = true;

    for (const auto& conn : m_connections) {
        disconnect(conn);
    }

    close();
    m_button->setState(NodeButtonState::Folded);
    deleteLater();
}

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
