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

    void setModel(QAbstractItemModel &model, bool setParent = false) {
        connect(this, &QPushButton::clicked, [this, &model, setParent]() {
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
    }

    const Node<T> *node() const {
        return m_node;
    }

private:
    NodeButtonState m_state {NodeButtonState::Folded};
    const Node<T> *m_node {nullptr};
    QTreeView *m_tree {nullptr};
};

class PopupTreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PopupTreeProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        m_timer.callOnTimeout([this](){
            doFilterWork();
        });
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    void setFilterString(const QString& filterString) {
        m_filterString = filterString;
        //qDebug() << "set filter:" << m_filterString;

        m_timer.stop();
        m_timer.start(500);
    }

    void doFilterWork() {
        invalidate();
    }

private:
    QString m_filterString;
    QTimer m_timer;
};

template <class T, class Model>
class PopupTree : public QWidget
{
public:
    PopupTree(QAbstractItemModel &model, NodeButton<T, Model> *button, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_treeView(new QTreeView())
        , m_button(button)
        , m_filterLine(new QLineEdit(this))
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(m_filterLine, 5);
        layout->addWidget(m_treeView);
        //setLayout(layout);

        PopupTreeProxyModel* proxy = new PopupTreeProxyModel(parent);
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

        const Node<T> *node = button->node();
        m_treeView->setCurrentIndex(node ? getProxyIndex(qobject_cast<Model*>(&model)->itemIndex(node)) : QModelIndex());
        move(button->mapToGlobal(QPoint(0, 0)));
        show();
        activateWindow();
        setFocus();

        connect(m_filterLine, &QLineEdit::textEdited, proxy, &PopupTreeProxyModel::setFilterString);
        connect(m_filterLine, &QLineEdit::textEdited, this, [this](){update();});
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void focusOutEvent(QFocusEvent *event);

    // do not forward scroll to parents in case of nonscrolling state
    void wheelEvent(QWheelEvent* event) {
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

private:
    QTreeView* m_treeView {nullptr};
    NodeButton<T, Model>* m_button {nullptr};
    QLineEdit* m_filterLine {nullptr};
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
