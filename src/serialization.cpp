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
static const QString headBackupFile { QString("%1/%2/%3%4").arg(rootDir, backupDir, headName, backupExt) };

static QString monthFile(const QDate &month) {
    return QString("%1/%2%3").arg(rootDir, month.toString(dateFormat), ext);
}

static QString monthBackupFile(const QDate &month, int n) {
    return QString("%1/%2/%3%4.%5").arg(rootDir, backupDir, month.toString(dateFormat), backupExt, QString::number(n));
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

static void saveLog(const Data &data, PitmObject &pitm)
{
    PitmArray log;

    save(data.log, log);
    pitm[QLatin1String("log")] = log;
}

static bool isSameMonth(const QDate &d1, const QDate &d2)
{
    return d1.year() == d2.year() && d1.month() == d2.month();
}

static void saveMonth(const PitmArray &array, const QDate &date)
{
    QFile f(storage::monthFile(date));
    f.open(QIODevice::WriteOnly);

    PitmDocument saveDoc(array);

    f.write(saveDoc.toPitm());
    f.close();
}

static void saveLog(const Data &data)
{
    if(data.log.log.empty()) {
        return;
    }

    QDate monthDate = data.log.log[0].date;
    PitmArray monthArray;

    for(const Transaction &t : data.log.log) {
        if(!isSameMonth(t.date, monthDate)) {
            saveMonth(monthArray, monthDate);
            monthArray = PitmArray(); // clear
            monthDate = t.date;
        }

        PitmObject obj;
        save(t, obj);
        monthArray.append(obj);
    }

    saveMonth(monthArray, monthDate); // most earlier month
}

static void saveHead(const Data &data, PitmObject &pitm)
{
    PitmArray owners;
    PitmArray wallets;
    PitmArray inCategories;
    PitmArray outCategories;

    save(data.owners, owners);
    save(data.wallets, wallets);
    save(data.inCategories, inCategories);
    save(data.outCategories, outCategories);

    pitm[QLatin1String("owners")] = owners;
    pitm[QLatin1String("wallets")] = wallets;
    pitm[QLatin1String("inCategories")] = inCategories;
    pitm[QLatin1String("outCategories")] = outCategories;
    pitm[QLatin1String("unanchored")] = data.log.unanchored;
}

void saveHead(const Data &data)
{
    QFile f(storage::headFile);
    f.open(QIODevice::WriteOnly);

    PitmObject pitm;
    saveHead(data, pitm);

    PitmDocument saveDoc(pitm);

    f.write(saveDoc.toPitm());
    f.close();
}

void saveBackups()
{
    QDirIterator it(storage::rootDir, {QStringLiteral("*.pitm")} , QDir::Files);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        QString backup = QString("%1/%2/%3").arg(info.path(), storage::backupDir, info.fileName());
        aske::copyFileForced(it.filePath(), backup);
    }
}

void save(const Data &data)
{
    // TODO: make proper multilevel backup
    QDir().mkpath(QString("%1/%2").arg(storage::rootDir, storage::backupDir));

    saveBackups();

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
            if(archNode.isValidPointer() && archNode.toPointer()->data.regular) {
                logModel.statistics.brief[month].regular.received += t.amount;
            }
        }

        if(t.type == Transaction::Type::Out) {
            Month month(t.date);
            logModel.statistics.brief[month].common.spent += t.amount;
            const auto &archNode = t.category;
            if(archNode.isValidPointer() && archNode.toPointer()->data.regular) {
                logModel.statistics.brief[month].regular.spent += t.amount;
            }
        }
    }
}

static void load(Data &data, PitmObject pitm)
{
    load(data.owners, pitm[QLatin1String("owners")].toArray());
    load(data.wallets, pitm[QLatin1String("wallets")].toArray(), data.owners);
    load(data.inCategories, pitm[QLatin1String("inCategories")].toArray());
    load(data.outCategories, pitm[QLatin1String("outCategories")].toArray());
    load(data.log, pitm[QLatin1String("log")].toArray(), data);

    data.log.unanchored = pitm[QLatin1String("unanchored")].toInt();

    QDate monthBegin(today.year(), today.month(), 1);

    data.loadCategoriesStatistics(monthBegin, today);
}

static void loadHead(Data &data, PitmObject pitm)
{
    load(data.owners, pitm[QLatin1String("owners")].toArray());
    load(data.wallets, pitm[QLatin1String("wallets")].toArray(), data.owners);
    load(data.inCategories, pitm[QLatin1String("inCategories")].toArray());
    load(data.outCategories, pitm[QLatin1String("outCategories")].toArray());
    data.log.unanchored = pitm[QLatin1String("unanchored")].toInt();
}

void loadMonths(Data &data)
{


}

void loadHead(Data &data)
{
    QFileInfo f(storage::headFile);
    if(!f.exists()) {
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
    loadMonths(data);
}

/*void load(Data &data)
{
    data.clear();

    QFile f("cashbook.pitm");
    f.open(QIODevice::ReadOnly);
    QByteArray bytes = f.readAll();
    f.close();

    PitmDocument loadDoc(PitmDocument::fromPitm(bytes));
    PitmObject pitm = loadDoc.object();
    load(data, pitm);
}*/

} // namespace cashbook
