#include "serialization.h"
#include "bookkeeping/bookkeeping.h"

#include "std/pitm/pitmarray.h"
#include "std/pitm/pitmobject.h"
#include "std/pitm/pitmdocument.h"
#include "std/fs.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <utility>
#include <stack>

namespace cashbook {

namespace storage
{

static const int backupCount        {3};
static const QString dateFormat     {"yyyy.MM"};
static const QString ext            {".pitm"};
static const QString backupExt      {".backup"};
static const QString rootDir        {"data"};
static const QString backupDir      {"backup"};
static const QString headName       {"_head"};
static const QString headFile       { QString("%1/%2%3").arg(rootDir, headName, ext) };

static QString monthFile(const QDate &month) {
    return QString("%1/%2%3").arg(rootDir, month.toString(dateFormat), ext);
}

} // namespace storage


//
// Save
//

static void save(const IdableString &data, PitmObject &pitm)
{
    pitm[QLatin1String("id")] = data.id.toString();
    pitm[QLatin1String("str")] = as<QString>(data);
}

static void save(const OwnersModel &data, PitmArray &pitm)
{
    for(const auto &owner : data.owners) {
        PitmObject obj;
        save(owner, obj);
        pitm.append(obj);
    }
}

static void save(const Node<Category> *node, PitmArray &pitmNodes)
{
    PitmObject pitmNode;

    PitmObject category;
    save(node->data, category);
    pitmNode[QLatin1String("category")] = category;
    pitmNode[QLatin1String("regular")] = node->data.regular;

    int children = as<int>(node->childCount());
    if(children) {
        pitmNode[QLatin1String("children")] = children;
    }
    pitmNodes.append(pitmNode);

    for(const auto &child : node->children) {
        save(child, pitmNodes);
    }
}

static void save(const CategoriesModel &data, PitmArray &pitm)
{
    save(data.rootItem, pitm);
}

static void save(const ArchPointer<Owner> &data, PitmObject &pitm)
{
    bool valid = data.isValidPointer();
    pitm[QLatin1String("valid")] = valid;
    if(valid) {
        const Owner *pointer = data.toPointer();
        if(pointer) {
            pitm[QLatin1String("ref")] = pointer->id.toString();
        } else {
            pitm[QLatin1String("ref")] = QUuid().toString();
        }
    } else {
        pitm[QLatin1String("archive")] = data.toString();
    }
}

static void save(const Wallet &wallet, PitmObject &pitm)
{
    pitm[QLatin1String("id")] = wallet.id.toString();
    pitm[QLatin1String("type")] = Wallet::Type::toConfigString(wallet.type);
    pitm[QLatin1String("name")] = wallet.name;
    pitm[QLatin1String("canBeNegative")] = wallet.canBeNegative;
    pitm[QLatin1String("amount")] = wallet.amount.as_cents();

    PitmObject ownerJson;
    save(wallet.owner, ownerJson);
    pitm[QLatin1String("owner")] = ownerJson;
}

static void save(const Node<Wallet> *node, PitmArray &pitmNodes)
{
    PitmObject pitmNode;

    PitmObject wallet;
    save(node->data, wallet);
    pitmNode[QLatin1String("wallet")] = wallet;

    int children = as<int>(node->childCount());
    if(children) {
        pitmNode[QLatin1String("children")] = children;
    }

    pitmNodes.append(pitmNode);

    for(const Node<Wallet> *child : node->children) {
        save(child, pitmNodes);
    }
}

static void save(const WalletsModel &data, PitmArray &pitm)
{
    save(data.rootItem, pitm);
}

template <class T>
static void save(const ArchNode<T> &data, PitmObject &pitm)
{
    bool valid = data.isValidPointer();
    pitm[QLatin1String("valid")] = valid;
    if(valid) {
        const Node<T> *pointer = data.toPointer();
        if(pointer) {
            pitm[QLatin1String("ref")] = pointer->data.id.toString();
        } else {
            pitm[QLatin1String("ref")] = QUuid().toString();
        }
    } else {
        pitm[QLatin1String("archive")] = data.toString();
    }
}

static void save(const Transaction &t, PitmObject &pitm)
{
    pitm[QLatin1String("date")] = t.date.toString(QStringLiteral("dd.MM.yyyy"));

    if(!t.note.isEmpty()) {
        pitm[QLatin1String("note")] = t.note;
    }
    pitm[QLatin1String("type")] = Transaction::Type::toConfigString(t.type);

    PitmObject category;
    save(t.category, category);
    if(t.type != Transaction::Type::Transfer) {
        pitm[QLatin1String("category")] = category;
    }
    pitm[QLatin1String("amount")] = t.amount.as_cents();
    PitmObject from;
    save(t.from, from);
    PitmObject to;
    save(t.to, to);

    if(t.type != Transaction::Type::In) {
        pitm[QLatin1String("from")] = from;
    }

    if(t.type != Transaction::Type::Out) {
        pitm[QLatin1String("to")] = to;
    }
}

static void save(const LogModel &data, PitmArray &pitm)
{
    for(const Transaction &t : data.log) {
        PitmObject obj;
        save(t, obj);
        pitm.append(obj);
    }
}

static void save(const Plan &item, PitmObject &pitm)
{
    if(!item.name.isEmpty()) {
        pitm[QLatin1String("name")] = item.name;
    }

    pitm[QLatin1String("type")] = Transaction::Type::toConfigString(item.type);

    PitmObject category;
    save(item.category, category);
    if(item.type != Transaction::Type::Transfer) {
        pitm[QLatin1String("category")] = category;
    }
    pitm[QLatin1String("amount")] = item.amount.as_cents();
}

static void save(const PlansModel &data, PitmArray &pitm)
{
    for(const Plan &t : data.plans) {
        PitmObject obj;
        save(t, obj);
        pitm.append(obj);
    }
}

static void save(const Plans &data, PitmObject &pitm)
{
    PitmArray shortPlans;
    PitmArray middlePlans;
    PitmArray longPlans;

    save(data.shortPlans, shortPlans);
    save(data.middlePlans, middlePlans);
    save(data.longPlans, longPlans);

    pitm[QLatin1String("short")] = shortPlans;
    pitm[QLatin1String("middle")] = middlePlans;
    pitm[QLatin1String("long")] = longPlans;
}

static void save(const Task &item, PitmObject &pitm)
{
    pitm[QLatin1String("type")] = Transaction::Type::toConfigString(item.type);

    PitmObject category;
    save(item.category, category);
    if(item.type != Transaction::Type::Transfer) {
        pitm[QLatin1String("category")] = category;
    }
    pitm[QLatin1String("from")] = item.from.toString(QStringLiteral("dd.MM.yyyy"));
    pitm[QLatin1String("to")] = item.to.toString(QStringLiteral("dd.MM.yyyy"));
    pitm[QLatin1String("amount")] = item.amount.as_cents();
}

static void save(const Tasks &data, PitmArray &pitm)
{
    for(const Task &task : data.active.tasks) {
        PitmObject obj;
        save(task, obj);
        pitm.append(obj);
    }

    for(const Task &task : data.completed.tasks) {
        PitmObject obj;
        save(task, obj);
        pitm.append(obj);
    }
}

static void saveLog(const Data &data, PitmObject &pitm)
{
    PitmArray log;

    save(data.log, log);
    pitm[QLatin1String("log")] = log;
}

static QString backupFile(const QString &originalFile, int number)
{
    QFileInfo info(originalFile);
    QString filePath = info.path();
    QString fileName = info.fileName();
    QString fileBase = fileName.left(fileName.lastIndexOf('.'));

    return QString("%1/%2/%3%4.%5").arg(filePath, storage::backupDir, fileBase, storage::backupExt, QString::number(number));
}

template <class ObjectOrArray>
static void saveFile(const QString &fileName, ObjectOrArray data)
{
    // 1. backup file

    // if storage::backupCount == 3 then
    // file.backup.2 -> file.backup.3
    // file.backup.1 -> file.backup.2
    for(int i = storage::backupCount; i>=2; --i) {
        QString from { backupFile(fileName, i-1) };
        QString to   { backupFile(fileName, i)   };
        aske::copyFileForced(from, to);
    }

    // file.pitm -> file.backup.1
    if(storage::backupCount >= 1) {
        aske::copyFileForced(fileName, backupFile(fileName, 1));
    }

    // 2. save file
    PitmDocument saveDoc(data);
    QFile f(fileName);
    f.open(QIODevice::WriteOnly);
    f.write(saveDoc.toPitm());
    f.close();
}

static void saveLog(Data &data)
{
    const auto &log = data.log.log;
    auto &changedMonths = data.log.changedMonths;

    if(log.empty()) {
        return;
    }

    if(changedMonths.empty()) {
        return;
    }

    for(const Month &month : changedMonths) {
        PitmArray monthArray;
        for(const Transaction &t : log) {
            Month tMonth {t.date};

            if(tMonth != month) {
                continue;
            }

            PitmObject obj;
            save(t, obj);
            monthArray.append(obj);
        }

        saveFile(storage::monthFile(month.toDate()), monthArray);
    }

    changedMonths.clear();
}

static void saveHead(const Data &data, PitmObject &pitm)
{
    PitmArray owners;
    PitmArray wallets;
    PitmArray inCategories;
    PitmArray outCategories;
    PitmObject plans;
    PitmArray tasks;

    save(data.owners, owners);
    save(data.wallets, wallets);
    save(data.inCategories, inCategories);
    save(data.outCategories, outCategories);
    save(data.plans, plans);
    save(data.tasks, tasks);

    pitm[QLatin1String("owners")] = owners;
    pitm[QLatin1String("wallets")] = wallets;
    pitm[QLatin1String("inCategories")] = inCategories;
    pitm[QLatin1String("outCategories")] = outCategories;
    pitm[QLatin1String("plans")] = plans;
    pitm[QLatin1String("tasks")] = tasks;
    pitm[QLatin1String("unanchored")] = data.log.unanchored;
}

static void saveHead(const Data &data)
{
    PitmObject pitm;
    saveHead(data, pitm);

    saveFile(storage::headFile, pitm);
}

void save(Data &data)
{
    // make sure that we have existing root and backup directories (i.e. "data/backup")
    QDir().mkpath(QString("%1/%2").arg(storage::rootDir, storage::backupDir));

    saveHead(data);
    saveLog(data);
}

//
// Load
//
static void load(IdableString &data, PitmObject pitm)
{
    QString id = pitm[QLatin1String("id")].toString();
    data.id = QUuid(id);
    as<QString &>(data) = pitm[QLatin1String("str")].toString();
}

static void load(OwnersModel &data, PitmArray arr)
{
    for(PitmValue v : std::as_const(arr)) {
        PitmObject obj = v.toObject();
        Owner owner;
        load(owner, obj);
        data.owners.push_back(owner);
    }
}

static void load(CategoriesModel &data, PitmArray arr)
{
    Node<Category> *currNode = data.rootItem;
    std::stack<int> children;

    for(PitmValue v : std::as_const(arr)) {
        PitmObject nodeObj = v.toObject();
        PitmObject categoryObj = nodeObj[QLatin1String("category")].toObject();
        currNode->data.regular = nodeObj[QLatin1String("regular")].toBool();
        load(currNode->data, categoryObj);
        int ch = nodeObj[QLatin1String("children")].toInt(0);
        children.push(ch);

        while(!children.empty() && children.top() == 0) {
            currNode = currNode->parent;
            children.pop();
        }

        if(!children.empty()) {
            currNode = currNode->addChild(Category());
            children.top() -= 1;
        }
    }
}

static void load(ArchPointer<Owner> &data, PitmObject pitm, const OwnersModel &ownersModel)
{
    bool valid = pitm[QLatin1String("valid")].toBool();

    if(valid) {
        QString id = pitm[QLatin1String("ref")].toString();
        QUuid uid = QUuid{id};

        if(!uid.isNull()) {
            for(const Owner &owner : ownersModel.owners) {
                if(owner.id == uid) {
                    data = &owner;
                    return;
                }
            }
        } else {
            data = as<const Owner*>(nullptr);
        }
    } else {
        data = pitm[QLatin1String("archive")].toString();
    }
}

static void load(Wallet &wallet, PitmObject pitm, const OwnersModel &ownersModel)
{
    QString id = pitm[QLatin1String("id")].toString();
    wallet.id = QUuid(id);
    wallet.type = Wallet::Type::fromConfigString( pitm[QLatin1String("type")].toString() );
    wallet.name = pitm[QLatin1String("name")].toString();
    wallet.canBeNegative = pitm[QLatin1String("canBeNegative")].toBool();
    wallet.amount = Money(as<intmax_t>(pitm[QLatin1String("amount")].toInt()));

    PitmObject ownerObj = pitm[QLatin1String("owner")].toObject();
    load(wallet.owner, ownerObj, ownersModel);
}

static void load(WalletsModel &data, PitmArray arr, const OwnersModel &ownersModel)
{
    Node<Wallet> *currNode = data.rootItem;
    std::stack<int> children;

    for(PitmValue v : std::as_const(arr)) {
        PitmObject nodeObj = v.toObject();
        PitmObject walletObj = nodeObj[QLatin1String("wallet")].toObject();
        load(currNode->data, walletObj, ownersModel);
        int ch = nodeObj[QLatin1String("children")].toInt(0);
        children.push(ch);

        while(!children.empty() && children.top() == 0) {
            currNode = currNode->parent;
            children.pop();
        }

        if(!children.empty()) {
            currNode = currNode->addChild(Wallet());
            children.top() -= 1;
        }
    }

    emit data.recalculated();
}

template <class T, class Model>
static void load(ArchNode<T> &data, PitmObject pitm, const Model &refModel)
{
    bool valid = pitm[QLatin1String("valid")].toBool();

    if(valid) {
        QString id = pitm[QLatin1String("ref")].toString();
        QUuid uid = QUuid{id};

        if(!uid.isNull()) {
            auto nodes = refModel.rootItem->toList();
            for(const Node<T> *obj : nodes) {
                if(obj->data.id == uid) {
                    data = obj;
                    return;
                }
            }
        } else {
            data = as<const Node<T>*>(nullptr);
        }
    } else {
        data = pitm[QLatin1String("archive")].toString();
    }


    if(valid) {
        const Node<T> *pointer = data.toPointer();
        if(pointer) {
            pitm[QLatin1String("ref")] = pointer->data.id.toString();
        } else {
            pitm[QLatin1String("ref")] = QUuid().toString();
        }
    } else {
        pitm[QLatin1String("archive")] = data.toString();
    }
}

static void load(Transaction &t, PitmObject pitm,
                 const WalletsModel &wallets,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
) {
    t.date = QDate::fromString(pitm[QLatin1String("date")].toString(), "dd.MM.yyyy");
    t.note = pitm[QLatin1String("note")].toString();
    t.type = Transaction::Type::fromConfigString( pitm[QLatin1String("type")].toString() );
    t.amount = Money(as<intmax_t>(pitm[QLatin1String("amount")].toInt()));

    if(t.type != Transaction::Type::Transfer) {
        PitmObject categoryObj = pitm[QLatin1String("category")].toObject();
        ArchNode<Category> category;
        if(t.type == Transaction::Type::In) {
            load(category, categoryObj, inCategories);
        } else if(t.type == Transaction::Type::Out) {
            load(category, categoryObj, outCategories);
        }
        t.category = category;
    }

    if(t.type != Transaction::Type::In) {
        PitmObject from = pitm[QLatin1String("from")].toObject();
        load(t.from, from, wallets);
    }

    if(t.type != Transaction::Type::Out) {
        PitmObject to = pitm[QLatin1String("to")].toObject();
        load(t.to, to, wallets);
    }
}

static void load(LogModel &logModel, PitmArray pitm, Data &data){
    int n = pitm.size();
    for (int i = n-1; i>=0; --i) {
        PitmObject tObj = pitm[i].toObject();
        Transaction t;
        load(t, tObj, data.wallets, data.inCategories, data.outCategories);
        logModel.log.push_front(t);

        // update brief statistics
        if(t.type == Transaction::Type::In) {
            Month month(t.date);
            logModel.statistics.brief[month].common.received += t.amount;
            const auto &archNode = t.category;
            if(archNode.isValidPointer() && archNode.toPointer() && archNode.toPointer()->data.regular) {
                logModel.statistics.brief[month].regular.received += t.amount;
            }
        }

        if(t.type == Transaction::Type::Out) {
            Month month(t.date);
            logModel.statistics.brief[month].common.spent += t.amount;
            const auto &archNode = t.category;
            if(archNode.isValidPointer() && archNode.toPointer() && archNode.toPointer()->data.regular) {
                logModel.statistics.brief[month].regular.spent += t.amount;
            }
        }
    }
}

static void load(Plan &item, PitmObject pitm,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
) {
    item.name = pitm[QLatin1String("name")].toString();
    item.type = Transaction::Type::fromConfigString( pitm[QLatin1String("type")].toString() );

    if(item.type != Transaction::Type::Transfer) {
        PitmObject categoryObj = pitm[QLatin1String("category")].toObject();
        ArchNode<Category> category;
        if(item.type == Transaction::Type::In) {
            load(category, categoryObj, inCategories);
        } else if(item.type == Transaction::Type::Out) {
            load(category, categoryObj, outCategories);
        }
        item.category = category;
    }

    item.amount = Money(as<intmax_t>(pitm[QLatin1String("amount")].toInt()));
}

static void load(PlansModel &data, PitmArray arr,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
)
{
    for(PitmValue v : std::as_const(arr)) {
        PitmObject obj = v.toObject();
        Plan item;
        load(item, obj, inCategories, outCategories);
        data.plans.push_back(item);
    }
}

static void load(Plans &data, PitmObject pitm,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
)
{
    load(data.shortPlans, pitm[QLatin1String("short")].toArray(), inCategories, outCategories);
    load(data.middlePlans, pitm[QLatin1String("middle")].toArray(), inCategories, outCategories);
    load(data.longPlans, pitm[QLatin1String("long")].toArray(), inCategories, outCategories);
}

static void load(Task &item, PitmObject pitm,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
) {
    item.type = Transaction::Type::fromConfigString( pitm[QLatin1String("type")].toString() );

    if(item.type != Transaction::Type::Transfer) {
        PitmObject categoryObj = pitm[QLatin1String("category")].toObject();
        ArchNode<Category> category;
        if(item.type == Transaction::Type::In) {
            load(category, categoryObj, inCategories);
        } else if(item.type == Transaction::Type::Out) {
            load(category, categoryObj, outCategories);
        }
        item.category = category;
    }

    item.from = QDate::fromString(pitm[QLatin1String("from")].toString(), "dd.MM.yyyy");
    item.to = QDate::fromString(pitm[QLatin1String("to")].toString(), "dd.MM.yyyy");

    item.amount = Money(as<intmax_t>(pitm[QLatin1String("amount")].toInt()));
}

static void load(Tasks &data, PitmArray arr,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
)
{
    for(PitmValue v : std::as_const(arr)) {
        PitmObject obj = v.toObject();
        Task item;
        load(item, obj, inCategories, outCategories);
        if(item.to >= today) {
            data.active.tasks.push_back(item);
        } else {
            data.completed.tasks.push_back(item);
        }
    }
}

static void loadHead(Data &data, PitmObject pitm)
{
    load(data.owners, pitm[QLatin1String("owners")].toArray());
    load(data.wallets, pitm[QLatin1String("wallets")].toArray(), data.owners);
    load(data.inCategories, pitm[QLatin1String("inCategories")].toArray());
    load(data.outCategories, pitm[QLatin1String("outCategories")].toArray());
    load(data.plans, pitm[QLatin1String("plans")].toObject(), data.inCategories, data.outCategories);
    load(data.tasks, pitm[QLatin1String("tasks")].toArray(), data.inCategories, data.outCategories);
    data.log.unanchored = pitm[QLatin1String("unanchored")].toInt();
}

static void loadMonth(Data &data, PitmArray pitm)
{
    load(data.log, pitm, data);
}

static void loadLog(Data &data)
{
    QDirIterator it(storage::rootDir, {QStringLiteral("*")+storage::ext}, QDir::Files);
    while (it.hasNext()) {
        it.next();

        QFile f(it.filePath());
        f.open(QIODevice::ReadOnly);
        QByteArray bytes = f.readAll();
        f.close();

        PitmDocument loadDoc(PitmDocument::fromPitm(bytes));
        PitmArray pitm = loadDoc.array();
        loadMonth(data, pitm);
    }

    QDate monthBegin(today.year(), today.month(), 1);
    data.loadCategoriesStatistics(monthBegin, today);
    data.updateTasks();
}

static void loadHead(Data &data)
{
    QFileInfo info(storage::headFile);
    if(!info.exists()) {
        return;
    }

    QFile f(storage::headFile);
    f.open(QIODevice::ReadOnly);
    QByteArray bytes = f.readAll();
    f.close();

    PitmDocument loadDoc(PitmDocument::fromPitm(bytes));
    PitmObject pitm = loadDoc.object();
    loadHead(data, pitm);
}

void load(Data &data)
{
    data.clear();

    loadHead(data);
    loadLog(data);
}

} // namespace cashbook
