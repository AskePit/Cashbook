#include "modelsdelegate.h"
#include "gui/widgets/widgets.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QApplication>
#include <QPainter>

namespace cashbook
{

static int categoryNodeType = qRegisterMetaType<const Node<Category> *>();
static int walletNodeType = qRegisterMetaType<const Node<Wallet> *>();
static int transactionTypeType = qRegisterMetaType<Transaction::Type::t>();

ModelsDelegate::ModelsDelegate(DataModels &dataModels, QObject* parent)
    : QItemDelegate(parent)
    , m_data(dataModels)
{}

ModelsDelegate::~ModelsDelegate()
{}

void ModelsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.data().metaType().id() != QMetaType::Bool) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionButton style;
    style.state = QStyle::State_Enabled;
    style.state |= index.data().toBool() ? QStyle::State_On : QStyle::State_Off;
    style.direction = QApplication::layoutDirection();
    style.rect = option.rect;

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }
    QApplication::style()->drawControl(QStyle::CE_CheckBox, &style, painter);
}

QWidget* ModelsDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QVariant data = index.data(Qt::EditRole);
    int type = data.metaType().id();

    if(type == QMetaType::Type::Bool) {
        return new QCheckBox(parent);
    }

    if(type == QMetaType::Type::Double) {
        return new RelaxedDoubleSpinBox(parent);
    }

    if(type == QMetaType::Type::QDate) {
        QDateEdit *edit = new QDateEdit(parent);
        edit->setCalendarPopup(true);

        bool log = !!qobject_cast<const LogModel*>(index.model());
        if(log) {
            QDate min;
            QDate max {Today};
            if(static_cast<int>(m_data.logModel.m_data.log.size()) > index.row()+1) {
                min = m_data.logModel.m_data.log[static_cast<size_t>(index.row()+1)].date;
            }

            if(min == max) {
                return nullptr;
            }

            edit->setMinimumDate(min);
            edit->setMaximumDate(max);
        }

        return edit;
    }

    // custom types
    if(type == transactionTypeType) {
        QComboBox *box = new QComboBox(parent);
        QVector<Transaction::Type::t> v {Transaction::Type::In, Transaction::Type::Out};

        bool tasks = !!qobject_cast<const TasksModel*>(index.model());
        if(!tasks) {
            v << Transaction::Type::Transfer;
        }

        for(auto t : v) {
            box->addItem(Transaction::Type::toString(t), static_cast<int>(t));
        }
        return box;
    }

    if(type == categoryNodeType) {
        int column = LogColumn::Type;
        if(qobject_cast<const LogModel*>(index.model())) {
            column = LogColumn::Type;
        } else if(qobject_cast<const PlansModel*>(index.model())) {
            column = PlansColumn::Type;
        } else if(qobject_cast<const TasksModel*>(index.model())) {
            column = TasksColumn::Type;
        }

        QModelIndex typeIndex = index.model()->index(index.row(), column);
        QVariant data = typeIndex.data(Qt::EditRole);
        Transaction::Type::t type = static_cast<Transaction::Type::t>(data.toInt());
        if(type == Transaction::Type::Transfer) {
            return nullptr;
        }

        CategoriesModel &categories =
                type == Transaction::Type::In ?
                    m_data.inCategoriesModel :
                    m_data.outCategoriesModel;

        CategoryNodeButton *button = new CategoryNodeButton(categories, parent);
        return button;
    }

    if(type == walletNodeType) {
        // assume that only LogModel uses wallets tree editor
        QModelIndex typeIndex = index.model()->index(index.row(), LogColumn::Type);
        QVariant data = typeIndex.data(Qt::EditRole);
        Transaction::Type::t type = static_cast<Transaction::Type::t>(data.toInt());

        int col = index.column();

        if((col == LogColumn::From && type == Transaction::Type::In)
        || (col == LogColumn::To   && type == Transaction::Type::Out)) {
            return nullptr;
        }

        WalletNodeButton *button = new WalletNodeButton(m_data.walletsModel, parent);
        return button;
    }

    // should not reach this code, but just to be sure
    return QItemDelegate::createEditor(parent, option, index);
}

void ModelsDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    // bool case
    if (QCheckBox *box = qobject_cast<QCheckBox*>(editor)) {
        box->setChecked(index.data(Qt::EditRole).toBool());
        return;
    }

    // double case
    if (RelaxedDoubleSpinBox *box = qobject_cast<RelaxedDoubleSpinBox*>(editor)) {
        box->setValue(index.data(Qt::EditRole).toDouble());
        return;
    }

    // Date case
    if (QDateEdit *edit = qobject_cast<QDateEdit*>(editor)) {
        QDate date = index.data(Qt::EditRole).toDate();
        edit->setDate(date);
        return;
    }

    // Transaction type case
    if (QComboBox *box = qobject_cast<QComboBox*>(editor)) {
        QVariant value = index.data(Qt::EditRole);
        int i = box->findData(value);
        if(i >= 0) {
            box->setCurrentIndex(i);
        }
        return;
    }

    // trees case
    CategoryNodeButton *cat = dynamic_cast<CategoryNodeButton *>(editor);
    WalletNodeButton *wal = dynamic_cast<WalletNodeButton *>(editor);

    QVariant value = index.data(Qt::EditRole);

    // Categories tree
    if(cat) {
       const ArchNode<Category> archNode = value;
       if(!archNode.isValidPointer()) {
           QItemDelegate::setEditorData(editor, index);
           return;
       }

       const Node<Category> *node = archNode.toPointer();
       cat->setNode(node);
       return;

    // Wallets tree
    } else if(wal) {
        ArchNode<Wallet> archNode = value;
        if(!archNode.isValidPointer()) {
            QItemDelegate::setEditorData(editor, index);
            return;
        }

        const Node<Wallet> *node = archNode.toPointer();
        wal->setNode(node);
        return;
    }

    QItemDelegate::setEditorData(editor, index);
}

void ModelsDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    // bool case
    if (QCheckBox* box = qobject_cast<QCheckBox*>(editor)) {
        model->setData(index, box->isChecked(), Qt::EditRole);
        return;
    }

    // Date case
    if (QDateEdit *edit = qobject_cast<QDateEdit*>(editor)) {
        model->setData(index, edit->date(), Qt::EditRole);
        return;
    }

    // Transaction type case
    if (QComboBox* box = qobject_cast<QComboBox*>(editor)) {
        model->setData(index, box->currentData(), Qt::EditRole);
        return;
    }

    // trees case
    CategoryNodeButton *cat = dynamic_cast<CategoryNodeButton *>(editor);
    WalletNodeButton *wal = dynamic_cast<WalletNodeButton *>(editor);

    // Categories tree
    if(cat) {
       model->setData(index, ArchNode<Category>(cat->node()), Qt::EditRole);
       return;

    // Wallets tree
    } else if(wal) {
        model->setData(index, ArchNode<Wallet>(wal->node()), Qt::EditRole);
        return;
    }

    QItemDelegate::setModelData(editor, model, index);
}

bool ModelsDelegate::eventFilter(QObject *object, QEvent *event)
{
    CategoryNodeButton *cat = dynamic_cast<CategoryNodeButton *>(object);
    WalletNodeButton *wal = dynamic_cast<WalletNodeButton *>(object);

    NodeButtonState state {cat ? cat->state() : wal ? wal->state() : NodeButtonState::Folded};

    if(state == NodeButtonState::Expanded && event->type() == QEvent::FocusOut) { // for expaned case we do not want to close editor
        return true;
    }

    return QItemDelegate::eventFilter(object, event);
}

} // namespace cashbook
