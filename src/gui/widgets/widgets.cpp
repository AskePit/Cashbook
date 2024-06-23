#include "widgets.h"

namespace cashbook {

PopupTreeProxyModel::PopupTreeProxyModel(QObject* parent /*= nullptr*/)
    : QSortFilterProxyModel(parent)
{
    m_timer.callOnTimeout([this](){
        _doFilterWork();
    });
}

bool PopupTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceRow);

    if (!_isFiltered()) {
        return true;
    }

    //qDebug() << "filterAcceptsRow";

    auto *model = sourceModel();
    return m_filtereItems.contains(model->index(sourceRow, 0, sourceParent));
}

void PopupTreeProxyModel::setFilterString(const QString& filterString) {
    m_filterString = filterString.toLower();
    //qDebug() << "set filter:" << m_filterString;

    m_timer.stop();
    m_timer.start(500);
}

void PopupTreeProxyModel::_doFilterWork() {
    m_filtereItems.clear();
    m_mostMatched = QModelIndex();

    if(!_isFiltered()) {
        invalidateFilter();
        emit filterCanceled();
        return;
    }

    _doFilterIndex(QModelIndex());
    invalidateFilter();
    emit filtered();
}

bool PopupTreeProxyModel::_doFilterIndex(QModelIndex currIndex) {
    if (m_filtereItems.contains(currIndex)) {
        return true;
    }

    auto *model = sourceModel();
    bool childFiltered = false;

    for(int r = 0; r<model->rowCount(currIndex); ++r) {
        QModelIndex childIndex = model->index(r, 0, currIndex);
        if (_doFilterIndex(childIndex)) {
            childFiltered = true;
        }
    }

    if (childFiltered) {
        m_filtereItems.insert(currIndex);
        return true;
    }

    const bool pass = _filterPass(currIndex);

    if (pass) {
        m_filtereItems.insert(currIndex);
    }

    return pass;
}

static int levenshteinDistance(const QString& s1, const QString& s2) {
    const int len1 = static_cast<int>(s1.size()), len2 = static_cast<int>(s2.size());
    std::vector<int> v0(len2 + 1), v1(len2 + 1);

    for (int i = 0; i <= len2; ++i) {
        v0[i] = i;
    }

    for (int i = 0; i < len1; ++i) {
        v1[0] = i + 1;

        for (int j = 0; j < len2; ++j) {
            int cost = (s1[i] == s2[j]) ? 0 : 1;
            v1[j + 1] = std::min({ v1[j] + 1, v0[j + 1] + 1, v0[j] + cost });
        }

        std::swap(v0, v1);
    }

    return v0[len2];
}

bool PopupTreeProxyModel::_filterPass(QModelIndex currIndex) {
    if (!currIndex.isValid()) {
        return false;
    }

    auto *model = sourceModel();
    const QString& itemString = model->data(currIndex).toString();

    if (m_filterString.size() > 1) {
        if (itemString.startsWith(m_filterString, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return levenshteinDistance(m_filterString, itemString.toLower()) < 3;
}

bool PopupTreeProxyModel::_isFiltered() const {
    return !m_filterString.isEmpty();
}

template <>
void PopupTree<Category, CategoriesModel>::mouseDoubleClickEvent(QMouseEvent *event) {
    chooseValue(event);
}

template <>
void PopupTree<Category, CategoriesModel>::focusOutEvent(QFocusEvent *event) {
    (void)event;
    //chooseValue(event);
}

template <>
void PopupTree<Wallet, WalletsModel>::mouseDoubleClickEvent(QMouseEvent *event) {
    WalletsModel *model = getSourceModel();
    if(!model) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const Node<Wallet> *node = model->getItem(getCurrentSourceIndex());
    if(node->isLeaf()) {
        chooseValue(event);
    }
}

template <>
void PopupTree<Wallet, WalletsModel>::focusOutEvent(QFocusEvent *event) {
    (void)event;
//    WalletsModel *model = getSourceModel();
//    if(!model) {
//        selfDestroy();
//        return;
//    }

//    QModelIndex index = getCurrentSourceIndex();
//    if(!index.isValid()) {
//        selfDestroy();
//        return;
//    }

//    const Node<Wallet> *node = model->getItem(index);
//    if(!node) {
//        selfDestroy();
//        return;
//    }

//    if(node->isLeaf()) {
//        chooseValue(event);
//        return;
//    } else {
//        selfDestroy();
//        return;
//    }
}

} // namespace cashbook
