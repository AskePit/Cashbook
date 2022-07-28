#include "categoriesstaticchart.h"
#include "ui_categoriesstaticchart.h"

#include <QRectF>

namespace cashbook
{

TreemapModel::TreemapModel(QObject *parent)
    : QObject(parent)
{}

void TreemapModel::init(const Data& data)
{
    m_data = &data;
    m_from = MonthBegin.addDays(-60);
    m_to = Today;

    updatePeriod();
}

void TreemapModel::updatePeriod()
{
    m_inCategoriesMap.clear();
    m_outCategoriesMap.clear();

    if(m_data->log.log.empty()) {
        return;
    }

    const std::deque<Transaction>& log = m_data->log.log;

    const QDate &logBegin = log.at(log.size()-1).date;
    const QDate &logEnd = log.at(0).date;

    if(m_from.isNull()) {
        m_from = logBegin;
    }

    if(m_to.isNull()) {
        m_to = logEnd;
    }

    if(m_to < logBegin || m_from > logEnd) {
        return;
    }

    size_t i = 0;
    while(i < log.size()) {
        const Transaction &t = log[i++];
        if(t.date > m_to) {
            continue;
        }

        if(t.date < m_from) {
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

    //m_leftChart.setCategoryData(m_data->outCategories.rootItem, m_outCategoriesMap);
    emit onUpdated();
}

void TreemapModel::gotoNode(const QString& nodeName)
{
    if(!m_parentCategory) {
        return;
    }

    for(const Node<Category>* child : m_parentCategory->children) {
        if(child->data == nodeName) {
            m_parentCategory = child;
            return;
        }
    }
}

void TreemapModel::goUp()
{
    if(m_parentCategory) {
        m_parentCategory = m_parentCategory->parent;
    }
}

std::vector<RectData> TreemapModel::getCurrentValues()
{
    std::vector<RectData> res;

    if(!m_parentCategory) {
        return res;
    }

    res.reserve(m_parentCategory->children.size());

    const double sum = static_cast<double>(m_outCategoriesMap[m_parentCategory]);

    if(sum <= 0) {
        return res;
    }

    for(const Node<Category>* child : m_parentCategory->children) {
        const qreal percent = static_cast<double>(static_cast<double>(m_outCategoriesMap[child])) / static_cast<qreal>(sum);
        res.emplace_back(child->data, percent);
    }

    std::sort(res.begin(), res.end(), [](const RectData& a, const RectData& b) {
        return a.second > b.second;
    });

    return res;
}

std::vector<Rect> TreemapModel::getCurrenRects(float windowWidth, float windowHeight)
{
    std::vector<RectData> values = getCurrentValues();
    std::vector<Rect> res(values.size());

    std::transform(values.begin(), values.end(), res.begin(), [](const RectData& data){
        return Rect(data, 0, 0);
    });

    QRectF wholeSpace(0, 0, windowWidth, windowHeight);

    _getCurrenRects(res, wholeSpace, windowWidth*windowHeight);

    return res;
}

void TreemapModel::_getCurrenRects(std::span<Rect> res, QRectF space, qreal wholeSquare)
{
    if(res.empty()) {
        return;
    }

    if(res.size() == 1) {
        Rect& rect = res.front();
        rect.x = space.x();
        rect.y = space.y();
        rect.w = space.width();
        rect.h = space.height();

        return;
    }

    size_t spanTailIndex = 0;
    const bool isHeightFill = space.width() >= space.height();
    const qreal fillSideLength = isHeightFill ? space.height() : space.width();

    std::vector<qreal> ratios(res.size());
    constexpr qreal BigRatio = 10000.0f;
    std::fill(ratios.begin(), ratios.end(), BigRatio);

    while(spanTailIndex < res.size()) {
        if(spanTailIndex == 0) {
            Rect& rect = res.front();

            const qreal rectSideLength = fillSideLength;
            const qreal rectSquare = wholeSquare * rect.percentage;

            const qreal width  = isHeightFill ? (rectSquare / rectSideLength) : rectSideLength;
            const qreal height = isHeightFill ? rectSideLength : (rectSquare / rectSideLength);

            const qreal ratio = (width > height) ? (width / height) : (height / width);

            ratios.front() = ratio;

            rect.x = space.x();
            rect.y = space.y();
            rect.w = width;
            rect.h = height;
        } else {
            std::vector<Rect> rectWindow(spanTailIndex + 1);
            for(size_t i = 0; i<spanTailIndex + 1; ++i) {
                rectWindow[i] = res[i];
            }

            std::vector<qreal> rectWeigths(rectWindow.size());
            {
                const qreal percentageSum = std::accumulate(rectWindow.begin(), rectWindow.end(), 0.0f, [](qreal sum, const Rect& rect) {
                    return sum + rect.percentage;
                });


                std::transform(rectWindow.begin(), rectWindow.end(), rectWeigths.begin(), [&percentageSum](const Rect& rect){
                    return rect.percentage / percentageSum;
                });
            }

            qreal xOffset = 0;
            qreal yOffset = 0;

            for(size_t i = 0; i<rectWindow.size(); ++i) {
                Rect& rect = rectWindow[i];
                const qreal weight = rectWeigths[i];

                const qreal rectSideLength = fillSideLength * weight;
                const qreal rectSquare = wholeSquare * rect.percentage;

                const qreal width  = isHeightFill ? (rectSquare / rectSideLength) : rectSideLength;
                const qreal height = isHeightFill ? rectSideLength : (rectSquare / rectSideLength);

                const qreal ratio = (width > height) ? (width / height) : (height / width);
                if(ratio > ratios[i]) {
                    // break all now, go to next space sector
                    auto newHead = std::next(res.begin(), spanTailIndex);
                    size_t newSize = res.size() - spanTailIndex;

                    const Rect& refRect = *res.begin();

                    const qreal newSpaceX = space.x() + (isHeightFill ? refRect.w : 0);
                    const qreal newSpaceY = space.y() + (isHeightFill ? 0 : refRect.h);
                    const qreal newSpaceWidth = (space.width() + space.x()) - newSpaceX;
                    const qreal newSpaceHeight = (space.height() + space.y()) - newSpaceY;
                    const QRectF newSpace(newSpaceX, newSpaceY, newSpaceWidth, newSpaceHeight);

                    _getCurrenRects(std::span<Rect>(newHead, newSize), newSpace, wholeSquare);
                    return;
                } else {
                    ratios[i] = ratio;

                    rect.x = space.x() + xOffset;
                    rect.y = space.y() + yOffset;
                    rect.w = width;
                    rect.h = height;

                    (isHeightFill ? yOffset : xOffset) += rectSideLength;
                }
            }

            for(size_t i = 0; i<spanTailIndex + 1; ++i) {
                std::swap(res[i], rectWindow[i]);
            }
        }
        ++spanTailIndex;
    }
}

void TreemapModel::_setCategoryData(const Node<Category>* category, const CategoryMoneyMap& categoriesMap)
{
    m_parentCategory = category;
    m_currData.clear();

    if(!m_parentCategory) {
        return;
    }

    m_currData.reserve(m_parentCategory->children.size());

    for(Node<Category>* childCategory : m_parentCategory->children) {
        if(categoriesMap.count(childCategory) == 0) {
            continue;
        }

        const Money& money = categoriesMap.at(childCategory);
        m_currData.emplace_back(childCategory->data, money);
    }

    std::sort(m_currData.begin(), m_currData.end(), [](const std::pair<Category, Money>& a, const std::pair<Category, Money>& b)
    {
        return a.second > b.second;
    });
}

} // namespace cashbook
