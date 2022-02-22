#include "categoriesstaticchart.h"
#include "ui_categoriesstaticchart.h"

namespace cashbook
{

constexpr size_t PiePiecesCount {5};
static const Money Nothing{0.0f};

CategoriesStaticChart::PieChart::PieChart(QWidget* parent)
    : chart(new QChart())
    , series(new QPieSeries(parent))
    , view(new QChartView(chart, parent))
{}

void CategoriesStaticChart::PieChart::initUi()
{
    series->setPieSize(0.6);
    series->setHoleSize(0.26);
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTheme(QChart::ChartThemeBrownSand);
    chart->setBackgroundBrush(QBrush(Qt::white));

    view->setRenderHint(QPainter::Antialiasing, true);
}

static inline size_t getOffset(size_t frame)
{
    return (frame)*PiePiecesCount;
}

static inline Money getSum(const std::vector<std::pair<Category, Money>>& data, size_t startIndex = 0, size_t endIndex = std::numeric_limits<size_t>::max())
{
    auto begIt = std::next(data.begin(), startIndex);
    auto endIt = (endIndex == std::numeric_limits<size_t>::max())
        ? data.end()
        : (endIndex < data.size()) ? std::next(data.begin(), endIndex + 1) : data.end();

    return std::accumulate(begIt, endIt, Nothing, [](Money sum, const std::pair<Category, Money>& el){
        return sum + el.second;
    });
}

void CategoriesStaticChart::PieChart::setCategoryData(const Node<Category>* category, const CategoryMoneyMap& categoriesMap)
{
    parentCategory = category;
    data.clear();

    if(!parentCategory) {
        series->clear();
        return;
    }

    data.reserve(parentCategory->children.size());

    for(Node<Category>* childCategory : parentCategory->children) {
        if(categoriesMap.count(childCategory) == 0) {
            continue;
        }

        const Money& money = categoriesMap.at(childCategory);
        data.emplace_back(childCategory->data, money);
    }

    std::sort(data.begin(), data.end(), [](const std::pair<Category, Money>& a, const std::pair<Category, Money>& b)
    {
        return a.second > b.second;
    });

    frame = 0;
    framesCount = ((data.size() - 1) / PiePiecesCount) + 1;

    sums.resize(framesCount);

    for(size_t f = 0; f < framesCount; ++f) {
        sums[f] = getSum(data, getOffset(f), getOffset(f+1)-1);
    }

    //fillChart();
}

void CategoriesStaticChart::PieChart::increaseFrame()
{
    ++frame;
    fillChart();
}

void CategoriesStaticChart::PieChart::decreaseFrame()
{
    if(frame > 0) {
        --frame;
        fillChart();
    }
}

void CategoriesStaticChart::PieChart::fillChart()
{
    const bool hasOthers = frame < framesCount - 1;
    const size_t offset = getOffset(frame);
    const size_t othersOffset = hasOthers ? getOffset(frame + 1) : 0;

    const Money othersSum = hasOthers
        ? getSum(data, othersOffset)
        : Money();

    const Money allSum = sums[frame] + othersSum;

    const size_t endIndex = hasOthers ? othersOffset : data.size();

    series->clear();

    for(size_t i = offset; i<endIndex; ++i) {
        const auto& p = data[i];
        const Category& category = p.first;
        const Money& money = p.second;

        const double amount = static_cast<double>(money);
        const double percent = amount/static_cast<double>(allSum)*100.0f;

        QPieSlice* slice = series->append(
            QString("%1 %2%<br>%3").arg(category, formatPercent(percent), formatMoney(money)),
            amount
        );
        Q_UNUSED(slice);
    }

    if(hasOthers) {
        const double amount = static_cast<double>(othersSum);
        const double percent = amount/static_cast<double>(allSum)*100.0f;

        QPieSlice* slice = series->append(
            QString("%1 %2%<br>%3").arg(QObject::tr("Другое"), formatPercent(percent), formatMoney(othersSum)),
            amount
        );

        slice->setExploded(true);
        slice->setExplodeDistanceFactor(0.04f);
    }

    chart->setTitleFont(QFont("Segoe UI", 12));
    chart->setTitle(QObject::tr("%1:<br>%2").arg(
        (parentCategory->parent ? QObject::tr("Всего") : QString(parentCategory->data)),
        formatMoney(allSum)
    ));

    series->setLabelsVisible(true);
    series->setLabelsPosition(QPieSlice::LabelOutside);
}

CategoriesStaticChart::CategoriesStaticChart(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CategoriesStaticChart)
    , m_leftChart(this)
    , m_rightChart(this)

{
    ui->setupUi(this);

    connect(m_leftChart.series, &QPieSeries::clicked, this, &CategoriesStaticChart::onLeftPieClicked);
    connect(m_rightChart.series, &QPieSeries::clicked, this, &CategoriesStaticChart::onRightPieClicked);
}

void CategoriesStaticChart::init(const Data& data)
{
    m_data = &data;

    ui->dateFrom->setDate(MonthBegin.addDays(-120));
    ui->dateTo->setDate(Today);

    connect(ui->dateFrom, &QDateEdit::dateChanged, this, &CategoriesStaticChart::updatePeriod);
    connect(ui->dateTo, &QDateEdit::dateChanged, this, &CategoriesStaticChart::updatePeriod);

    m_leftChart.initUi();
    m_rightChart.initUi();

    ui->chartsLayout->addWidget(m_leftChart.view);
    ui->chartsLayout->addWidget(m_rightChart.view);

    updatePeriod();
}

CategoriesStaticChart::~CategoriesStaticChart()
{
    delete ui;
}

void CategoriesStaticChart::updatePeriod()
{
    m_inCategoriesMap.clear();
    m_outCategoriesMap.clear();

    m_leftChart.series->clear();
    m_rightChart.series->clear();

    if(m_data->log.log.empty()) {
        return;
    }

    const QDate from = ui->dateFrom->date();
    const QDate to = ui->dateTo->date();

    const std::deque<Transaction>& log = m_data->log.log;

    const QDate &logBegin = log.at(log.size()-1).date;
    const QDate &logEnd = log.at(0).date;

    if(to < logBegin || from > logEnd) {
        return;
    }

    size_t i = 0;
    while(i < log.size()) {
        const Transaction &t = log[i++];
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
                m_outCategoriesMap.propagateMoney(node, t.amount);
            }
        }

        if(t.type == Transaction::Type::In) {
            const ArchNode<Category> &archNode = t.category;
            if(archNode.isValidPointer()) {
                const Node<Category> *node = archNode.toPointer();
                m_inCategoriesMap.propagateMoney(node, t.amount);
            }
        }
    }

    m_leftChart.setCategoryData(m_data->outCategories.rootItem, m_outCategoriesMap);
    m_leftChart.fillChart();
}

void CategoriesStaticChart::onLeftPieClicked(QPieSlice* slice)
{
    const bool others = slice->isExploded();

    if(others) {
        m_rightChart.setCategoryData(m_leftChart.parentCategory, m_outCategoriesMap);
        m_rightChart.increaseFrame();
    } else {
        const size_t idx = m_leftChart.series->slices().indexOf(slice) + getOffset(m_leftChart.frame);
        const Category& category = m_leftChart.data[idx].first;

        const Node<Category>* selectedCategory = nullptr;
        for(const Node<Category>* c : m_leftChart.parentCategory->children) {
            if(c->data == category) {
                selectedCategory = c;
                break;
            }
        }

        if(!selectedCategory || selectedCategory->isLeaf()) {
            return;
        }

        m_rightChart.setCategoryData(selectedCategory, m_outCategoriesMap);
        m_rightChart.fillChart();
    }
}

void CategoriesStaticChart::onRightPieClicked(QPieSlice* slice)
{
    const bool others = slice->isExploded();

    if(others) {
        swapCharts();

        m_rightChart.setCategoryData(m_leftChart.parentCategory, m_outCategoriesMap);
        m_rightChart.frame = m_leftChart.frame;
        m_rightChart.increaseFrame();
    } else {
        const size_t idx = m_rightChart.series->slices().indexOf(slice) + getOffset(m_rightChart.frame);
        const Category& category = m_rightChart.data[idx].first;

        const Node<Category>* selectedCategory = nullptr;
        for(const Node<Category>* c : m_rightChart.parentCategory->children) {
            if(c->data == category) {
                selectedCategory = c;
                break;
            }
        }

        if(!selectedCategory || selectedCategory->isLeaf()) {
            return;
        }

        swapCharts();
        m_rightChart.setCategoryData(selectedCategory, m_outCategoriesMap);
        m_rightChart.fillChart();
    }
}

void CategoriesStaticChart::on_backButton_clicked()
{
    if(!m_leftChart.parentCategory || !m_rightChart.parentCategory) {
        return;
    }

    const bool leftFramed = m_leftChart.frame > 0;
    const bool rightFramed = m_rightChart.frame > 0;

    if(rightFramed) {
        swapCharts();
        m_leftChart.decreaseFrame();
        m_leftChart.decreaseFrame();
    } else {
        const bool leftIsRoot = !m_leftChart.parentCategory->parent;
        const bool rightExists = m_rightChart.parentCategory;

        if(leftIsRoot && !rightExists) {
            return;
        }

        const bool willBeRoot = leftIsRoot && rightExists;

        if(willBeRoot) {
            m_rightChart.setCategoryData(nullptr, m_outCategoriesMap);
        } else {
            swapCharts();
            m_leftChart.setCategoryData(m_rightChart.parentCategory->parent, m_outCategoriesMap);
            m_leftChart.fillChart();
        }
    }
}

void CategoriesStaticChart::swapCharts()
{
    disconnect(m_leftChart.series, &QPieSeries::clicked, this, &CategoriesStaticChart::onLeftPieClicked);
    disconnect(m_rightChart.series, &QPieSeries::clicked, this, &CategoriesStaticChart::onRightPieClicked);

    std::swap(m_leftChart, m_rightChart);
    ui->chartsLayout->removeWidget(m_leftChart.view);
    ui->chartsLayout->removeWidget(m_rightChart.view);
    ui->chartsLayout->addWidget(m_leftChart.view);
    ui->chartsLayout->addWidget(m_rightChart.view);

    connect(m_leftChart.series, &QPieSeries::clicked, this, &CategoriesStaticChart::onLeftPieClicked);
    connect(m_rightChart.series, &QPieSeries::clicked, this, &CategoriesStaticChart::onRightPieClicked);
}

} // namespace cashbook
