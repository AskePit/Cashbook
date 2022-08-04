#include "serialization.h"
#include "bookkeeping/bookkeeping.h"

#include <askelib_qt/std/fs.h>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <utility>
#include <stack>
#include <fstream>

#include "qtyaml.h"
#include <yaml-cpp/yaml.h>

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

static void save(const IdableString &data, YAML::Node& node)
{
    node["id"] = data.id.toString();
    node["str"] = static_cast<QString>(data);
}

static void save(const OwnersData &data, YAML::Node& node)
{
    for(const auto &owner : data.owners) {
        YAML::Node obj;
        save(owner, obj);
        node.push_back(obj);
    }
}

static void save(const BanksData &data, YAML::Node& node)
{
    for(const auto &bank : data.banks) {
        YAML::Node obj;
        save(bank, obj);
        node.push_back(obj);
    }
}

static void save(const Node<Category> *node, YAML::Node& nodeNodes)
{
    YAML::Node nodeNode;

    YAML::Node category;
    save(node->data, category);
    nodeNode["category"] = category;
    nodeNode["regular"] = node->data.regular;

    int children = static_cast<int>(node->childCount());
    if(children) {
        nodeNode["children"] = children;
    }
    nodeNodes.push_back(nodeNode);

    for(const auto &child : node->children) {
        save(child, nodeNodes);
    }
}

static void save(const CategoriesData &data, YAML::Node& node)
{
    save(data.rootItem, node);
}

template <class T>
static void save(const ArchPointer<T> &data, YAML::Node& node)
{
    bool valid = data.isValidPointer();
    if(valid) {
        const Owner *pointer = data.toPointer();
        if(pointer) {
            node["ref"] = pointer->id.toString();
        } else {
            node["ref"] = QUuid().toString();
        }
    } else {
        node["archive"] = data.toString();
    }
}

static void save(const Wallet &wallet, YAML::Node& node)
{
    node["id"] = wallet.id.toString();
    node["type"] = Wallet::Type::toConfigString(wallet.type);
    node["name"] = wallet.name;
    node["amount"] = wallet.amount.as_cents();

    YAML::Node ownerJson;
    save(wallet.info->owner, ownerJson);
    node["owner"] = ownerJson;
    node["canBeNegative"] = wallet.info->canBeNegative;
    node["availability"] = Wallet::Availability::toConfigString(wallet.info->availability);

    switch(wallet.type)
    {
        default:
        case Wallet::Type::Common:
        case Wallet::Type::Cash:
        case Wallet::Type::CryptoCurrency:
        case Wallet::Type::Points:
            break;
        case Wallet::Type::Deposit:
        {
            const auto* info = static_cast<const Wallet::DepositInfo*>(wallet.info.get());
            node["incomePercent"] = info->incomePercent;
        }
        [[fallthrough]];
        case Wallet::Type::Account:
        case Wallet::Type::Card:
        {
            const auto* info = static_cast<const Wallet::AccountInfo*>(wallet.info.get());

            YAML::Node bankJson;
            save(info->bank, bankJson);
            node["bank"] = bankJson;
        }
        break;
        case Wallet::Type::Investment:
        {
            const auto* info = static_cast<const Wallet::InvestmentInfo*>(wallet.info.get());

            node["investmentType"] = Wallet::InvestmentInfo::Type::toConfigString(info->type);

            if(info->account) {
                YAML::Node bankJson;
                save(info->account->bank, bankJson);
                node["bank"] = bankJson;
            }
        }
        break;
    }
}

static void save(const Node<Wallet> *node, YAML::Node& nodeNodes)
{
    YAML::Node nodeNode;

    YAML::Node wallet;
    save(node->data, wallet);
    nodeNode["wallet"] = wallet;

    int children = static_cast<int>(node->childCount());
    if(children) {
        nodeNode["children"] = children;
    }

    nodeNodes.push_back(nodeNode);

    for(const Node<Wallet> *child : node->children) {
        save(child, nodeNodes);
    }
}

static void save(const WalletsData &data, YAML::Node& node)
{
    save(data.rootItem, node);
}

template <class T>
static void save(const ArchNode<T> &data, YAML::Node& node)
{
    bool valid = data.isValidPointer();
    if(valid) {
        const Node<T> *pointer = data.toPointer();
        if(pointer) {
            node["ref"] = pointer->data.id.toString();
        } else {
            node["ref"] = QUuid().toString();
        }
    } else {
        node["archive"] = data.toString();
    }
}

static void save(const Transaction &t, YAML::Node& node)
{
    node["date"] = t.date.toString(QStringLiteral("dd.MM.yyyy"));

    if(!t.note.isEmpty()) {
        node["note"] = t.note;
    }
    node["type"] = Transaction::Type::toConfigString(t.type);

    YAML::Node category;
    save(t.category, category);
    if(t.type != Transaction::Type::Transfer) {
        node["category"] = category;
    }
    node["amount"] = t.amount.as_cents();
    YAML::Node from;
    save(t.from, from);
    YAML::Node to;
    save(t.to, to);

    if(t.type != Transaction::Type::In) {
        node["from"] = from;
    }

    if(t.type != Transaction::Type::Out) {
        node["to"] = to;
    }
}

static void save(const Plan &item, YAML::Node& node)
{
    if(!item.name.isEmpty()) {
        node["name"] = item.name;
    }

    node["type"] = Transaction::Type::toConfigString(item.type);

    YAML::Node category;
    save(item.category, category);
    if(item.type != Transaction::Type::Transfer) {
        node["category"] = category;
    }
    node["amount"] = item.amount.as_cents();
}

static void save(const PlansTermData &data, YAML::Node& node)
{
    for(const Plan &t : data.plans) {
        YAML::Node obj;
        save(t, obj);
        node.push_back(obj);
    }
}

static void save(const PlansData &data, YAML::Node& node)
{
    YAML::Node shortPlans;
    YAML::Node middlePlans;
    YAML::Node longPlans;

    save(data.shortTerm, shortPlans);
    save(data.middleTerm, middlePlans);
    save(data.longTerm, longPlans);

    node["short"] = shortPlans;
    node["middle"] = middlePlans;
    node["long"] = longPlans;
}

static void save(const Task &item, YAML::Node& node)
{
    node["type"] = Transaction::Type::toConfigString(item.type);

    YAML::Node category;
    save(item.category, category);
    if(item.type != Transaction::Type::Transfer) {
        node["category"] = category;
    }
    node["from"] = item.from.toString(QStringLiteral("dd.MM.yyyy"));
    node["to"] = item.to.toString(QStringLiteral("dd.MM.yyyy"));
    node["amount"] = item.amount.as_cents();
}

static void save(const TasksData &data, YAML::Node& node)
{
    for(const auto& tasks : {data.active.tasks, data.completed.tasks}) {
        for(const Task &task : tasks) {
            YAML::Node obj;
            save(task, obj);
            node.push_back(obj);
        }
    }
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

    // file.node -> file.backup.1
    if(storage::backupCount >= 1) {
        aske::copyFileForced(fileName, backupFile(fileName, 1));
    }

    // 2. save file
    std::ofstream fout(fileName.toStdString());
    fout << data;
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
        YAML::Node monthArray;
        for(const Transaction &t : log) {
            Month tMonth {t.date};

            if(tMonth != month) {
                continue;
            }

            YAML::Node obj;
            save(t, obj);
            monthArray.push_back(obj);
        }

        saveFile(storage::monthFile(month.toDate()), monthArray);
    }

    changedMonths.clear();
}

static void saveHead(const Data &data, YAML::Node& node)
{
    YAML::Node owners;
    YAML::Node banks;
    YAML::Node wallets;
    YAML::Node inCategories;
    YAML::Node outCategories;
    YAML::Node plans;
    YAML::Node tasks;

    save(data.owners, owners);
    save(data.banks, banks);
    save(data.wallets, wallets);
    save(data.inCategories, inCategories);
    save(data.outCategories, outCategories);
    save(data.plans, plans);
    save(data.tasks, tasks);

    node["owners"] = owners;
    node["banks"] = banks;
    node["wallets"] = wallets;
    node["inCategories"] = inCategories;
    node["outCategories"] = outCategories;
    node["plans"] = plans;
    node["tasks"] = tasks;
    node["unanchored"] = data.log.unanchored;
}

static void saveHead(const Data &data)
{
    YAML::Node node;
    saveHead(data, node);

    saveFile(storage::headFile, node);
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
static void load(IdableString &data, const YAML::Node& node)
{
    QString id = node["id"].as<QString>();
    data.id = QUuid(id);
    static_cast<QString &>(data) = node["str"].as<QString>();
}

static void load(OwnersData &data, const YAML::Node& node)
{
    for(const YAML::Node& v : node) {
        Owner owner;
        load(owner, v);
        data.owners.push_back(owner);
    }
}

static void load(BanksData &data, const YAML::Node& node)
{
    for(const YAML::Node& v : node) {
        Bank bank;
        load(bank, v);
        data.banks.push_back(bank);
    }
}

static void load(CategoriesData &data, const YAML::Node& node)
{
    Node<Category> *currNode = data.rootItem;
    std::stack<int> children;

    for(const YAML::Node& v : node) {
        const YAML::Node& categoryObj = v["category"];
        currNode->data.regular = v["regular"].as<bool>();
        load(currNode->data, categoryObj);
        int ch = v["children"].as<int>(0);
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

template <class T, class Model>
static void load(ArchPointer<T> &data, const YAML::Node& node, const Model &model)
{
    const YAML::Node& refNode = node["ref"];

    if(refNode.IsDefined()){
        QString id = refNode.as<QString>();
        QUuid uid = QUuid{id};

        if(!uid.isNull()) {
            for(const T &el : model) {
                if(el.id == uid) {
                    data = &el;
                    return;
                }
            }
        } else {
            data = static_cast<const Owner*>(nullptr);
        }
    } else {
        data = ArchiveString(node["archive"].as<QString>());
    }
}

static void load(Wallet &wallet, const YAML::Node& node, const OwnersData &owners, const BanksData &banks)
{
    QString id = node["id"].as<QString>();
    wallet.id = QUuid(id);
    wallet.type = Wallet::Type::fromConfigString( node["type"].as<QString>() );
    wallet.name = node["name"].as<QString>();
    wallet.amount = Money(static_cast<intmax_t>(node["amount"].as<double>()));

    switch(wallet.type)
    {
        default:
        case Wallet::Type::Common:
            wallet.info = std::make_shared<Wallet::Info>();
            break;
        case Wallet::Type::Cash:
            wallet.info = std::make_shared<Wallet::CashInfo>();
            break;
        case Wallet::Type::Deposit:
        {
            auto info = std::make_shared<Wallet::DepositInfo>();

            info->incomePercent = node["incomePercent"].as<float>();

            const YAML::Node& bankObj = node["bank"];
            load(info->bank, bankObj, banks.banks);

            wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::Account:
        {
            auto info = std::make_shared<Wallet::AccountInfo>();
            const YAML::Node& bankObj = node["bank"];
            load(info->bank, bankObj, banks.banks);

            wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::Card:
        {
            auto info = std::make_shared<Wallet::CardInfo>();
            const YAML::Node& bankObj = node["bank"];
            load(info->bank, bankObj, banks.banks);

            wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::Investment:
        {
            auto info = std::make_shared<Wallet::InvestmentInfo>();
            info->type = Wallet::InvestmentInfo::Type::fromConfigString(node["investmentType"].as<QString>());

            const YAML::Node& investmentank = node["bank"];
            if(investmentank.IsDefined()) {
                info->account = Wallet::AccountInfo();
                load(info->account->bank, investmentank, banks.banks);
            }
            wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::CryptoCurrency:
            wallet.info = std::make_shared<Wallet::CryptoCurrencyInfo>();
            break;
        case Wallet::Type::Points:
            wallet.info = std::make_shared<Wallet::PointsInfo>();
            break;
    }

    const YAML::Node& ownerObj = node["owner"];
    load(wallet.info->owner, ownerObj, owners.owners);
    wallet.info->canBeNegative = node["canBeNegative"].as<bool>();

    const YAML::Node& availability = node["availability"];
    if(availability.IsDefined()) {
        wallet.info->availability = Wallet::Availability::fromConfigString(node["availability"].as<QString>());
    }
}

static void load(WalletsData &data, const YAML::Node& arr, const OwnersData &owners, const BanksData &banks)
{
    Node<Wallet> *currNode = data.rootItem;
    std::stack<int> children;

    for(const YAML::Node& v : arr) {
        const YAML::Node& walletObj = v["wallet"];
        load(currNode->data, walletObj, owners, banks);
        int ch = v["children"].as<int>(0);
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
}

template <class T, class Model>
static void load(ArchNode<T> &data, const YAML::Node& node, const Model &refModel)
{
    const YAML::Node& refNode = node["ref"];

    if(refNode.IsDefined()){
        QString id = refNode.as<QString>();
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
            data = static_cast<const Node<T>*>(nullptr);
        }
    } else {
        data = ArchiveString(node["archive"].as<QString>());
    }
}

static void load(Transaction &t, const YAML::Node& node,
                 const WalletsData &wallets,
                 const CategoriesData &inCategories,
                 const CategoriesData &outCategories
) {
    t.date = QDate::fromString(node["date"].as<QString>(), "dd.MM.yyyy");
    t.note = node["note"].as<QString>("");
    t.type = Transaction::Type::fromConfigString( node["type"].as<QString>() );
    t.amount = Money(static_cast<intmax_t>(node["amount"].as<int>()));

    if(t.type != Transaction::Type::Transfer) {
        const YAML::Node& categoryObj = node["category"];
        ArchNode<Category> category;
        if(t.type == Transaction::Type::In) {
            load(category, categoryObj, inCategories);
        } else if(t.type == Transaction::Type::Out) {
            load(category, categoryObj, outCategories);
        }
        t.category = category;
    }

    if(t.type != Transaction::Type::In) {
        const YAML::Node& from = node["from"];
        load(t.from, from, wallets);
    }

    if(t.type != Transaction::Type::Out) {
        const YAML::Node& to = node["to"];
        load(t.to, to, wallets);
    }
}

static void load(LogData &log, const YAML::Node& node, Data &data){
    for (const YAML::Node& tObj : node) {
        Transaction t;
        load(t, tObj, data.wallets, data.inCategories, data.outCategories);

        if(t.type == Transaction::Type::In || t.type == Transaction::Type::Out) {
            // update brief statistics

            Month month(t.date);
            const auto &archNode = t.category;
            const bool isRegular = archNode.isValidPointer() && archNode.toPointer() && archNode.toPointer()->data.regular;

            BriefStatisticsRecord& monthBrief = log.statistics.brief[month];

            Money& common = t.type == Transaction::Type::In ? monthBrief.common.received : monthBrief.common.spent;
            common += t.amount;
            if(isRegular) {
                Money& regular = t.type == Transaction::Type::In ? monthBrief.regular.received : monthBrief.regular.spent;
                regular += t.amount;
            }
        }

        log.log.emplace_back(std::move(t));
    }
}

static void load(Plan &item, const YAML::Node& node,
                 const CategoriesData &inCategories,
                 const CategoriesData &outCategories
) {
    item.name = node["name"].as<QString>();
    item.type = Transaction::Type::fromConfigString( node["type"].as<QString>() );

    if(item.type != Transaction::Type::Transfer) {
        const YAML::Node& categoryObj = node["category"];
        ArchNode<Category> category;
        if(item.type == Transaction::Type::In) {
            load(category, categoryObj, inCategories);
        } else if(item.type == Transaction::Type::Out) {
            load(category, categoryObj, outCategories);
        }
        item.category = category;
    }

    item.amount = Money(static_cast<intmax_t>(node["amount"].as<int>()));
}

static void load(PlansTermData &data, const YAML::Node& arr,
                 const CategoriesData &inCategories,
                 const CategoriesData &outCategories
)
{
    for(const YAML::Node& v : arr) {
        Plan item;
        load(item, v, inCategories, outCategories);
        data.plans.push_back(item);
    }
}

static void load(PlansData &data, const YAML::Node& node,
                 const CategoriesData &inCategories,
                 const CategoriesData &outCategories
)
{
    load(data.shortTerm, node["short"], inCategories, outCategories);
    load(data.middleTerm, node["middle"], inCategories, outCategories);
    load(data.longTerm, node["long"], inCategories, outCategories);
}

static void load(Task &item, const YAML::Node& node,
                 const CategoriesData &inCategories,
                 const CategoriesData &outCategories
) {
    item.type = Transaction::Type::fromConfigString( node["type"].as<QString>() );

    if(item.type != Transaction::Type::Transfer) {
        const YAML::Node& categoryObj = node["category"];
        ArchNode<Category> category;
        if(item.type == Transaction::Type::In) {
            load(category, categoryObj, inCategories);
        } else if(item.type == Transaction::Type::Out) {
            load(category, categoryObj, outCategories);
        }
        item.category = category;
    }

    item.from = QDate::fromString(node["from"].as<QString>(), "dd.MM.yyyy");
    item.to = QDate::fromString(node["to"].as<QString>(), "dd.MM.yyyy");

    item.amount = Money(static_cast<intmax_t>(node["amount"].as<int>()));
}

static void load(TasksData &data, const YAML::Node& arr,
                 const CategoriesData &inCategories,
                 const CategoriesData &outCategories
)
{
    for(const YAML::Node& v : arr) {
        Task item;
        load(item, v, inCategories, outCategories);
        if(item.to >= Today) {
            data.active.tasks.push_back(item);
        } else {
            data.completed.tasks.push_back(item);
        }
    }
}

static void loadHead(Data &data, const YAML::Node& node)
{
    load(data.owners, node["owners"]);
    load(data.banks, node["banks"]);
    load(data.wallets, node["wallets"], data.owners, data.banks);
    load(data.inCategories, node["inCategories"]);
    load(data.outCategories, node["outCategories"]);
    load(data.plans, node["plans"], data.inCategories, data.outCategories);
    load(data.tasks, node["tasks"], data.inCategories, data.outCategories);
    data.log.unanchored = node["unanchored"].as<int>();
}

static void loadMonth(Data &data, const YAML::Node& node)
{
    load(data.log, node, data);
}

static void loadLog(Data &data, int recentMonths = -1) /* (recentMonths == -1) means load all */
{    
    QDir dir(storage::rootDir, {QStringLiteral("*")+storage::ext});
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name | QDir::Reversed);

    QFileInfoList files = dir.entryInfoList();
    {
        QMutableListIterator<QFileInfo> it(files);
        while (it.hasNext()) {
            if (it.next().filePath() == storage::headFile) {
                it.remove();
            }
        }
    }

    if(recentMonths == -1) {
        recentMonths = INT_MAX;
    }

    const qsizetype n = files.size();
    qsizetype i = 0;

    while (i < n && i <= recentMonths) {
        const QFileInfo& file = files[static_cast<int>(i++)];

        auto filePath = file.filePath();

        YAML::Node monthDoc = YAML::LoadFile(filePath.toStdString());
        loadMonth(data, monthDoc);
    }

    data.updateTasks();
}

static void loadHead(Data &data)
{
    QFileInfo info(storage::headFile);
    if(!info.exists()) {
        return;
    }

    YAML::Node doc = YAML::LoadFile(storage::headFile.toStdString());
    loadHead(data, doc);
}

void load(Data &data)
{
    data.clear();

    constexpr int LoadMonths = -1;

    loadHead(data);
    loadLog(data, LoadMonths);
}

} // namespace cashbook
