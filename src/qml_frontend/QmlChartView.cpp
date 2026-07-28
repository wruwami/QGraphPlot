#include "QmlChartView.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/qmath.h>
#include <QtQuick/QSGSimpleRectNode>

#include "QmlLineSeries.h"
#include "QmlScatterSeries.h"
#include "transform/QAutoScaler.h"

namespace qgraphplot
{

namespace
{
Q_LOGGING_CATEGORY(lcChartView, "qgraphplot.chartview")
}  // namespace

QmlChartView::QmlChartView(QQuickItem* parent) : QQuickItem(parent)
{
    // Renders the background (entire item) and plot area (area excluding margins) directly with
    // theme colors. Without a theme, updatePaintNode returns nullptr so the item remains
    // transparent.
    setFlag(ItemHasContents, true);
}

QmlChartView::~QmlChartView()
{
    // Disconnect external-destroy tracking before ~QObject deletes children,
    // otherwise the destroyed handlers would access already-destroyed members.
    // The QML wrappers own the core series; the chart only observes, so it
    // must not reparent or delete them (issue #58).
    for (auto it = m_seriesDestroyedConnections.constBegin();
         it != m_seriesDestroyedConnections.constEnd();
         ++it) {
        disconnect(it.value());
    }
    m_seriesDestroyedConnections.clear();
    for (auto it = m_autoScaleConnections.constBegin(); it != m_autoScaleConnections.constEnd();
         ++it) {
        disconnect(it.value().first);
        disconnect(it.value().second);
    }
    m_autoScaleConnections.clear();
}

void QmlChartView::setTheme(qgraphplot::QGraphPlotTheme* theme)
{
    if (m_theme == theme) {
        return;
    }
    if (m_theme) {
        disconnect(m_theme, nullptr, this, nullptr);
    }
    m_theme = theme;
    applyThemeConnections();
    emit themeChanged();
    update();
}

void QmlChartView::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    emit titleChanged();
    update();
}

void QmlChartView::applyThemeConnections()
{
    if (!m_theme) {
        return;
    }
    connect(m_theme,
            &qgraphplot::QGraphPlotTheme::themeChanged,
            this,
            &QQuickItem::update,
            Qt::UniqueConnection);
    // A theme destroyed while still assigned must repaint (QPointer nulls
    // itself, so the next updatePaintNode drops the background nodes).
    connect(m_theme, &QObject::destroyed, this, [this]() {
        emit themeChanged();
        update();
    });
}

void QmlChartView::addSeries(qgraphplot::QAbstractSeries* aSeries)
{
    if (!aSeries || m_series.contains(aSeries)) {
        return;
    }
    m_series.append(aSeries);
    // The QML wrappers (QmlLineSeries/QmlScatterSeries) own the core series
    // via Qt parent/child, so the chart does NOT reparent or delete it — it
    // only observes destruction to drop stale entries. This differs from
    // WidgetChartView::addSeries, which takes ownership via setParent(this),
    // because in QML the wrapper item is the declarative owner (issue #58).
    m_seriesDestroyedConnections[aSeries] =
        connect(aSeries, &QObject::destroyed, this, [this](QObject* obj) {
            auto* series = static_cast<qgraphplot::QAbstractSeries*>(obj);
            m_series.removeAll(series);
            m_seriesDestroyedConnections.remove(series);
            disconnectAutoScaleModel(series);
        });
    connectAutoScaleModel(aSeries);
    // The newly-added series may extend the auto-scaled range: recompute now
    // so the view follows the data immediately instead of waiting for the
    // next boundsChanged (issue #63).
    applyAutoScale();
    emit seriesAdded(aSeries);
}

void QmlChartView::removeSeries(qgraphplot::QAbstractSeries* aSeries)
{
    if (!m_series.contains(aSeries)) {
        return;
    }
    if (m_seriesDestroyedConnections.contains(aSeries)) {
        disconnect(m_seriesDestroyedConnections.take(aSeries));
    }
    disconnectAutoScaleModel(aSeries);
    m_series.removeOne(aSeries);
    // A removed series may have anchored the previous range; recompute so the
    // view tightens to the remaining data (issue #63).
    applyAutoScale();
    emit seriesRemoved(aSeries);
}

void QmlChartView::clearSeries()
{
    while (!m_series.isEmpty()) {
        qgraphplot::QAbstractSeries* aSeries = m_series.takeLast();
        if (m_seriesDestroyedConnections.contains(aSeries)) {
            disconnect(m_seriesDestroyedConnections.take(aSeries));
        }
        disconnectAutoScaleModel(aSeries);
        emit seriesRemoved(aSeries);
    }
    applyAutoScale();
}

qgraphplot::QAbstractSeries* QmlChartView::seriesAt(qsizetype index) const noexcept
{
    if (index < 0 || index >= m_series.size()) {
        return nullptr;
    }
    return m_series.at(index);
}

qgraphplot::QAbstractSeries* QmlChartView::coreSeriesFromItem(QQuickItem* item) const noexcept
{
    if (!item) {
        return nullptr;
    }
    // The QML wrappers compose a core series; ask each known wrapper type
    // for its core. qobject_cast keeps this safe against arbitrary children
    // (axes, labels, plain QQuickItems) that are not series wrappers.
    if (auto* line = qobject_cast<qgraphplot::QmlLineSeries*>(item)) {
        return line->coreSeries();
    }
    if (auto* scatter = qobject_cast<qgraphplot::QmlScatterSeries*>(item)) {
        return scatter->coreSeries();
    }
    return nullptr;
}

void QmlChartView::connectAutoScaleModel(qgraphplot::QAbstractSeries* aSeries)
{
    if (!aSeries || m_autoScaleConnections.contains(aSeries)) {
        return;
    }
    // Subscribe to modelChanged so a model assigned after addSeries still
    // wires boundsChanged; and to the current model's boundsChanged so the
    // auto-range follows streaming data (signal-driven, not per-frame).
    QMetaObject::Connection modelConn =
        connect(aSeries, &qgraphplot::QAbstractSeries::modelChanged, this, [this, aSeries]() {
            auto it = m_autoScaleConnections.find(aSeries);
            if (it == m_autoScaleConnections.end()) {
                return;
            }
            disconnect(it.value().second);
            it.value().second = QMetaObject::Connection();
            if (auto* model = aSeries->model()) {
                it.value().second = connect(model,
                                            &qgraphplot::QAbstractSeriesModel::boundsChanged,
                                            this,
                                            &QmlChartView::applyAutoScale,
                                            Qt::UniqueConnection);
            }
            applyAutoScale();
        });
    QMetaObject::Connection boundsConn;
    if (auto* model = aSeries->model()) {
        boundsConn = connect(model,
                             &qgraphplot::QAbstractSeriesModel::boundsChanged,
                             this,
                             &QmlChartView::applyAutoScale,
                             Qt::UniqueConnection);
    }
    m_autoScaleConnections.insert(aSeries, {modelConn, boundsConn});
}

void QmlChartView::disconnectAutoScaleModel(qgraphplot::QAbstractSeries* aSeries)
{
    auto it = m_autoScaleConnections.find(aSeries);
    if (it == m_autoScaleConnections.end()) {
        return;
    }
    disconnect(it.value().first);
    disconnect(it.value().second);
    m_autoScaleConnections.erase(it);
}

void QmlChartView::applyAutoScale()
{
    if (!m_autoScaleX && !m_autoScaleY) {
        return;
    }
    const QRectF padded =
        qgraphplot::QAutoScaler::computePaddedBounds(m_series, m_autoScalePadding);
    // computePaddedBounds guarantees min < max on both axes, so we bypass
    // the manual setters' "min < max" validation and write the members
    // directly (autoScale is the source of truth while enabled — see the
    // priority rule in QmlChartView.h).
    bool changed = false;
    if (m_autoScaleX) {
        if (qgraphplot::fuzzyValuesDiffer(m_xMin, padded.left())) {
            m_xMin = padded.left();
            emit xMinChanged();
            changed = true;
        }
        if (qgraphplot::fuzzyValuesDiffer(m_xMax, padded.right())) {
            m_xMax = padded.right();
            emit xMaxChanged();
            changed = true;
        }
    }
    if (m_autoScaleY) {
        if (qgraphplot::fuzzyValuesDiffer(m_yMin, padded.top())) {
            m_yMin = padded.top();
            emit yMinChanged();
            changed = true;
        }
        if (qgraphplot::fuzzyValuesDiffer(m_yMax, padded.bottom())) {
            m_yMax = padded.bottom();
            emit yMaxChanged();
            changed = true;
        }
    }
    if (changed) {
        emit transformChanged();
        update();
    }
}

void QmlChartView::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    // Declarative QML adds series as children of the ChartView
    // (ChartView { LineSeries {...} }). Detect such children and register
    // their core QAbstractSeries* into the collection (issue #58).
    // ItemChildRemovedChange mirrors the same lifecycle so reparenting and
    // explicit removal both keep the collection in sync.
    if (change == ItemChildAddedChange && value.item) {
        if (auto* core = coreSeriesFromItem(value.item)) {
            addSeries(core);
        }
    } else if (change == ItemChildRemovedChange && value.item) {
        if (auto* core = coreSeriesFromItem(value.item)) {
            removeSeries(core);
        }
    }
}

void QmlChartView::setXMin(double val)
{
    if (!qIsFinite(val) || val >= m_xMax) {
        qCWarning(lcChartView)
            << "QmlChartView::setXMin: rejected non-finite or invalid xMin (must be < xMax):"
            << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMin, val)) {
        m_xMin = val;
        emit xMinChanged();
        emit transformChanged();
    }
}

void QmlChartView::setXMax(double val)
{
    if (!qIsFinite(val) || val <= m_xMin) {
        qCWarning(lcChartView)
            << "QmlChartView::setXMax: rejected non-finite or invalid xMax (must be > xMin):"
            << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMax, val)) {
        m_xMax = val;
        emit xMaxChanged();
        emit transformChanged();
    }
}

void QmlChartView::setYMin(double val)
{
    if (!qIsFinite(val) || val >= m_yMax) {
        qCWarning(lcChartView)
            << "QmlChartView::setYMin: rejected non-finite or invalid yMin (must be < yMax):"
            << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMin, val)) {
        m_yMin = val;
        emit yMinChanged();
        emit transformChanged();
    }
}

void QmlChartView::setYMax(double val)
{
    if (!qIsFinite(val) || val <= m_yMin) {
        qCWarning(lcChartView)
            << "QmlChartView::setYMax: rejected non-finite or invalid yMax (must be > yMin):"
            << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMax, val)) {
        m_yMax = val;
        emit yMaxChanged();
        emit transformChanged();
    }
}

void QmlChartView::setXRange(double min, double max)
{
    if (!qIsFinite(min) || !qIsFinite(max) || min >= max) {
        return;
    }
    if (m_autoScaleX) {
        m_autoScaleX = false;
        emit autoScaleXChanged();
    }
    bool changed = false;
    if (qgraphplot::fuzzyValuesDiffer(m_xMin, min)) {
        m_xMin = min;
        emit xMinChanged();
        changed = true;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_xMax, max)) {
        m_xMax = max;
        emit xMaxChanged();
        changed = true;
    }
    if (changed) {
        emit transformChanged();
    }
}

void QmlChartView::setYRange(double min, double max)
{
    if (!qIsFinite(min) || !qIsFinite(max) || min >= max) {
        return;
    }
    if (m_autoScaleY) {
        m_autoScaleY = false;
        emit autoScaleYChanged();
    }
    bool changed = false;
    if (qgraphplot::fuzzyValuesDiffer(m_yMin, min)) {
        m_yMin = min;
        emit yMinChanged();
        changed = true;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_yMax, max)) {
        m_yMax = max;
        emit yMaxChanged();
        changed = true;
    }
    if (changed) {
        emit transformChanged();
    }
}

void QmlChartView::setZoomXEnabled(bool enabled)
{
    if (m_zoomXEnabled == enabled) {
        return;
    }
    m_zoomXEnabled = enabled;
    emit zoomXEnabledChanged();
}

void QmlChartView::setZoomYEnabled(bool enabled)
{
    if (m_zoomYEnabled == enabled) {
        return;
    }
    m_zoomYEnabled = enabled;
    emit zoomYEnabledChanged();
}

void QmlChartView::setAutoScaleX(bool enabled)
{
    if (m_autoScaleX == enabled) {
        return;
    }
    m_autoScaleX = enabled;
    emit autoScaleXChanged();
    if (enabled) {
        applyAutoScale();
    }
}

void QmlChartView::setAutoScaleY(bool enabled)
{
    if (m_autoScaleY == enabled) {
        return;
    }
    m_autoScaleY = enabled;
    emit autoScaleYChanged();
    if (enabled) {
        applyAutoScale();
    }
}

void QmlChartView::setAutoScalePadding(double ratio)
{
    if (!qIsFinite(ratio) || ratio < 0.0) {
        qCWarning(lcChartView)
            << "QmlChartView::setAutoScalePadding: rejected negative or non-finite ratio:" << ratio;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_autoScalePadding, ratio)) {
        m_autoScalePadding = ratio;
        emit autoScalePaddingChanged();
        if (m_autoScaleX || m_autoScaleY) {
            applyAutoScale();
        }
    }
}

void QmlChartView::setMarginLeft(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView)
            << "QmlChartView::setMarginLeft: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginLeft, val)) {
        m_marginLeft = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginRight(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView)
            << "QmlChartView::setMarginRight: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginRight, val)) {
        m_marginRight = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginTop(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView)
            << "QmlChartView::setMarginTop: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginTop, val)) {
        m_marginTop = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

void QmlChartView::setMarginBottom(double val)
{
    if (!qIsFinite(val) || val < 0.0) {
        qCWarning(lcChartView)
            << "QmlChartView::setMarginBottom: rejected negative or non-finite margin:" << val;
        return;
    }
    if (qgraphplot::fuzzyValuesDiffer(m_marginBottom, val)) {
        m_marginBottom = val;
        emit marginsChanged();
        emit transformChanged();
        update();
    }
}

qgraphplot::QCoordinateTransform QmlChartView::coordinateTransform() const noexcept
{
    const QRectF dataBounds(m_xMin, m_yMin, m_xMax - m_xMin, m_yMax - m_yMin);
    const double pixelWidth = qMax(0.0, width() - m_marginLeft - m_marginRight);
    const double pixelHeight = qMax(0.0, height() - m_marginTop - m_marginBottom);
    const QRectF pixelRect(m_marginLeft, m_marginTop, pixelWidth, pixelHeight);
    return qgraphplot::QCoordinateTransform(dataBounds, pixelRect);
}

QPointF QmlChartView::mapToPixel(double x, double y) const noexcept
{
    return coordinateTransform().toPixel(QPointF(x, y));
}

void QmlChartView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        emit transformChanged();
        update();
    }
}

QSGNode* QmlChartView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updateData)
{
    Q_UNUSED(updateData);

    if (!m_theme || width() <= 0.0 || height() <= 0.0) {
        delete oldNode;
        return nullptr;
    }

    // Node tree: root ─┬─ background (entire item)
    //                  └─ plotArea   (area excluding margins)
    QSGNode* root = oldNode;
    QSGSimpleRectNode* background = nullptr;
    QSGSimpleRectNode* plotAreaNode = nullptr;

    if (!root) {
        root = new QSGNode();
        background = new QSGSimpleRectNode();
        plotAreaNode = new QSGSimpleRectNode();
        root->appendChildNode(background);
        root->appendChildNode(plotAreaNode);
    } else {
        background = static_cast<QSGSimpleRectNode*>(root->firstChild());
        plotAreaNode = static_cast<QSGSimpleRectNode*>(root->lastChild());
    }

    background->setRect(QRectF(0.0, 0.0, width(), height()));
    background->setColor(m_theme->backgroundColor());

    plotAreaNode->setRect(plotArea());
    plotAreaNode->setColor(m_theme->plotAreaColor());

    return root;
}

}  // namespace qgraphplot
