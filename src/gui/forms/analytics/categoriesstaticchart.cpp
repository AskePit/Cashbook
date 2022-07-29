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
    m_from = Today.addDays(-30);
    m_to = Today;

    m_parentCategory = m_data->outCategories.rootItem;

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

    emit onUpdated();
}

bool TreemapModel::gotoNode(const QString& nodeName)
{
    if(!m_parentCategory) {
        return false;
    }

    for(const Node<Category>* child : m_parentCategory->children) {
        if(child->data == nodeName) {
            if(!child->isLeaf()) {
                m_parentCategory = child;
                emit onUpdated();
                return true;
            }
            return false;
        }
    }

    return false;
}

bool TreemapModel::goUp()
{
    if(m_parentCategory && m_parentCategory->parent) {
        m_parentCategory = m_parentCategory->parent;
        emit onUpdated();
        return true;
    }

    return false;
}

std::vector<Rect> TreemapModel::_getCurrentValues()
{
    std::vector<Rect> res;

    if(!m_parentCategory) {
        return res;
    }

    res.reserve(m_parentCategory->children.size());

    const Money parentSum = m_outCategoriesMap[m_parentCategory];
    if(parentSum.isZero()) {
        return res;
    }

    Money restSum = parentSum;

    const auto processCategory = [&res, parentSum, &restSum](const QString& name, Money childSum) {
        if (childSum.isZero()) {
            return;
        }
        restSum -= childSum;
        const qreal percent = childSum.as_cents() / static_cast<qreal>(parentSum.as_cents());
        res.emplace_back(Rect{name, formatMoney(childSum), percent});
    };

    for(const Node<Category>* child : m_parentCategory->children) {
        processCategory(child->data, m_outCategoriesMap[child]);
    }
    processCategory(tr("Остальное"), restSum);

    std::sort(res.begin(), res.end(), [](const Rect& a, const Rect& b) {
        return a.percentage > b.percentage;
    });

    return res;
}

std::vector<Rect> TreemapModel::getCurrenRects(float windowWidth, float windowHeight)
{
    std::vector<Rect> res = _getCurrentValues();

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

} // namespace cashbook
