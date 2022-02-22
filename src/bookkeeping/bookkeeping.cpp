#include "bookkeeping/bookkeeping.h"
#include <askelib_qt/std/fs.h>

namespace cashbook
{

QString Wallet::Type::toString(Type::t type) {
    switch(type) {
        case Count:
        case Common: return QObject::tr("Общий");
        case Cash: return QObject::tr("Наличные");
        case Card: return QObject::tr("Карта");
        case Account: return QObject::tr("Счет");
        case Deposit: return QObject::tr("Вклад");
        case Investment: return QObject::tr("Инвестиции");
        case CryptoCurrency: return QObject::tr("Криптовалюта");
        case Points: return QObject::tr("Баллы");
    }

    return QString();
}

QString Wallet::Type::toConfigString(Type::t type) {
    switch(type) {
        case Count:
        case Common: return QStringLiteral("Common");
        case Cash: return QStringLiteral("Cash");
        case Card: return QStringLiteral("Card");
        case Account: return QStringLiteral("Account");
        case Deposit: return QStringLiteral("Deposit");
        case Investment: return QObject::tr("Investment");
        case CryptoCurrency: return QStringLiteral("CryptoCurrency");
        case Points: return QObject::tr("Points");
    }

    return QString();
}

Wallet::Type::t Wallet::Type::fromConfigString(const QString &str) {
    if(str == "Common") {
        return Common;
    } else if(str == "Cash") {
        return Cash;
    } else if(str == "Card") {
        return Card;
    } else if(str == "Account") {
        return Account;
    } else if(str == "Deposit") {
        return Deposit;
    } else if(str == "Investment") {
        return Investment;
    } else if(str == "CryptoCurrency") {
        return CryptoCurrency;
    } else if(str == "Points") {
        return Points;
    } else {
        return Common;
    }
}

QString Wallet::Availability::toString(Availability::t type)
{
    switch(type) {
        case Count:
        case Free: return QObject::tr("Свободные");
        case InAMonth: return QObject::tr("В течении месяца");
        case InAQarter: return QObject::tr("В течении сезона");
        case InAYear: return QObject::tr("Год");
        case In3Years: return QObject::tr("3 года");
        case In5Years: return QObject::tr("5 лет");
        case In10Years: return QObject::tr("10 лет");
        case In20Years: return QObject::tr("20 лет");
        case AfterRetirement: return QObject::tr("После пенсии");
        case ForFutureGenerations: return QObject::tr("Будущим поколениям");
    }

    return QString();
}

QString Wallet::Availability::toConfigString(Availability::t type)
{
    switch(type) {
        case Count:
        case Free: return QStringLiteral("Free");
        case InAMonth: return QStringLiteral("InAMonth");
        case InAQarter: return QStringLiteral("InAQarter");
        case InAYear: return QStringLiteral("InAYear");
        case In3Years: return QStringLiteral("In3Years");
        case In5Years: return QStringLiteral("In5Years");
        case In10Years: return QStringLiteral("In10Years");
        case In20Years: return QStringLiteral("In20Years");
        case AfterRetirement: return QStringLiteral("AfterRetirement");
        case ForFutureGenerations: return QStringLiteral("ForFutureGenerations");
    }

    return QString();
}

Wallet::Availability::t Wallet::Availability::fromConfigString(const QString &str)
{
    if(str == "Free") {
        return Free;
    } else if(str == "InAMonth") {
        return InAMonth;
    } else if(str == "InAQarter") {
        return InAQarter;
    } else if(str == "InAYear") {
        return InAYear;
    } else if(str == "In3Years") {
        return In3Years;
    } else if(str == "In5Years") {
        return In5Years;
    } else if(str == "In10Years") {
        return In10Years;
    } else if(str == "In20Years") {
        return In20Years;
    } else if(str == "AfterRetirement") {
        return AfterRetirement;
    } else if(str == "ForFutureGenerations") {
        return ForFutureGenerations;
    } else {
        return Free;
    }
}

QString Wallet::InvestmentInfo::Type::toString(Type::t type)
{
    switch(type) {
        case Count:
        case Common: return QObject::tr("Общий");
        case Currency: return QObject::tr("Валюта");
        case Stocks: return QObject::tr("Акции");
        case Metals: return QObject::tr("Металлы");
        case CryptoCurrency: return QObject::tr("Криптовалюта");
        case RealEstate: return QObject::tr("Недвжимость");
    }

    return QString();
}

QString Wallet::InvestmentInfo::Type::toConfigString(Type::t type)
{
    switch(type) {
        case Count:
        case Common: return QStringLiteral("Common");
        case Currency: return QStringLiteral("Currency");
        case Stocks: return QStringLiteral("Stocks");
        case Metals: return QStringLiteral("Metals");
        case CryptoCurrency: return QStringLiteral("CryptoCurrency");
        case RealEstate: return QStringLiteral("RealEstate");
    }

    return QString();
}

Wallet::InvestmentInfo::Type::t Wallet::InvestmentInfo::Type::fromConfigString(const QString &str)
{
    if(str == "Common") {
        return Common;
    } else if(str == "Currency") {
        return Currency;
    } else if(str == "Stocks") {
        return Stocks;
    } else if(str == "Metals") {
        return Metals;
    } else if(str == "CryptoCurrency") {
        return CryptoCurrency;
    } else if(str == "RealEstate") {
        return RealEstate;
    } else {
        return Common;
    }
}

Wallet::Wallet()
{}

Wallet::Wallet(const QString &n)
    : name(n)
{}

Wallet::Wallet(const QString &n, Money a)
    : name(n)
    , amount(a)
{}

bool Wallet::operator==(const Wallet &other) const
{
    return name == other.name
        && id == other.id
        && amount == other.amount
        && type == other.type;
}

QString Transaction::Type::toString(Type::t type) {
    switch(type) {
        case In: return QObject::tr("Доход");
        default:
        case Out: return QObject::tr("Трата");
        case Transfer: return QObject::tr("Перевод");
    }
}

QString Transaction::Type::toConfigString(Type::t type) {
    switch(type) {
        case In: return QStringLiteral("In");
        default:
        case Out: return QStringLiteral("Out");
        case Transfer: return QStringLiteral("Transfer");
    }
}

Transaction::Type::t Transaction::Type::fromConfigString(const QString &str) {
    if(str == "In") {
        return In;
    } else if(str == "Out") {
        return Out;
    } else if(str == "Transfer") {
        return Transfer;
    } else {
        return Out;
    }
}

template <>
QString extractPathString<Category>(const Node<Category> *node) {
    return node ? node->data : "";
}

template <>
QString extractPathString<Wallet>(const Node<Wallet> *node) {
    return node ? node->data.name : "";
}

void CategoryMoneyMap::propagateMoney(const Node<Category> *node, const Money &amount) {
    while(node) {
        (*this)[node] += amount;
        node = node->parent;
    }
}

void LogData::insertRow(int position)
{
    Transaction t;
    t.date = Today;
    log.insert(std::next(log.begin(), position), 1, t);
    ++unanchored;
    changedMonths.insert(Month(t.date));
}

void LogData::insertRows(int position, size_t rows)
{
    Transaction t;
    t.date = Today;
    log.insert(std::next(log.begin(), position), static_cast<size_t>(rows), t);
    unanchored += static_cast<int>(rows);
    changedMonths.insert(Month(t.date));
    setChanged();
}

template <class T>
static bool isErrorNode(const ArchNode<T> &node)
{
    if(node.isValidPointer()) {
        if(node.toPointer() == nullptr) {
            return true;
        }
    } else {
        if(node.toString().isEmpty()) {
            return true;
        }
    }

    return false;
}

bool LogData::canAnchore() const
{
    for(int i = 0; i<unanchored; ++i) {
        const Transaction &t = log[static_cast<size_t>(i)];

        if(t.type == Transaction::Type::In || t.type == Transaction::Type::Out) {
           if(isErrorNode(t.category)) {
               return false;
           }
        }

        if(t.type != Transaction::Type::Out) {
            if(isErrorNode(t.to)) {
                return false;
            }
        }

        if(t.type != Transaction::Type::In) {
            if(isErrorNode(t.from)) {
                return false;
            }
        }
    }

    return true;
}

bool LogData::anchoreTransactions()
{
    if(!unanchored) {
        return false;
    }

    for(int i = unanchored-1; i>=0; --i) {
        Transaction &t = log[static_cast<size_t>(i)];
        if(t.note.startsWith('*')) {
            t.note.clear();
        }

        if(t.type != Transaction::Type::In && t.from.isValidPointer()) {
            Node<Wallet> *w = const_cast<Node<Wallet>*>(t.from.toPointer());
            if(w) {
                w->data.amount -= t.amount;
            }
            if(t.type == Transaction::Type::Out) {
                Month month(t.date);
                statistics.brief[month].common.spent += t.amount;

                const auto &archNode = t.category;
                if(archNode.isValidPointer()) {
                    const Node<Category> *category = archNode.toPointer();
                    if(category && category->data.regular) {
                        statistics.brief[month].regular.spent += t.amount;
                    }

                    statistics.outCategories.propagateMoney(category, t.amount);
                }
            }
        }

        if(t.type != Transaction::Type::Out && t.to.isValidPointer()) {
            Node<Wallet> *w = const_cast<Node<Wallet>*>(t.to.toPointer());
            if(w) {
                w->data.amount += t.amount;
            }
            if(t.type == Transaction::Type::In) {
                Month month(t.date);
                statistics.brief[month].common.received += t.amount;

                const auto &archNode = t.category;
                if(archNode.isValidPointer()) {
                    const Node<Category> *category = archNode.toPointer();
                    if(category && category->data.regular) {
                        statistics.brief[month].regular.received += t.amount;
                    }

                    statistics.inCategories.propagateMoney(category, t.amount);
                }
            }
        }
    }

    setChanged();

    unanchored = 0;
    return true;
}

void LogData::appendTransactions(const std::vector<Transaction> &transactions)
{
    if(transactions.empty()) {
        return;
    }
    QDate date = transactions.front().date;
    changedMonths.insert(Month(date));
    setChanged();

    for(const auto &t : transactions) {
        log.push_front(t);
    }

    unanchored += static_cast<int>(transactions.size());
}

void LogData::updateNote(size_t row, const QString &note)
{
    Transaction &t = log[row];
    t.note = note;

    changedMonths.insert(Month(t.date));
    setChanged();
}

static bool isNodeBelongsTo(const Node<Category> *node, const Node<Category> *parent) {
    while(node) {
        if(node == parent) {
            return true;
        }
        node = node->parent;
    }

    return false;
}

void LogData::updateTask(Task &task) const
{
    task.spent = 0;
    task.rest = 0;

    if(log.empty()) {
        task.rest = task.amount;
        return;
    }

    const QDate &logBegin = log.at(log.size()-1).date;
    const QDate &logEnd = log.at(0).date;

    if(task.to < logBegin || task.from > logEnd) {
        task.rest = task.amount;
        return;
    }

    size_t i = 0;
    while(i < log.size()) {
        const Transaction &t = log[i++];

        if(task.type != t.type) {
            continue;
        }

        if(t.date > task.to) {
            continue;
        }

        if(t.date < task.from) {
            break;
        }

        const ArchNode<Category> &trArchNode = t.category;
        const ArchNode<Category> &taskArchNode = task.category;
        if(trArchNode.isValidPointer() && taskArchNode.isValidPointer()) {
            const Node<Category> *trNode = trArchNode.toPointer();
            const Node<Category> *taskNode = taskArchNode.toPointer();
            if(isNodeBelongsTo(trNode, taskNode)) {
                task.spent += t.amount;
            }
        }
    }

    task.rest = task.amount - task.spent;
}

struct Balance
{
    int sum {0}; // wallet's balance
    uint32_t padding;
    std::vector<ArchNode<Wallet>> ins; // pointers to wallets that received money from this wallet
};

bool LogData::normalizeData()
{
    if(log.empty()) {
        return false;
    }

    QDate date = log.front().date;
    if(date.isNull()) {
        return false;
    }

    std::deque<Transaction> res; // our result
    std::vector<std::reference_wrapper<const Transaction>> day; // temp container for every day

    const auto processDay = [this, &day, &date, &res]() {
        std::vector<Transaction> normalizedDay;
        normalizedDay.reserve(day.size());

        // 1. Merge same Categories for same Transaction Type that has same Note (or have no any)
        for(size_t i = 0; i<day.size(); ++i) {
            Transaction iRec = day[i];

            for(size_t j = i+1; j<day.size(); ++j) {
                const Transaction& jRec = day[j];

                bool merge = iRec.type == jRec.type
                          && iRec.category == jRec.category
                          && iRec.from == jRec.from
                          && iRec.to == jRec.to
                          && iRec.note == jRec.note;

                if(merge) {
                    qDebug() << QString("Merge: %1, \'%2\' %3 merges with %4").arg(
                        iRec.date.toString(),
                        (iRec.category.toPointer() ? iRec.category.toPointer()->data : "TRANSFER"),
                        formatMoney(iRec.amount, false),
                        formatMoney(jRec.amount, false)
                    ).toStdString().c_str();

                    iRec.amount += jRec.amount;
                    std::swap(*std::next(day.begin(), static_cast<ptrdiff_t>(j)), day.back());
                    day.pop_back();
                    changedMonths.insert(Month(iRec.date));
                    setChanged();
                    --j;
                }
            }

            normalizedDay.emplace_back(std::move(iRec));
        }

        // 2. Merge Transer Transactions like: from A->B->C to A->C for same amounts of money
        {
            // collect all transfer transactions for a day
            std::vector<std::reference_wrapper<const Transaction>> transfers;
            for(const auto& t : normalizedDay) {
                if(t.type == Transaction::Type::Transfer) {
                    transfers.push_back(t);
                }
            }

            // calculate wallets' balances for all transactions
            std::unordered_map<ArchNode<Wallet>, Balance> balances;

            for(const Transaction& t : transfers) {
                balances[t.from].sum -= t.amount.as_cents();
                balances[t.to].sum += t.amount.as_cents();
                balances[t.from].ins.push_back(t.to);
            }

            // container for optimized transfer trnsactions
            std::vector<Transaction> newTransfers;

            // if a pair of wallets refers to only one transaction, remove it from balances and just move to a result container unchanged
            {
                auto it = transfers.begin();
                while(it != transfers.end()) {
                    const Transaction& t = *it;
                    if(balances[t.from].sum + balances[t.to].sum == 0) {
                        newTransfers.push_back(t);
                        balances.erase(t.from);
                        balances.erase(t.to);

                        it = transfers.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            // if remained transfers have a picture of graph with paths no longer than 1, discard it static_cast nonoptimizable
            if(!balances.empty())
            {
                // we avoided recursive check because it's enough to check 2nd nesting level to deside that graph needs normalization
                bool allShortPaths = true;
                for(const auto& p : balances) {
                    const Balance& balance = p.second;
                    for(const ArchNode<Wallet>& in : balance.ins) {
                        if(!balances[in].ins.empty()) {
                            allShortPaths = false;
                            break;
                        }
                    }
                }

                if(allShortPaths) {
                    goto skipTransferNormalization;
                }
            }

            // remove all zeros in balance
            auto it = balances.begin();
            while(it != balances.end()) {
                if(it->second.sum == 0) {
                    it = balances.erase(it);
                } else {
                    ++it;
                }
            }

            // no balances - no optimization
            if(balances.empty()) {
                goto skipTransferNormalization; // one of rare places, where it is reasonable, i think
            }

            changedMonths.insert(Month(date));
            setChanged();

            // spread balance records between outs and ins depending on sign of wallets' balance
            std::vector<std::pair<ArchNode<Wallet>, int>> outs;
            std::vector<std::pair<ArchNode<Wallet>, int>> ins;

            for(const auto& acc : balances) {
                if(acc.second.sum > 0) {
                    ins.emplace_back(std::make_pair(acc.first, acc.second.sum));
                } else {
                    outs.emplace_back(std::make_pair(acc.first, -acc.second.sum));
                }
            }

            // sort ins and outs
            const auto less = [](const std::pair<ArchNode<Wallet>, int>& p1, const std::pair<ArchNode<Wallet>, int>& p2) -> bool {
                return p1.second < p2.second;
            };

            std::sort(outs.begin(), outs.end(), less);
            std::sort(ins.begin(), ins.end(), less);

            // resolve transport task
            std::vector<std::vector<std::optional<int>>> tt(outs.size());
            for(auto& vec : tt) {
                vec.resize(ins.size());
            }

            for(size_t o = 0; o<tt.size(); ++o) {
                auto& oRow = tt[o];
                for(size_t i = 0; i<oRow.size(); ++i) {
                    auto& cell = oRow[i];

                    if(cell) {
                        continue;
                    }

                    auto& out = outs[o].second;
                    auto& in = ins[i].second;

                    if(out < in) {
                        cell = out;
                        in -= out;
                        out = 0;
                        for(auto& c : oRow) {
                            if(!c) {
                                c = 0;
                            }
                        }
                    } else {
                        cell = in;
                        out -= in;
                        in = 0;
                        for(size_t n = 0; n<outs.size(); ++n) {
                            auto& c = tt[n][i];
                            if(!c) {
                                c = 0;
                            }
                        }
                    }
                }
            }

            // create new transfer transactions from result of transport task in `tt`
            for(size_t o = 0; o<tt.size(); ++o) {
                auto& oRow = tt[o];
                for(size_t i = 0; i<oRow.size(); ++i) {
                    auto& cell = oRow[i];

                    if(!cell || cell.value() == 0) {
                        continue;
                    }

                    auto& out = outs[o].first;
                    auto& in = ins[i].first;

                    Transaction t;
                    t.type = Transaction::Type::Transfer;
                    t.date = date;
                    t.from = out;
                    t.to = in;
                    t.amount = Money(static_cast<intmax_t>(cell.value()));

                    newTransfers.push_back(t);
                }
            }

            // remove all transfers from normalizedDay
            qDebug() << QString("Before transfer optimization for '%1' day:").arg(date.toString()).toStdString().c_str();
            {
                auto it = normalizedDay.begin();
                while(it != normalizedDay.end()) {
                    if(it->type == Transaction::Type::Transfer) {
                        qDebug()
                                << '\t'
                                << archNodeToShortString(it->from).toStdString().c_str()
                                << "->"
                                << archNodeToShortString(it->to).toStdString().c_str()
                                << formatMoney(it->amount, false).toStdString().c_str();
                        std::swap(*it, normalizedDay.back());
                        normalizedDay.pop_back();
                    } else {
                        ++it;
                    }
                }
            }

            // add new transfers to normalizedDay
            qDebug() << "After transfer optimization:";
            for(const auto& t : newTransfers) {
                qDebug()
                        << '\t'
                        << archNodeToShortString(t.from).toStdString().c_str()
                        << "->"
                        << archNodeToShortString(t.to).toStdString().c_str()
                        << formatMoney(t.amount, false).toStdString().c_str();
            }
            normalizedDay.insert(normalizedDay.end(), newTransfers.begin(), newTransfers.end());
            qDebug() << "";
        }

        skipTransferNormalization:

        // Add normalized day to res
        res.insert(res.end(), normalizedDay.begin(), normalizedDay.end());
    }; // end of processDay

    for(const auto &record : log)
    {
        if(record.date != date) {
            // process previous day
            processDay();
            date = record.date;
            day.clear();
        }

        day.push_back(record);
    }
    processDay(); // process last day

    bool changed = isChanged();
    if(changed) {
        std::swap(log, res);
    }

    return changed;
}

Data::Data()
    : inCategories(statistics.inCategories)
    , outCategories(statistics.outCategories)
    , log(statistics)
{
    setChangableItems({
        &owners,
        &wallets,
        &inCategories,
        &outCategories,
        &log,
        &plans,
        &tasks
    });

    connect(this, &Data::categoriesStatisticsUpdated, [this](){
        updateTasks();
    });
}

void Data::onOwnersRemove(QStringList paths)
{
    auto nodes = wallets.rootItem->toList();
    for(auto *node : nodes) {
        ArchPointer<Owner> &owner = node->data.info->owner;
        if(owner.isValidPointer()) {
            QString name = *owner.toPointer();
            if(paths.contains(name)) {
                owner = ArchiveString(name); // invalidate ArchPointer by assigning QString to it.
            }
        }
    }
}

template <class DataType>
static void invalidateArchNode(ArchNode<DataType> &archNode, const QStringList &paths)
{
    if(archNode.isValidPointer()) {
        QString path = pathToString(archNode.toPointer());
        if(paths.contains(path)) {
            archNode = ArchiveString(path); // invalidate ArchPointer by assigning QString to it.
        }
    }
}

void Data::onInCategoriesRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        if(t.type != Transaction::Type::In) {
            continue;
        }

        invalidateArchNode(t.category, paths);
    }
}

void Data::onOutCategoriesRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        if(t.type != Transaction::Type::Out) {
            continue;
        }

        invalidateArchNode(t.category, paths);
    }
}

void Data::onWalletsRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        invalidateArchNode(t.from, paths);
        invalidateArchNode(t.to, paths);
    }
}

void Data::loadCategoriesStatistics(const QDate &from, const QDate &to)
{
    statistics.outCategories.clear();
    statistics.inCategories.clear();

    statistics.categoriesFrom = from;
    statistics.categoriesTo = to;

    if(log.log.empty()) {
        return;
    }

    const QDate &logBegin = log.log.at(log.log.size()-1).date;
    const QDate &logEnd = log.log.at(0).date;

    if(to < logBegin || from > logEnd) {
        return;
    }

    size_t i = 0;
    while(i < log.log.size()) {
        const Transaction &t = log.log[i++];
        if(t.date > to) {
            continue;
        }

        if(t.date < from) {
            break;
        }

        if(t.type == Transaction::Type::Out) {
            const ArchNode<Category> &archNode = t.category;
            if(archNode.isValidPointer()) {
                const Node<Category> *node = archNode.toPointer();
                statistics.outCategories.propagateMoney(node, t.amount);
            }
        }

        if(t.type == Transaction::Type::In) {
            const ArchNode<Category> &archNode = t.category;
            if(archNode.isValidPointer()) {
                const Node<Category> *node = archNode.toPointer();
                statistics.inCategories.propagateMoney(node, t.amount);
            }
        }
    }

    emit categoriesStatisticsUpdated();
}

void Data::updateTasks()
{
    updateTasks(tasks.active);
    updateTasks(tasks.completed);
}

void Data::updateTasks(TasksListsData &tasksData)
{
    for(Task &task : tasksData.tasks) {
        log.updateTask(task);
    }
}

bool Data::anchoreTransactions()
{
    bool did = log.anchoreTransactions();
    if(did) {
        emit categoriesStatisticsUpdated();
    }

    return did;
}

Node<Wallet> *Data::walletFromPath(const QString &path) {
    return nodeFromPath<Wallet, WalletsData>(wallets, path);
}

Node<Category> *Data::inCategoryFromPath(const QString &path) {
    return nodeFromPath<Category, CategoriesData>(inCategories, path);
}

Node<Category> *Data::outCategoryFromPath(const QString &path) {
    return nodeFromPath<Category, CategoriesData>(outCategories, path);
}

void Data::clear()
{
    owners.owners.clear();

    if(wallets.rootItem) {
        delete wallets.rootItem;
    }
    wallets.rootItem = new Node<Wallet>;

    if(inCategories.rootItem) {
        delete inCategories.rootItem;
    }
    inCategories.rootItem = new Node<Category>;

    if(outCategories.rootItem) {
        delete outCategories.rootItem;
    }
    outCategories.rootItem = new Node<Category>;

    log.log.clear();
    plans.shortTerm.plans.clear();
    plans.middleTerm.plans.clear();
    plans.longTerm.plans.clear();
    tasks.active.tasks.clear();
    tasks.completed.tasks.clear();

    resetChanged();
}

void Data::importReceiptFile(const QString &json, const Node<Wallet> *wallet)
{
    QString data = aske::readFile(json);

    const auto findWord = [&data](const QLatin1String &startMarker, const QLatin1String &endMarker, qsizetype from) -> std::pair<QStringView, int> {
        from = data.indexOf(startMarker, from);
        if(from < 0) {
            return std::make_pair(QStringView(), from);
        }
        from += startMarker.size();
        qsizetype to = data.indexOf(endMarker, from);

        QStringView str(&data.data()[from], to-from);
        to += endMarker.size();
        return std::make_pair(str, to);
    };

    QStringView dateString = findWord(QLatin1String(R"("dateTime":)"), QLatin1String(R"(,")"), 0).first;
    QDate date = QDateTime::fromSecsSinceEpoch(dateString.toInt()).date();

    std::vector<Transaction> transactions;

    int i {0};
    while(1) {
        auto p = findWord(QLatin1String(R"("name":")"), QLatin1String(R"(",")"), i);
        if(p.second < 0) {
            break;
        }

        QString name = p.first.toString();
        i = p.second;

        p = findWord(QLatin1String(R"("sum":)"), QLatin1String(R"(,")"), i);
        QString sum = p.first.toString();
        i = p.second;

        Transaction t;
        t.note = "*" + name; // all notes which starts with '*' will be removed after anchoring
        t.amount = Money(static_cast<intmax_t>(sum.toInt()));
        t.date = date;
        t.from = wallet;
        transactions.emplace_back(std::move(t));
    }

    log.appendTransactions(transactions);
}

} // namespace cashbook
