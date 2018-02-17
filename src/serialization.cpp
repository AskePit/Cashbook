#include "serialization.h"
#include "bookkeeping/bookkeeping.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <utility>
#include <stack>

namespace cashbook {

//
// Save
//

static void save(const IdableString &data, QJsonObject &json)
{
    json[QLatin1String("id")] = data.id.toString();
    json[QLatin1String("str")] = as<QString>(data);
}

static void save(const OwnersModel &data, QJsonArray &json)
{
    for(const auto &owner : data.owners) {
        QJsonObject obj;
        save(owner, obj);
        json.append(obj);
    }
}

static void save(const Node<Category> *node, QJsonArray &jsonNodes)
{
    QJsonObject jsonNode;

    QJsonObject category;
    save(node->data, category);
    jsonNode[QLatin1String("category")] = category;
    jsonNode[QLatin1String("regular")] = node->data.regular;
    jsonNode[QLatin1String("children")] = as<int>(node->childCount());
    jsonNodes.append(jsonNode);

    for(const auto &child : node->children) {
        save(child, jsonNodes);
    }
}

static void save(const CategoriesModel &data, QJsonArray &json)
{
    save(data.rootItem, json);
}

static void save(const ArchPointer<Owner> &data, QJsonObject &json)
{
    bool valid = data.isValidPointer();
    json[QLatin1String("valid")] = valid;
    if(valid) {
        const Owner *pointer = data.toPointer();
        if(pointer) {
            json[QLatin1String("ref")] = pointer->id.toString();
        } else {
            json[QLatin1String("ref")] = QUuid().toString();
        }
    } else {
        json[QLatin1String("archive")] = data.toString();
    }
}

static void save(const Wallet &wallet, QJsonObject &json)
{
    json[QLatin1String("id")] = wallet.id.toString();
    json[QLatin1String("type")] = Wallet::Type::toConfigString(wallet.type);
    json[QLatin1String("name")] = wallet.name;
    json[QLatin1String("canBeNegative")] = wallet.canBeNegative;
    json[QLatin1String("amount")] = wallet.amount.as_cents();

    QJsonObject ownerJson;
    save(wallet.owner, ownerJson);
    json[QLatin1String("owner")] = ownerJson;
}

static void save(const Node<Wallet> *node, QJsonArray &jsonNodes)
{
    QJsonObject jsonNode;

    QJsonObject wallet;
    save(node->data, wallet);
    jsonNode[QLatin1String("wallet")] = wallet;
    jsonNode[QLatin1String("children")] = as<int>(node->childCount());
    jsonNodes.append(jsonNode);

    for(const Node<Wallet> *child : node->children) {
        save(child, jsonNodes);
    }
}

static void save(const WalletsModel &data, QJsonArray &json)
{
    save(data.rootItem, json);
}

template <class T>
static void save(const ArchNode<T> &data, QJsonObject &json)
{
    bool valid = data.isValidPointer();
    json[QLatin1String("valid")] = valid;
    if(valid) {
        const Node<T> *pointer = data.toPointer();
        if(pointer) {
            json[QLatin1String("ref")] = pointer->data.id.toString();
        } else {
            json[QLatin1String("ref")] = QUuid().toString();
        }
    } else {
        json[QLatin1String("archive")] = data.toString();
    }
}

static void save(const Transaction &t, QJsonObject &json)
{
    json[QLatin1String("date")] = t.date.toString(QStringLiteral("dd.MM.yyyy"));
    json[QLatin1String("note")] = t.note;
    json[QLatin1String("type")] = Transaction::Type::toConfigString(t.type);

    QJsonObject category;
    save(t.category, category);
    json[QLatin1String("category")] = category;
    json[QLatin1String("amount")] = t.amount.as_cents();
    QJsonObject from;
    save(t.from, from);
    QJsonObject to;
    save(t.to, to);

    if(t.type != Transaction::Type::In) {
        json[QLatin1String("from")] = from;
    }

    if(t.type != Transaction::Type::Out) {
        json[QLatin1String("to")] = to;
    }
}

static void save(const LogModel &data, QJsonArray &json)
{
    for(const Transaction &t : data.log) {
        QJsonObject obj;
        save(t, obj);
        json.append(obj);
    }
}

static void save(const Data &data, QJsonObject &json)
{
    QJsonArray owners;
    QJsonArray wallets;
    QJsonArray inCategories;
    QJsonArray outCategories;
    QJsonArray log;

    save(data.owners, owners);
    save(data.wallets, wallets);
    save(data.inCategories, inCategories);
    save(data.outCategories, outCategories);
    save(data.log, log);


    json[QLatin1String("owners")] = owners;
    json[QLatin1String("wallets")] = wallets;
    json[QLatin1String("inCategories")] = inCategories;
    json[QLatin1String("outCategories")] = outCategories;
    json[QLatin1String("log")] = log;

    json[QLatin1String("unanchored")] = data.log.unanchored;
}

static QByteArray save(const Data &data)
{
    QJsonObject json;
    save(data, json);

    QJsonDocument saveDoc(json);
    return saveDoc.toJson();
}

void save(const Data &data, const QString &fileName)
{
    QFile f(fileName);
    f.open(QIODevice::WriteOnly);
    f.write(save(data));
    f.close();
}

//
// Load
//
static void load(IdableString &data, QJsonObject json)
{
    QString id = json[QLatin1String("id")].toString();
    data.id = QUuid(id);
    as<QString &>(data) = json[QLatin1String("str")].toString();
}

static void load(OwnersModel &data, QJsonArray arr)
{
    for(QJsonValue v : std::as_const(arr)) {
        QJsonObject obj = v.toObject();
        Owner owner;
        load(owner, obj);
        data.owners.push_back(owner);
    }
}

static void load(CategoriesModel &data, QJsonArray arr)
{
    Node<Category> *currNode = data.rootItem;
    std::stack<int> children;

    for(QJsonValue v : std::as_const(arr)) {
        QJsonObject nodeObj = v.toObject();
        QJsonObject categoryObj = nodeObj[QLatin1String("category")].toObject();
        currNode->data.regular = nodeObj[QLatin1String("regular")].toBool();
        load(currNode->data, categoryObj);
        int ch = nodeObj[QLatin1String("children")].toInt();
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

static void load(ArchPointer<Owner> &data, QJsonObject json, const OwnersModel &ownersModel)
{
    bool valid = json[QLatin1String("valid")].toBool();

    if(valid) {
        QString id = json[QLatin1String("ref")].toString();
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
        data = json[QLatin1String("archive")].toString();
    }
}

static void load(Wallet &wallet, QJsonObject json, const OwnersModel &ownersModel)
{
    QString id = json[QLatin1String("id")].toString();
    wallet.id = QUuid(id);
    wallet.type = Wallet::Type::fromConfigString( json[QLatin1String("type")].toString() );
    wallet.name = json[QLatin1String("name")].toString();
    wallet.canBeNegative = json[QLatin1String("canBeNegative")].toBool();
    wallet.amount = Money(as<intmax_t>(json[QLatin1String("amount")].toInt()));

    QJsonObject ownerObj = json[QLatin1String("owner")].toObject();
    load(wallet.owner, ownerObj, ownersModel);
}

static void load(WalletsModel &data, QJsonArray arr, const OwnersModel &ownersModel)
{
    Node<Wallet> *currNode = data.rootItem;
    std::stack<int> children;

    for(QJsonValue v : std::as_const(arr)) {
        QJsonObject nodeObj = v.toObject();
        QJsonObject walletObj = nodeObj[QLatin1String("wallet")].toObject();
        load(currNode->data, walletObj, ownersModel);
        int ch = nodeObj[QLatin1String("children")].toInt();
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
static void load(ArchNode<T> &data, QJsonObject json, const Model &refModel)
{
    bool valid = json[QLatin1String("valid")].toBool();

    if(valid) {
        QString id = json[QLatin1String("ref")].toString();
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
        data = json[QLatin1String("archive")].toString();
    }


    if(valid) {
        const Node<T> *pointer = data.toPointer();
        if(pointer) {
            json[QLatin1String("ref")] = pointer->data.id.toString();
        } else {
            json[QLatin1String("ref")] = QUuid().toString();
        }
    } else {
        json[QLatin1String("archive")] = data.toString();
    }
}

static void load(Transaction &t, QJsonObject json,
                 const WalletsModel &wallets,
                 const CategoriesModel &inCategories,
                 const CategoriesModel &outCategories
) {
    t.date = QDate::fromString(json[QLatin1String("date")].toString(), "dd.MM.yyyy");
    t.note = json[QLatin1String("note")].toString();
    t.type = Transaction::Type::fromConfigString( json[QLatin1String("type")].toString() );
    t.amount = Money(as<intmax_t>(json[QLatin1String("amount")].toInt()));

    QJsonObject categoryObj = json[QLatin1String("category")].toObject();
    ArchNode<Category> category;
    if(t.type == Transaction::Type::In) {
        load(category, categoryObj, inCategories);
    } else if(t.type == Transaction::Type::Out) {
        load(category, categoryObj, outCategories);
    }
    t.category = category;

    if(t.type != Transaction::Type::In) {
        QJsonObject from = json[QLatin1String("from")].toObject();
        load(t.from, from, wallets);
    }

    if(t.type != Transaction::Type::Out) {
        QJsonObject to = json[QLatin1String("to")].toObject();
        load(t.to, to, wallets);
    }
}

static void load(LogModel &logModel, QJsonArray json, Data &data){
    int n = json.size();
    for (int i = n-1; i>=0; --i) {
        QJsonObject tObj = json[i].toObject();
        Transaction t;
        load(t, tObj, data.wallets, data.inCategories, data.outCategories);
        logModel.log.push_front(t);
    }
}

static void load(Data &data, QJsonObject json)
{
    load(data.owners, json[QLatin1String("owners")].toArray());
    load(data.wallets, json[QLatin1String("wallets")].toArray(), data.owners);
    load(data.inCategories, json[QLatin1String("inCategories")].toArray());
    load(data.outCategories, json[QLatin1String("outCategories")].toArray());
    load(data.log, json[QLatin1String("log")].toArray(), data);

    data.log.unanchored = json[QLatin1String("unanchored")].toInt();

    QDate monthBegin(today.year(), today.month(), 1);

    data.loadCategoriesStatistics(monthBegin, today);
}

void load(Data &data, const QString &fileName)
{
    data.clear();

    QFile f(fileName);
    f.open(QIODevice::ReadOnly);
    QByteArray bytes = f.readAll();
    f.close();

    QJsonDocument loadDoc(QJsonDocument::fromJson(bytes));
    QJsonObject json = loadDoc.object();
    load(data, json);
}

} // namespace cashbook
