#include "chartwidget.h"
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <algorithm>

// ============================================================================
// GridLayoutItem
// ============================================================================

void GridLayoutItem::setGridPosition(int row, int col, int rowSpan, int colSpan)
{
    m_row = row;
    m_col = col;
    m_rowSpan = qMax(1, rowSpan);
    m_colSpan = qMax(1, colSpan);
}

// ============================================================================
// Layer
// ============================================================================

Layer::Layer(const QString &name, int order)
    : m_name(name)
    , m_order(order)
{
}

void Layer::addGraph(GraphBase *graph)
{
    if (graph && !m_graphs.contains(graph))
        m_graphs.append(graph);
}

void Layer::removeGraph(GraphBase *graph)
{
    m_graphs.removeAll(graph);
}

void Layer::clearGraphs()
{
    m_graphs.clear();
}

// ============================================================================
// BaseAxis
// ============================================================================

BaseAxis::BaseAxis(AxisOrientation orientation, QObject *parent)
    : QObject(parent)
    , m_orientation(orientation)
{
}

double BaseAxis::niceNumber(double x, bool roundUp) const
{
    if (qFuzzyIsNull(x))
        return 0.0;

    double exponent = qFloor(qLn(x) / qLn(10.0));
    double fraction = x / qPow(10.0, exponent);

    double nice;
    if (roundUp) {
        if (fraction <= 1.0)       nice = 1.0;
        else if (fraction <= 2.0)  nice = 2.0;
        else if (fraction <= 5.0)  nice = 5.0;
        else                       nice = 10.0;
    } else {
        if (fraction < 1.5)        nice = 1.0;
        else if (fraction < 3.0)   nice = 2.0;
        else if (fraction < 7.0)   nice = 5.0;
        else                       nice = 10.0;
    }

    return nice * qPow(10.0, exponent);
}

// ============================================================================
// NumericAxis
// ============================================================================

NumericAxis::NumericAxis(AxisOrientation orientation, QObject *parent)
    : BaseAxis(orientation, parent)
{
}

void NumericAxis::setRange(double min, double max)
{
    m_min = qMin(min, max);
    m_max = qMax(min, max);
    if (qFuzzyCompare(m_min, m_max))
        m_max = m_min + 1.0;
}

double NumericAxis::valueToPixel(const QVariant &value) const
{
    bool ok;
    double v = value.toDouble(&ok);
    if (!ok || m_cachePlotArea.isNull())
        return 0.0;

    double range = m_max - m_min;
    if (qFuzzyIsNull(range))
        return 0.0;

    if (m_orientation == Horizontal) {
        return m_cachePlotArea.left()
               + (v - m_min) / range * m_cachePlotArea.width();
    } else {
        return m_cachePlotArea.bottom()
               - (v - m_min) / range * m_cachePlotArea.height();
    }
}

QVariant NumericAxis::pixelToValue(double pixel) const
{
    if (m_cachePlotArea.isNull())
        return QVariant(0.0);

    double range = m_max - m_min;
    if (qFuzzyIsNull(range))
        return QVariant(m_min);

    if (m_orientation == Horizontal) {
        return QVariant(m_min + (pixel - m_cachePlotArea.left())
                        / m_cachePlotArea.width() * range);
    } else {
        return QVariant(m_min + (m_cachePlotArea.bottom() - pixel)
                        / m_cachePlotArea.height() * range);
    }
}

void NumericAxis::calculateTicks()
{
    m_tickValues.clear();
    m_tickPixelPositions.clear();

    if (qFuzzyCompare(m_min, m_max))
        m_max = m_min + 1.0;

    double range = m_max - m_min;
    double interval = niceNumber(range / (m_tickCount - 1), true);

    double firstTick = qFloor(m_min / interval) * interval;
    if (firstTick < m_min)
        firstTick += interval;

    for (double v = firstTick; v <= m_max + interval * 0.5; v += interval) {
        m_tickValues.append(QVariant(v));
        m_tickPixelPositions.append(valueToPixel(QVariant(v)));
    }
}

void NumericAxis::setLabelFormat(const QString &format)
{
    m_labelFormat = format;
}

QString NumericAxis::formatTickLabel(const QVariant &value) const
{
    bool ok;
    double v = value.toDouble(&ok);
    if (!ok)
        return value.toString();

    if (!m_labelFormat.isEmpty())
        return QString::asprintf(qPrintable(m_labelFormat), v);

    return QString::number(v, 'f', m_decimalPlaces);
}

void NumericAxis::draw(QPainter *painter, const QRectF &axisRect,
                       const QRectF &plotArea)
{
    if (!m_visible)
        return;

    m_cachePlotArea = plotArea;
    calculateTicks();

    QPen axisPen(m_axisColor, m_axisLineWidth);
    painter->save();
    painter->setPen(axisPen);

    if (m_orientation == Horizontal) {
        painter->drawLine(QPointF(plotArea.left(), plotArea.bottom()),
                          QPointF(plotArea.right(), plotArea.bottom()));
    } else {
        painter->drawLine(QPointF(plotArea.left(), plotArea.top()),
                          QPointF(plotArea.left(), plotArea.bottom()));
    }

    painter->setPen(QPen(m_tickColor, 1.0));
    for (int i = 0; i < m_tickPixelPositions.size(); ++i) {
        double pos = m_tickPixelPositions[i];
        if (m_orientation == Horizontal) {
            painter->drawLine(QPointF(pos, plotArea.bottom()),
                              QPointF(pos, plotArea.bottom() + m_tickLength));
        } else {
            painter->drawLine(QPointF(plotArea.left() - m_tickLength, pos),
                              QPointF(plotArea.left(), pos));
        }
    }

    if (m_showTickLabels) {
        painter->setPen(QPen(m_labelColor, 1.0));
        painter->setFont(m_labelFont);
        for (int i = 0; i < m_tickValues.size(); ++i) {
            QString label = formatTickLabel(m_tickValues[i]);
            double pos = m_tickPixelPositions[i];
            if (m_orientation == Horizontal) {
                QRectF textRect(pos - 50, plotArea.bottom() + m_tickLength + 2,
                                100, 20);
                painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);
            } else {
                QRectF textRect(plotArea.left() - m_tickLength - 60, pos - 10,
                                55, 20);
                painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
            }
        }
    }

    painter->restore();
}

void NumericAxis::zoomRange(double factor, const QVariant &center)
{
    double c = center.toDouble();
    double newMin = c - (c - m_min) / factor;
    double newMax = c + (m_max - c) / factor;
    if (newMin < newMax && (newMax - newMin) > 1e-10)
        setRange(newMin, newMax);
}

void NumericAxis::panRange(double pixelDelta, double plotAreaSize)
{
    if (qFuzzyIsNull(plotAreaSize))
        return;
    double range = m_max - m_min;
    double shift = -pixelDelta / plotAreaSize * range;
    setRange(m_min + shift, m_max + shift);
}

// ============================================================================
// DateTimeAxis
// ============================================================================

DateTimeAxis::DateTimeAxis(AxisOrientation orientation, QObject *parent)
    : BaseAxis(orientation, parent)
{
    m_min = QDateTime::currentDateTime();
    m_max = m_min.addSecs(3600);
}

void DateTimeAxis::setRange(const QDateTime &min, const QDateTime &max)
{
    m_min = qMin(min, max);
    m_max = qMax(min, max);
    if (m_min == m_max)
        m_max = m_min.addSecs(60);
}

void DateTimeAxis::setLabelFormat(const QString &format)
{
    m_labelFormat = format;
}

double DateTimeAxis::valueToPixel(const QVariant &value) const
{
    QDateTime dt = value.toDateTime();
    if (!dt.isValid() || m_cachePlotArea.isNull())
        return 0.0;

    qint64 range = m_min.secsTo(m_max);
    if (range == 0)
        return 0.0;

    qint64 pos = m_min.secsTo(dt);

    if (m_orientation == Horizontal) {
        return m_cachePlotArea.left()
               + static_cast<double>(pos) / range * m_cachePlotArea.width();
    } else {
        return m_cachePlotArea.bottom()
               - static_cast<double>(pos) / range * m_cachePlotArea.height();
    }
}

QVariant DateTimeAxis::pixelToValue(double pixel) const
{
    if (m_cachePlotArea.isNull())
        return QVariant();

    qint64 range = m_min.secsTo(m_max);
    if (range == 0)
        return QVariant(m_min);

    double fraction;
    if (m_orientation == Horizontal) {
        fraction = (pixel - m_cachePlotArea.left()) / m_cachePlotArea.width();
    } else {
        fraction = (m_cachePlotArea.bottom() - pixel) / m_cachePlotArea.height();
    }

    return QVariant(m_min.addSecs(static_cast<qint64>(fraction * range)));
}

qint64 DateTimeAxis::niceInterval(qint64 rangeSecs) const
{
    if (rangeSecs <= 0)
        return 60;

    QVector<qint64> candidates = {
        1, 5, 10, 15, 30,
        60, 120, 300, 600, 900, 1800,
        3600, 7200, 14400, 21600, 43200, 86400
    };

    qint64 rough = rangeSecs / (m_tickCount - 1);

    for (qint64 c : candidates) {
        if (c >= rough)
            return c;
    }
    return candidates.last();
}

void DateTimeAxis::calculateTicks()
{
    m_tickValues.clear();
    m_tickPixelPositions.clear();

    qint64 range = m_min.secsTo(m_max);
    if (range <= 0)
        range = 60;

    qint64 interval = niceInterval(range);

    qint64 startSecs = m_min.toSecsSinceEpoch();
    qint64 firstTick = (startSecs / interval) * interval;
    if (firstTick < startSecs)
        firstTick += interval;

    qint64 endSecs = m_max.toSecsSinceEpoch();
    for (qint64 s = firstTick; s <= endSecs + interval / 2; s += interval) {
        QDateTime tickTime = QDateTime::fromSecsSinceEpoch(s);
        m_tickValues.append(QVariant(tickTime));
        m_tickPixelPositions.append(valueToPixel(QVariant(tickTime)));
    }
}

QString DateTimeAxis::formatTickLabel(const QVariant &value) const
{
    QDateTime dt = value.toDateTime();
    if (!dt.isValid())
        return value.toString();

    return dt.toString(m_labelFormat);
}

void DateTimeAxis::draw(QPainter *painter, const QRectF &axisRect,
                        const QRectF &plotArea)
{
    if (!m_visible)
        return;

    m_cachePlotArea = plotArea;
    calculateTicks();

    QPen axisPen(m_axisColor, m_axisLineWidth);
    painter->save();
    painter->setPen(axisPen);

    if (m_orientation == Horizontal) {
        painter->drawLine(QPointF(plotArea.left(), plotArea.bottom()),
                          QPointF(plotArea.right(), plotArea.bottom()));
    } else {
        painter->drawLine(QPointF(plotArea.left(), plotArea.top()),
                          QPointF(plotArea.left(), plotArea.bottom()));
    }

    painter->setPen(QPen(m_tickColor, 1.0));
    for (int i = 0; i < m_tickPixelPositions.size(); ++i) {
        double pos = m_tickPixelPositions[i];
        if (m_orientation == Horizontal) {
            painter->drawLine(QPointF(pos, plotArea.bottom()),
                              QPointF(pos, plotArea.bottom() + m_tickLength));
        } else {
            painter->drawLine(QPointF(plotArea.left() - m_tickLength, pos),
                              QPointF(plotArea.left(), pos));
        }
    }

    if (m_showTickLabels) {
        painter->setPen(QPen(m_labelColor, 1.0));
        painter->setFont(m_labelFont);
        double labelWidth = (m_orientation == Horizontal) ? 120.0 : 55.0;
        for (int i = 0; i < m_tickValues.size(); ++i) {
            QString label = formatTickLabel(m_tickValues[i]);
            double pos = m_tickPixelPositions[i];
            if (m_orientation == Horizontal) {
                QRectF textRect(pos - labelWidth / 2.0,
                                plotArea.bottom() + m_tickLength + 2,
                                labelWidth, 20);
                painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);
            } else {
                QRectF textRect(plotArea.left() - m_tickLength - 60,
                                pos - 10, 55, 20);
                painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
            }
        }
    }

    painter->restore();
}

void DateTimeAxis::zoomRange(double factor, const QVariant &center)
{
    QDateTime c = center.toDateTime();
    if (!c.isValid()) return;

    qint64 minSecs = m_min.toSecsSinceEpoch();
    qint64 maxSecs = m_max.toSecsSinceEpoch();
    qint64 centerSecs = c.toSecsSinceEpoch();

    double dMin = static_cast<double>(centerSecs - minSecs) / factor;
    double dMax = static_cast<double>(maxSecs - centerSecs) / factor;

    qint64 newMin = centerSecs - qMax(static_cast<qint64>(qRound(dMin)), qint64(0));
    qint64 newMax = centerSecs + qMax(static_cast<qint64>(qRound(dMax)), qint64(0));

    if (newMin >= newMax) {
        newMin = centerSecs;
        newMax = centerSecs + 1;
    }

    setRange(QDateTime::fromSecsSinceEpoch(newMin),
             QDateTime::fromSecsSinceEpoch(newMax));
}

void DateTimeAxis::panRange(double pixelDelta, double plotAreaSize)
{
    if (qFuzzyIsNull(plotAreaSize))
        return;
    qint64 range = m_min.secsTo(m_max);
    m_panRemainder += -pixelDelta / plotAreaSize * range;
    qint64 shift = static_cast<qint64>(m_panRemainder);
    if (shift != 0) {
        m_panRemainder -= shift;
        setRange(m_min.addSecs(shift), m_max.addSecs(shift));
    }
}

// ============================================================================
// DateAxis
// ============================================================================

DateAxis::DateAxis(AxisOrientation orientation, QObject *parent)
    : BaseAxis(orientation, parent)
{
    m_min = QDate::currentDate();
    m_max = m_min.addDays(30);
}

void DateAxis::setRange(const QDate &min, const QDate &max)
{
    m_min = qMin(min, max);
    m_max = qMax(min, max);
    if (m_min == m_max)
        m_max = m_min.addDays(1);
}

void DateAxis::setLabelFormat(const QString &format)
{
    m_labelFormat = format;
}

double DateAxis::valueToPixel(const QVariant &value) const
{
    QDate d = value.toDate();
    if (!d.isValid() || m_cachePlotArea.isNull())
        return 0.0;

    qint64 range = m_min.daysTo(m_max);
    if (range == 0)
        return 0.0;

    qint64 pos = m_min.daysTo(d);

    if (m_orientation == Horizontal) {
        return m_cachePlotArea.left()
               + static_cast<double>(pos) / range * m_cachePlotArea.width();
    } else {
        return m_cachePlotArea.bottom()
               - static_cast<double>(pos) / range * m_cachePlotArea.height();
    }
}

QVariant DateAxis::pixelToValue(double pixel) const
{
    if (m_cachePlotArea.isNull())
        return QVariant();

    qint64 range = m_min.daysTo(m_max);
    if (range == 0)
        return QVariant(m_min);

    double fraction;
    if (m_orientation == Horizontal) {
        fraction = (pixel - m_cachePlotArea.left()) / m_cachePlotArea.width();
    } else {
        fraction = (m_cachePlotArea.bottom() - pixel) / m_cachePlotArea.height();
    }

    return QVariant(m_min.addDays(static_cast<qint64>(fraction * range)));
}

qint64 DateAxis::niceInterval(qint64 rangeDays) const
{
    if (rangeDays <= 0)
        return 1;

    QVector<qint64> candidates = {
        1, 2, 5, 7, 10, 14, 15, 21, 30, 60, 90, 120, 180, 365
    };

    qint64 rough = rangeDays / (m_tickCount - 1);

    for (qint64 c : candidates) {
        if (c >= rough)
            return c;
    }
    return candidates.last();
}

void DateAxis::calculateTicks()
{
    m_tickValues.clear();
    m_tickPixelPositions.clear();

    qint64 range = m_min.daysTo(m_max);
    if (range <= 0)
        range = 1;

    qint64 interval = niceInterval(range);
    qint64 epochStart = m_min.toJulianDay();
    qint64 epochEnd = m_max.toJulianDay();

    qint64 firstTick = (epochStart / interval) * interval;
    if (firstTick < epochStart)
        firstTick += interval;

    for (qint64 jd = firstTick; jd <= epochEnd + interval / 2; jd += interval) {
        QDate tickDate = QDate::fromJulianDay(jd);
        m_tickValues.append(QVariant(tickDate));
        m_tickPixelPositions.append(valueToPixel(QVariant(tickDate)));
    }
}

QString DateAxis::formatTickLabel(const QVariant &value) const
{
    QDate d = value.toDate();
    if (!d.isValid())
        return value.toString();

    return d.toString(m_labelFormat);
}

void DateAxis::draw(QPainter *painter, const QRectF &axisRect,
                    const QRectF &plotArea)
{
    if (!m_visible)
        return;

    m_cachePlotArea = plotArea;
    calculateTicks();

    QPen axisPen(m_axisColor, m_axisLineWidth);
    painter->save();
    painter->setPen(axisPen);

    if (m_orientation == Horizontal) {
        painter->drawLine(QPointF(plotArea.left(), plotArea.bottom()),
                          QPointF(plotArea.right(), plotArea.bottom()));
    } else {
        painter->drawLine(QPointF(plotArea.left(), plotArea.top()),
                          QPointF(plotArea.left(), plotArea.bottom()));
    }

    painter->setPen(QPen(m_tickColor, 1.0));
    for (int i = 0; i < m_tickPixelPositions.size(); ++i) {
        double pos = m_tickPixelPositions[i];
        if (m_orientation == Horizontal) {
            painter->drawLine(QPointF(pos, plotArea.bottom()),
                              QPointF(pos, plotArea.bottom() + m_tickLength));
        } else {
            painter->drawLine(QPointF(plotArea.left() - m_tickLength, pos),
                              QPointF(plotArea.left(), pos));
        }
    }

    if (m_showTickLabels) {
        painter->setPen(QPen(m_labelColor, 1.0));
        painter->setFont(m_labelFont);
        double labelWidth = (m_orientation == Horizontal) ? 120.0 : 60.0;
        for (int i = 0; i < m_tickValues.size(); ++i) {
            QString label = formatTickLabel(m_tickValues[i]);
            double pos = m_tickPixelPositions[i];
            if (m_orientation == Horizontal) {
                QRectF textRect(pos - labelWidth / 2.0,
                                plotArea.bottom() + m_tickLength + 2,
                                labelWidth, 20);
                painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);
            } else {
                QRectF textRect(plotArea.left() - m_tickLength - 70,
                                pos - 10, 65, 20);
                painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
            }
        }
    }

    painter->restore();
}

void DateAxis::zoomRange(double factor, const QVariant &center)
{
    QDate c = center.toDate();
    if (!c.isValid()) return;

    qint64 minJD = m_min.toJulianDay();
    qint64 maxJD = m_max.toJulianDay();
    qint64 centerJD = c.toJulianDay();

    double dMin = static_cast<double>(centerJD - minJD) / factor;
    double dMax = static_cast<double>(maxJD - centerJD) / factor;

    qint64 newMin = centerJD - qMax(static_cast<qint64>(qRound(dMin)), qint64(0));
    qint64 newMax = centerJD + qMax(static_cast<qint64>(qRound(dMax)), qint64(0));

    if (newMin >= newMax) {
        newMin = centerJD;
        newMax = centerJD + 1;
    }

    setRange(QDate::fromJulianDay(newMin), QDate::fromJulianDay(newMax));
}

void DateAxis::panRange(double pixelDelta, double plotAreaSize)
{
    if (qFuzzyIsNull(plotAreaSize))
        return;
    qint64 range = m_min.daysTo(m_max);
    m_panRemainder += -pixelDelta / plotAreaSize * range;
    qint64 shift = static_cast<qint64>(m_panRemainder);
    if (shift != 0) {
        m_panRemainder -= shift;
        setRange(m_min.addDays(shift), m_max.addDays(shift));
    }
}

// ============================================================================
// AxisRect
// ============================================================================

AxisRect::AxisRect()
{
}

AxisRect::~AxisRect()
{
}

void AxisRect::setXAxis(BaseAxis *axis)
{
    m_xAxis = axis;
}

void AxisRect::setYAxis(BaseAxis *axis)
{
    m_yAxis = axis;
}

void AxisRect::addGraph(GraphBase *graph)
{
    if (graph && !m_graphs.contains(graph))
        m_graphs.append(graph);
}

void AxisRect::removeGraph(GraphBase *graph)
{
    m_graphs.removeAll(graph);
}

void AxisRect::clearGraphs()
{
    m_graphs.clear();
}

QRectF AxisRect::plotArea() const
{
    return m_geometry.marginsRemoved(m_margins.toMargins());
}

QSize AxisRect::sizeHint() const
{
    return QSize(400, 300);
}

void AxisRect::setGeometry(const QRect &rect)
{
    m_geometry = rect;
}

QRect AxisRect::geometry() const
{
    return m_geometry;
}

void AxisRect::render(QPainter *painter)
{
    QRectF pa = plotArea();
    if (pa.isEmpty())
        return;

    drawBackground(painter);

    if (m_showGrid)
        drawGridLines(painter);

    if (m_xAxis)
        m_xAxis->draw(painter, m_geometry, pa);

    if (m_yAxis)
        m_yAxis->draw(painter, m_geometry, pa);

    QVector<GraphBase *> sortedGraphs = m_graphs;
    std::sort(sortedGraphs.begin(), sortedGraphs.end(),
              [](GraphBase *a, GraphBase *b) {
                  return a->layer() < b->layer();
              });

    for (GraphBase *graph : sortedGraphs) {
        if (graph->isVisible())
            graph->draw(painter, m_xAxis, m_yAxis, pa);
    }
}

void AxisRect::drawBackground(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(m_backgroundColor));
    painter->drawRect(m_geometry);
    painter->restore();
}

void AxisRect::drawGridLines(QPainter *painter)
{
    QRectF pa = plotArea();
    painter->save();
    QPen gridPen(m_gridLineColor, 1.0, Qt::DotLine);
    painter->setPen(gridPen);

    if (m_showGridY && m_yAxis) {
        const auto &positions = m_yAxis->tickPixelPositions();
        for (double pos : positions) {
            painter->drawLine(QPointF(pa.left(), pos),
                              QPointF(pa.right(), pos));
        }
    }

    if (m_showGridX && m_xAxis) {
        const auto &positions = m_xAxis->tickPixelPositions();
        for (double pos : positions) {
            painter->drawLine(QPointF(pos, pa.top()),
                              QPointF(pos, pa.bottom()));
        }
    }

    painter->restore();
}

// ============================================================================
// ChartTable
// ============================================================================

ChartTable::ChartTable(int rows, int cols)
    : m_rows(qMax(1, rows))
    , m_cols(qMax(1, cols))
{
}

ChartTable::~ChartTable()
{
}

void ChartTable::setGridSize(int rows, int cols)
{
    m_rows = qMax(1, rows);
    m_cols = qMax(1, cols);
    recalculateLayout();
}

void ChartTable::addItem(GridLayoutItem *item, int row, int col,
                          int rowSpan, int colSpan)
{
    if (!item || m_items.contains(item))
        return;

    item->setGridPosition(row, col, rowSpan, colSpan);
    m_items.append(item);
    recalculateLayout();
}

void ChartTable::removeItem(GridLayoutItem *item)
{
    m_items.removeAll(item);
    recalculateLayout();
}

GridLayoutItem *ChartTable::itemAt(int row, int col) const
{
    for (GridLayoutItem *item : m_items) {
        int r = item->row();
        int c = item->col();
        if (r <= row && row < r + item->rowSpan()
            && c <= col && col < c + item->colSpan())
            return item;
    }
    return nullptr;
}

void ChartTable::clearItems()
{
    m_items.clear();
    recalculateLayout();
}

QSize ChartTable::sizeHint() const
{
    return QSize(m_cols * 400, m_rows * 300);
}

void ChartTable::setGeometry(const QRect &rect)
{
    m_geometry = rect;
    recalculateLayout();
}

QRect ChartTable::geometry() const
{
    return m_geometry;
}

void ChartTable::render(QPainter *painter)
{
    for (GridLayoutItem *item : m_items)
        item->render(painter);
}

void ChartTable::recalculateLayout()
{
    if (m_geometry.isEmpty())
        return;

    double cellWidth = m_geometry.width() / static_cast<double>(m_cols);
    double cellHeight = m_geometry.height() / static_cast<double>(m_rows);

    for (GridLayoutItem *item : m_items) {
        int r = item->row();
        int c = item->col();
        int rs = item->rowSpan();
        int cs = item->colSpan();

        QRect cellRect(
            static_cast<int>(m_geometry.left() + c * cellWidth),
            static_cast<int>(m_geometry.top() + r * cellHeight),
            static_cast<int>(cellWidth * cs),
            static_cast<int>(cellHeight * rs)
        );

        item->setGeometry(cellRect);
    }
}

GridLayoutItem *ChartTable::itemAtPos(const QPoint &pos) const
{
    for (GridLayoutItem *item : m_items) {
        if (item->geometry().contains(pos))
            return item;
    }
    return nullptr;
}

// ============================================================================
// ChartWidget
// ============================================================================

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 150);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    m_chartTable = new ChartTable(1, 1);
    m_ownsTable = true;
}

ChartWidget::~ChartWidget()
{
    if (m_ownsTable)
        delete m_chartTable;
}

void ChartWidget::setChartTable(ChartTable *table)
{
    if (m_ownsTable)
        delete m_chartTable;

    m_chartTable = table;
    m_ownsTable = false;
    invalidateBuffer();
}

void ChartWidget::invalidateBuffer()
{
    m_bufferDirty = true;
    update();
}

void ChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (m_bufferDirty || m_buffer.size() != size()) {
        updateBuffer();
        m_bufferDirty = false;
    }

    QPainter widgetPainter(this);
    widgetPainter.drawPixmap(0, 0, m_buffer);
}

void ChartWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    invalidateBuffer();
}

void ChartWidget::updateBuffer()
{
    if (size().isEmpty())
        return;

    m_buffer = QPixmap(size());
    m_buffer.fill(Qt::white);

    QPainter bufferPainter(&m_buffer);
    bufferPainter.setRenderHint(QPainter::Antialiasing, true);

    if (m_chartTable) {
        m_chartTable->setGeometry(rect());
        m_chartTable->render(&bufferPainter);
    }
}

void ChartWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_chartTable || !(m_interactions & ZoomWheel) || m_zoomAxes == ZoomNone) {
        QWidget::wheelEvent(event);
        return;
    }

    GridLayoutItem *item = m_chartTable->itemAtPos(event->pos());
    AxisRect *axisRect = dynamic_cast<AxisRect *>(item);
    if (!axisRect) {
        QWidget::wheelEvent(event);
        return;
    }

    QPointF pos = event->posF();
    QRectF pa = axisRect->plotArea();
    if (!pa.contains(pos)) {
        QWidget::wheelEvent(event);
        return;
    }

    double factor = (event->angleDelta().y() > 0) ? m_zoomFactor
                                                   : (1.0 / m_zoomFactor);

    if ((m_zoomAxes & ZoomX) && axisRect->xAxis()
        && axisRect->xAxis()->isZoomEnabled()) {
        QVariant center = axisRect->xAxis()->pixelToValue(pos.x());
        axisRect->xAxis()->zoomRange(factor, center);
    }

    if ((m_zoomAxes & ZoomY) && axisRect->yAxis()
        && axisRect->yAxis()->isZoomEnabled()) {
        QVariant center = axisRect->yAxis()->pixelToValue(pos.y());
        axisRect->yAxis()->zoomRange(factor, center);
    }

    invalidateBuffer();
    event->accept();
}

void ChartWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_chartTable
        && (m_interactions & DragPan) && m_panAxes != PanNone) {
        GridLayoutItem *item = m_chartTable->itemAtPos(event->pos());
        AxisRect *ar = dynamic_cast<AxisRect *>(item);
        if (ar) {
            m_panning = true;
            m_panLastPos = event->pos();
            m_panAxisRect = ar;

            if (auto *dta = dynamic_cast<DateTimeAxis *>(ar->xAxis()))
                dta->resetPanRemainder();
            if (auto *da = dynamic_cast<DateAxis *>(ar->xAxis()))
                da->resetPanRemainder();
            if (auto *dta = dynamic_cast<DateTimeAxis *>(ar->yAxis()))
                dta->resetPanRemainder();
            if (auto *da = dynamic_cast<DateAxis *>(ar->yAxis()))
                da->resetPanRemainder();

            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning && m_panAxisRect) {
        QPoint delta = event->pos() - m_panLastPos;
        m_panLastPos = event->pos();

        QRectF pa = m_panAxisRect->plotArea();

        if ((m_panAxes & PanX) && m_panAxisRect->xAxis()
            && m_panAxisRect->xAxis()->isPanEnabled())
            m_panAxisRect->xAxis()->panRange(static_cast<double>(delta.x()),
                                              pa.width());

        if ((m_panAxes & PanY) && m_panAxisRect->yAxis()
            && m_panAxisRect->yAxis()->isPanEnabled())
            m_panAxisRect->yAxis()->panRange(static_cast<double>(-delta.y()),
                                              pa.height());

        invalidateBuffer();
        event->accept();
        return;
    }

    // Tooltip signal — emit nearest point info when hovering
    if (m_chartTable) {
        GridLayoutItem *item = m_chartTable->itemAtPos(event->pos());
        AxisRect *ar = dynamic_cast<AxisRect *>(item);
        if (ar && ar->plotArea().contains(event->pos())) {
            ChartToolTipInfo info;
            info.axisRect = ar;
            info.pixelPos = event->pos();

            double bestDist = std::numeric_limits<double>::max();
            for (GraphBase *graph : ar->graphs()) {
                if (!graph->isVisible()) continue;
                QVariant key, value;
                double dist;
                if (graph->nearestPoint(ar->xAxis(), ar->yAxis(),
                                        ar->plotArea(), event->pos(),
                                        key, value, dist)) {
                    if (dist < bestDist) {
                        bestDist = dist;
                        info.graph = graph;
                        info.dataKey = key;
                        info.dataValue = value;
                        info.distancePx = dist;
                    }
                }
            }
            emit toolTipRequested(info);
            return;
        }
    }

    QWidget::mouseMoveEvent(event);
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        m_panAxisRect = nullptr;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
