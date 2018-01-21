#include "serialization.h"
#include "bookkeeping.h"
#include "types.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

namespace cashbook {

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
    json[QLatin1String("type")] = as<int>(wallet.type);
    json[QLatin1String("name")] = wallet.name;
    json[QLatin1String("canBeNegative")] = wallet.canBeNegative;
    json[QLatin1String("amount")] = as<double>(wallet.amount);

    QJsonArray ownersJson;
    for(const ArchPointer<Owner> &owner : wallet.owners) {
        QJsonObject ownerJson;
        save(owner, ownerJson);
        ownersJson.append(ownerJson);
    }

    json[QLatin1String("owners")] = ownersJson;
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
    json[QLatin1String("type")] = as<int>(t.type);

    QJsonArray categories;
    for(const ArchNode<Category> &c : t.category) {
        QJsonObject obj;
        save(c, obj);
        categories.append(obj);
    }

    json[QLatin1String("categories")] = categories;
    json[QLatin1String("amount")] = as<double>(t.amount);
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
}

QByteArray save(const Data &data)
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

} // namespace cashbook


