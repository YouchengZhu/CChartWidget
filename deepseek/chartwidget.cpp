#include "chartwidget.h"
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

// ============================================================================
// AxisRange
// ============================================================================

void AxisRange::expand(double v)
{
    if (v < mLower) mLower = v;
    if (v > mUpper) mUpper = v;
}

AxisRange AxisRange::expanded(double v) const
{
    AxisRange r = *this;
    r.expand(v);
    return r;
}

AxisRange AxisRange::sanitized(double minRange) const
{
    AxisRange r = *this;
    if (r.size() < minRange) {
        double mid = r.center();
        r.mLower = mid - minRange * 0.5;
        r.mUpper = mid + minRange * 0.5;
    }
    return r;
}

AxisRange AxisRange::bounded(double lo, double hi) const
{
    AxisRange r = *this;
    if (r.mLower < lo) r.mLower = lo;
    if (r.mUpper > hi) r.mUpper = hi;
    if (r.mLower > r.mUpper) r.mLower = r.mUpper;
    return r;
}

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
// AxisTicker
// ============================================================================

double AxisTicker::getMantissa(double input, double *magnitude)
{
    const double mag = std::pow(10.0, std::floor(std::log10(input)));
    if (magnitude) *magnitude = mag;
    return input / mag;
}

double AxisTicker::cleanMantissa(double input)
{
    double magnitude;
    const double mantissa = getMantissa(input, &magnitude);
    return pickClosest(mantissa,
                       QVector<double>() << 1.0 << 2.0 << 2.5 << 5.0 << 10.0)
           * magnitude;
}

double AxisTicker::pickClosest(double target, const QVector<double> &candidates)
{
    if (candidates.size() == 1)
        return candidates.first();
    auto it = std::lower_bound(candidates.cbegin(), candidates.cend(), target);
    if (it == candidates.cbegin())
        return candidates.first();
    if (it == candidates.cend())
        return candidates.last();
    double before = *(it - 1);
    double after = *it;
    return (after - target < target - before) ? after : before;
}

QVector<double> AxisTicker::createTickVector(double tickStep, double rangeMin,
                                              double rangeMax) const
{
    QVector<double> result;
    qint64 firstStep = qint64(std::floor((rangeMin - m_tickOrigin) / tickStep));
    qint64 lastStep  = qint64(std::ceil((rangeMax - m_tickOrigin) / tickStep));
    int tickCount = int(lastStep - firstStep + 1);
    if (tickCount < 0) tickCount = 0;
    result.resize(tickCount);
    for (int i = 0; i < tickCount; ++i)
        result[i] = m_tickOrigin + (firstStep + i) * tickStep;
    return result;
}

int AxisTicker::getSubTickCount(double tickStep) const
{
    int result = 1;
    const double epsilon = 0.01;
    double intPartf;
    double fracPart = modf(getMantissa(tickStep), &intPartf);
    int intPart = int(intPartf);

    if (fracPart < epsilon || 1.0 - fracPart < epsilon) {
        if (1.0 - fracPart < epsilon) ++intPart;
        switch (intPart) {
        case 1: result = 4; break; case 2: result = 3; break;
        case 3: result = 2; break; case 4: result = 3; break;
        case 5: result = 4; break; case 6: result = 2; break;
        case 7: result = 6; break; case 8: result = 3; break;
        case 9: result = 2; break;
        }
    } else if (qAbs(fracPart - 0.5) < epsilon) {
        switch (intPart) {
        case 1: result = 2; break; case 2: result = 4; break;
        case 3: result = 4; break; case 4: result = 2; break;
        case 5: result = 4; break; case 6: result = 4; break;
        case 7: result = 2; break; case 8: result = 4; break;
        case 9: result = 4; break;
        }
    }
    return result;
}

QVector<double> AxisTicker::createSubTickVector(int subTickCount,
                                                 const QVector<double> &ticks) const
{
    QVector<double> result;
    if (subTickCount <= 0 || ticks.size() < 2)
        return result;
    result.reserve((ticks.size() - 1) * subTickCount);
    for (int i = 1; i < ticks.size(); ++i) {
        double subStep = (ticks[i] - ticks[i - 1]) / double(subTickCount + 1);
        for (int j = 1; j <= subTickCount; ++j)
            result.append(ticks[i - 1] + subStep * j);
    }
    return result;
}

QVector<QString> AxisTicker::createLabelVector(const QVector<double> &ticks) const
{
    QVector<QString> result;
    result.reserve(ticks.size());
    for (double t : ticks)
        result.append(getTickLabel(t));
    return result;
}

void AxisTicker::trimTicks(double rangeMin, double rangeMax,
                           QVector<double> &ticks, bool keepOneOutlier)
{
    int lowIdx = -1, highIdx = -1;
    for (int i = 0; i < ticks.size(); ++i) {
        if (ticks[i] >= rangeMin) { lowIdx = i; break; }
    }
    for (int i = ticks.size() - 1; i >= 0; --i) {
        if (ticks[i] <= rangeMax) { highIdx = i; break; }
    }
    if (lowIdx < 0 || highIdx < 0) {
        ticks.clear();
        return;
    }
    int front = qMax(0, lowIdx - (keepOneOutlier ? 1 : 0));
    int back  = qMax(0, ticks.size() - (keepOneOutlier ? 2 : 1) - highIdx);
    if (front > 0 || back > 0)
        ticks = ticks.mid(front, ticks.size() - front - back);
}

QString AxisTicker::getTickLabel(double /*value*/) const
{
    return QString::number(0);
}

// ============================================================================
// NumericTicker
// ============================================================================

double NumericTicker::getTickStep(double rangeSize) const
{
    double exactStep = rangeSize / double(m_tickCount + 1e-10);
    return cleanMantissa(exactStep);
}

int NumericTicker::getSubTickCount(double tickStep) const
{
    return AxisTicker::getSubTickCount(tickStep);
}

QString NumericTicker::getTickLabel(double value) const
{
    if (!m_labelFormat.isEmpty())
        return QString::asprintf(qPrintable(m_labelFormat), value);
    return QString::number(value, 'f', m_decimalPlaces);
}

// ============================================================================
// DateTimeTicker
// ============================================================================

double DateTimeTicker::getTickStep(double rangeSize) const
{
    double result = rangeSize / double(m_tickCount + 1e-10);
    m_dateStrategy = dsNone;

    if (result < 1.0) {
        result = cleanMantissa(result);
    } else if (result < 86400.0 * 30.4375 * 12.0) {
        result = pickClosest(result, QVector<double>()
            << 1.0 << 2.5 << 5.0 << 10.0 << 15.0 << 30.0
            << 60.0 << 2.5*60 << 5*60 << 10*60 << 15*60 << 30*60 << 60*60
            << 3600.0*2 << 3600.0*3 << 3600.0*6 << 3600.0*12 << 3600.0*24
            << 86400.0*2 << 86400.0*5 << 86400.0*7 << 86400.0*14
            << 86400.0*30.4375 << 86400.0*30.4375*2 << 86400.0*30.4375*3
            << 86400.0*30.4375*6 << 86400.0*30.4375*12);
        if (result > 86400.0 * 30.4375 - 1.0)
            m_dateStrategy = dsUniformDayInMonth;
        else if (result > 3600.0 * 24.0 - 1.0)
            m_dateStrategy = dsUniformTimeInDay;
    } else {
        const double secsPerYear = 86400.0 * 30.4375 * 12.0;
        result = cleanMantissa(result / secsPerYear) * secsPerYear;
        m_dateStrategy = dsUniformDayInMonth;
    }
    return result;
}

int DateTimeTicker::getSubTickCount(double tickStep) const
{
    int result = AxisTicker::getSubTickCount(tickStep);
    switch (qRound(tickStep)) {
    case 5*60: result = 4; break; case 10*60: result = 1; break;
    case 15*60: result = 2; break; case 30*60: result = 1; break;
    case 60*60: result = 3; break; case 3600*2: result = 3; break;
    case 3600*3: result = 2; break; case 3600*6: result = 1; break;
    case 3600*12: result = 3; break; case 3600*24: result = 3; break;
    }
    return result;
}

QString DateTimeTicker::formatString(DateTimeFormat f)
{
    switch (f) {
    case DateTimeFormat::HHmm:         return "HH:mm";
    case DateTimeFormat::HHmmss:       return "HH:mm:ss";
    case DateTimeFormat::MMdd:         return "MM-dd";
    case DateTimeFormat::MMddHHmm:     return "MM-dd HH:mm";
    case DateTimeFormat::yyyyMMdd:     return "yyyy-MM-dd";
    case DateTimeFormat::yyyyMMddHHmm: return "yyyy-MM-dd HH:mm";
    case DateTimeFormat::yyyyMM:       return "yyyy-MM";
    case DateTimeFormat::MMMyy:        return "MMM yy";
    }
    return "HH:mm";
}

QString DateTimeTicker::getTickLabel(double value) const
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(qint64(value));
    return dt.toString(formatString(m_format));
}

// ============================================================================
// DateTicker
// ============================================================================

double DateTicker::getTickStep(double rangeSize) const
{
    double result = rangeSize / double(m_tickCount + 1e-10);
    result = pickClosest(result, QVector<double>()
        << 1.0 << 2.0 << 5.0 << 7.0 << 10.0 << 14.0 << 15.0 << 21.0
        << 30.0 << 30.4375 << 30.4375*2 << 30.4375*3 << 30.4375*6
        << 30.4375*12 << 365.25);
    return result;
}

int DateTicker::getSubTickCount(double tickStep) const
{
    int result = AxisTicker::getSubTickCount(tickStep);
    switch (qRound(tickStep)) {
    case 1: result = 0; break; case 7: result = 6; break;
    case 14: result = 1; break; case 30: result = 2; break;
    }
    return result;
}

QString DateTicker::formatString(DateTimeFormat f)
{
    switch (f) {
    case DateTimeFormat::yyyyMMdd:     return "yyyy-MM-dd";
    case DateTimeFormat::MMdd:         return "MM-dd";
    case DateTimeFormat::yyyyMM:       return "yyyy-MM";
    case DateTimeFormat::MMMyy:        return "MMM yy";
    default:                           return "yyyy-MM-dd";
    }
}

QString DateTicker::getTickLabel(double value) const
{
    QDate d = QDate::fromJulianDay(qint64(value));
    return d.toString(formatString(m_format));
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
    if (qFuzzyIsNull(x)) return 0.0;
    double exponent = qFloor(qLn(x) / qLn(10.0));
    double fraction = x / qPow(10.0, exponent);
    double nice;
    if (roundUp) {
        if (fraction <= 1.0) nice = 1.0; else if (fraction <= 2.0) nice = 2.0;
        else if (fraction <= 5.0) nice = 5.0; else nice = 10.0;
    } else {
        if (fraction < 1.5) nice = 1.0; else if (fraction < 3.0) nice = 2.0;
        else if (fraction < 7.0) nice = 5.0; else nice = 10.0;
    }
    return nice * qPow(10.0, exponent);
}

void BaseAxis::setTicker(AxisTicker *ticker)
{
    if (m_ownsTicker) delete m_ticker;
    m_ticker = ticker;
    m_ownsTicker = true;
}

void BaseAxis::setTickCount(int count) { if (m_ticker) m_ticker->setTickCount(count); }
int  BaseAxis::tickCount() const { return m_ticker ? m_ticker->tickCount() : 5; }

void BaseAxis::calculateTicks()
{
    m_tickValues.clear();
    m_tickValuesDouble.clear();
    m_tickPixelPositions.clear();
    m_subTickPixelPositions.clear();
    if (!m_ticker) return;

    double min = tickRangeMin();
    double max = tickRangeMax();
    if (qFuzzyCompare(min, max)) return;

    double range = max - min;
    double tickStep = m_ticker->getTickStep(range);
    QVector<double> ticks = m_ticker->createTickVector(tickStep, min, max);
    AxisTicker::trimTicks(min, max, ticks, false);

    // Determine pixel bounds for clamping
    double pixMin = 0.0, pixMax = 0.0;
    if (m_orientation == Horizontal) {
        pixMin = m_cachePlotArea.left();
        pixMax = m_cachePlotArea.right();
    } else {
        pixMin = m_cachePlotArea.top();
        pixMax = m_cachePlotArea.bottom();
    }

    for (double t : ticks) {
        m_tickValuesDouble.append(t);
        m_tickValues.append(tickValueToVariant(t));
        double pix = valueToPixel(tickValueToVariant(t));
        pix = qBound(pixMin, pix, pixMax);  // clamp outlier to plot edge
        m_tickPixelPositions.append(pix);
    }

    if (m_subTickCount > 0) {
        int subCnt = m_ticker->getSubTickCount(tickStep);
        if (subCnt > 0) {
            QVector<double> subTicks = m_ticker->createSubTickVector(subCnt, ticks);
            for (double st : subTicks) {
                double pix = valueToPixel(tickValueToVariant(st));
                if (pix >= pixMin && pix <= pixMax) // skip sub-ticks fully outside
                    m_subTickPixelPositions.append(pix);
            }
        }
    }
}

void BaseAxis::drawAxisEnding(QPainter *painter, AxisEnding ending,
                               const QPointF &pos, double angleDeg) const
{
    if (ending == EndingNone) return;
    double size = m_axisLineWidth * 5.0;
    painter->save();
    painter->translate(pos);
    painter->rotate(angleDeg);
    QPen p(m_axisColor, m_axisLineWidth);
    painter->setPen(p);
    painter->setBrush(m_axisColor);

    if (ending == EndingArrow) {
        QPointF pts[3] = { {0, 0}, {-size, -size * 0.6}, {-size, size * 0.6} };
        painter->drawPolygon(pts, 3);
    } else if (ending == EndingDisc) {
        painter->drawEllipse(QPointF(0, 0), size * 0.5, size * 0.5);
    }
    painter->restore();
}

void BaseAxis::drawSubTicks(QPainter *painter, const QRectF &plotArea) const
{
    if (m_subTickPixelPositions.isEmpty()) return;
    painter->save();
    painter->setPen(QPen(m_subTickColor, 1.0));
    for (double pos : m_subTickPixelPositions) {
        if (m_orientation == Horizontal) {
            painter->drawLine(QPointF(pos, plotArea.bottom()),
                              QPointF(pos, plotArea.bottom() + m_subTickLength));
        } else {
            painter->drawLine(QPointF(plotArea.left() - m_subTickLength, pos),
                              QPointF(plotArea.left(), pos));
        }
    }
    painter->restore();
}

// ============================================================================
// NumericAxis
// ============================================================================

NumericAxis::NumericAxis(AxisOrientation orientation, QObject *parent)
    : BaseAxis(orientation, parent)
{
    setTicker(new NumericTicker);
}

void NumericAxis::setDecimalPlaces(int places)
{
    if (auto *nt = dynamic_cast<NumericTicker *>(m_ticker))
        nt->setDecimalPlaces(places);
}

int NumericAxis::decimalPlaces() const
{
    if (auto *nt = dynamic_cast<NumericTicker *>(m_ticker))
        return nt->decimalPlaces();
    return 0;
}

void NumericAxis::setLabelFormat(const QString &format)
{
    if (auto *nt = dynamic_cast<NumericTicker *>(m_ticker))
        nt->setLabelFormat(format);
}

QString NumericAxis::labelFormat() const
{
    if (auto *nt = dynamic_cast<NumericTicker *>(m_ticker))
        return nt->labelFormat();
    return {};
}

double NumericAxis::valueToPixel(const QVariant &value) const
{
    bool ok;
    double v = value.toDouble(&ok);
    if (!ok || m_cachePlotArea.isNull())
        return 0.0;

    double r = m_range.size();
    if (qFuzzyIsNull(r))
        return 0.0;

    if (m_orientation == Horizontal) {
        return m_cachePlotArea.left()
               + (v - m_range.lower()) / r * m_cachePlotArea.width();
    } else {
        return m_cachePlotArea.bottom()
               - (v - m_range.lower()) / r * m_cachePlotArea.height();
    }
}

QVariant NumericAxis::pixelToValue(double pixel) const
{
    if (m_cachePlotArea.isNull())
        return QVariant(0.0);

    double r = m_range.size();
    if (qFuzzyIsNull(r))
        return QVariant(m_range.lower());

    if (m_orientation == Horizontal) {
        return QVariant(m_range.lower() + (pixel - m_cachePlotArea.left())
                        / m_cachePlotArea.width() * r);
    } else {
        return QVariant(m_range.lower() + (m_cachePlotArea.bottom() - pixel)
                        / m_cachePlotArea.height() * r);
    }
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

    // Draw sub-ticks first (behind main ticks)
    drawSubTicks(painter, plotArea);

    painter->setPen(QPen(m_tickColor, 1.0));
    for (int i = 0; i < m_tickPixelPositions.size(); ++i) {
        double pos = m_tickPixelPositions[i];
        if (m_orientation == Horizontal) {
            if (pos < plotArea.left() || pos > plotArea.right()) continue;
            painter->drawLine(QPointF(pos, plotArea.bottom()),
                              QPointF(pos, plotArea.bottom() + m_tickLength));
        } else {
            if (pos < plotArea.top() || pos > plotArea.bottom()) continue;
            painter->drawLine(QPointF(plotArea.left() - m_tickLength, pos),
                              QPointF(plotArea.left(), pos));
        }
    }

    // Axis endings
    if (m_orientation == Horizontal) {
        drawAxisEnding(painter, m_lowerEnding,
                       QPointF(plotArea.left(), plotArea.bottom()), 0.0);
        drawAxisEnding(painter, m_upperEnding,
                       QPointF(plotArea.right(), plotArea.bottom()), 180.0);
    } else {
        drawAxisEnding(painter, m_lowerEnding,
                       QPointF(plotArea.left(), plotArea.bottom()), 90.0);
        drawAxisEnding(painter, m_upperEnding,
                       QPointF(plotArea.left(), plotArea.top()), -90.0);
    }

    if (m_showTickLabels) {
        painter->save();
        painter->setPen(QPen(m_labelColor, 1.0));
        painter->setFont(m_labelFont);
        if (!qFuzzyIsNull(m_tickLabelRotation)) {
            if (m_orientation == Horizontal) {
                painter->translate(plotArea.center().x(), plotArea.bottom() + m_tickLength + 2);
                painter->rotate(m_tickLabelRotation);
                painter->translate(-plotArea.center().x(), -(plotArea.bottom() + m_tickLength + 2));
            }
        }
        for (int i = 0; i < m_tickValues.size(); ++i) {
            double pos = m_tickPixelPositions[i];
            if (m_orientation == Horizontal) {
                if (pos < plotArea.left() || pos > plotArea.right()) continue;
            } else {
                if (pos < plotArea.top() || pos > plotArea.bottom()) continue;
            }
            QString label = m_ticker ? m_ticker->getTickLabel(m_tickValuesDouble[i]) : QString();
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
        painter->restore();
    }

    painter->restore();
}

void NumericAxis::zoomRange(double factor, const QVariant &center)
{
    double c = center.toDouble();
    double newMin = c - (c - m_range.lower()) / factor;
    double newMax = c + (m_range.upper() - c) / factor;
    if (newMin < newMax && (newMax - newMin) > 1e-10)
        setRange(newMin, newMax);
}

void NumericAxis::panRange(double pixelDelta, double plotAreaSize)
{
    if (qFuzzyIsNull(plotAreaSize))
        return;
    double shift = -pixelDelta / plotAreaSize * m_range.size();
    setRange(m_range.lower() + shift, m_range.upper() + shift);
}

// ============================================================================
// DateTimeAxis
// ============================================================================

DateTimeAxis::DateTimeAxis(AxisOrientation orientation, QObject *parent)
    : BaseAxis(orientation, parent)
{
    setTicker(new DateTimeTicker);
    qint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();
    m_range.set(double(now), double(now + 3600));
}

QDateTime DateTimeAxis::rangeMin() const { return QDateTime::fromSecsSinceEpoch(qint64(m_range.lower())); }
QDateTime DateTimeAxis::rangeMax() const { return QDateTime::fromSecsSinceEpoch(qint64(m_range.upper())); }
QVariant DateTimeAxis::tickValueToVariant(double v) const
{ return QVariant(QDateTime::fromSecsSinceEpoch(qint64(v))); }

void DateTimeAxis::setRange(const QDateTime &min, const QDateTime &max)
{
    qint64 lo = qMin(min, max).toSecsSinceEpoch();
    qint64 hi = qMax(min, max).toSecsSinceEpoch();
    if (lo == hi) hi = lo + 60;
    BaseAxis::setRange(double(lo), double(hi));
}

void DateTimeAxis::setDateTimeFormat(DateTimeFormat f)
{
    if (auto *dt = dynamic_cast<DateTimeTicker *>(m_ticker))
        dt->setDateTimeFormat(f);
}

DateTimeFormat DateTimeAxis::dateTimeFormat() const
{
    if (auto *dt = dynamic_cast<DateTimeTicker *>(m_ticker))
        return dt->dateTimeFormat();
    return DateTimeFormat::HHmm;
}

double DateTimeAxis::valueToPixel(const QVariant &value) const
{
    QDateTime dt = value.toDateTime();
    if (!dt.isValid() || m_cachePlotArea.isNull())
        return 0.0;

    double r = m_range.size();
    if (qFuzzyIsNull(r)) return 0.0;
    double v = static_cast<double>(dt.toSecsSinceEpoch());

    if (m_orientation == Horizontal) {
        return m_cachePlotArea.left()
               + (v - m_range.lower()) / r * m_cachePlotArea.width();
    } else {
        return m_cachePlotArea.bottom()
               - (v - m_range.lower()) / r * m_cachePlotArea.height();
    }
}

QVariant DateTimeAxis::pixelToValue(double pixel) const
{
    if (m_cachePlotArea.isNull())
        return QVariant();

    double r = m_range.size();
    if (qFuzzyIsNull(r))
        return QVariant(rangeMin());

    double fraction;
    if (m_orientation == Horizontal) {
        fraction = (pixel - m_cachePlotArea.left()) / m_cachePlotArea.width();
    } else {
        fraction = (m_cachePlotArea.bottom() - pixel) / m_cachePlotArea.height();
    }

    return QVariant(QDateTime::fromSecsSinceEpoch(
        qint64(m_range.lower() + fraction * r)));
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
        drawAxisEnding(painter, m_lowerEnding, QPointF(plotArea.left(), plotArea.bottom()), 0.0);
        drawAxisEnding(painter, m_upperEnding, QPointF(plotArea.right(), plotArea.bottom()), 180.0);
    } else {
        painter->drawLine(QPointF(plotArea.left(), plotArea.top()),
                          QPointF(plotArea.left(), plotArea.bottom()));
        drawAxisEnding(painter, m_lowerEnding, QPointF(plotArea.left(), plotArea.bottom()), 90.0);
        drawAxisEnding(painter, m_upperEnding, QPointF(plotArea.left(), plotArea.top()), -90.0);
    }

    // Draw sub-ticks
    drawSubTicks(painter, plotArea);

    painter->setPen(QPen(m_tickColor, 1.0));
    for (int i = 0; i < m_tickPixelPositions.size(); ++i) {
        double pos = m_tickPixelPositions[i];
        if (m_orientation == Horizontal) {
            if (pos < plotArea.left() || pos > plotArea.right()) continue;
            painter->drawLine(QPointF(pos, plotArea.bottom()),
                              QPointF(pos, plotArea.bottom() + m_tickLength));
        } else {
            if (pos < plotArea.top() || pos > plotArea.bottom()) continue;
            painter->drawLine(QPointF(plotArea.left() - m_tickLength, pos),
                              QPointF(plotArea.left(), pos));
        }
    }

    if (m_showTickLabels) {
        painter->save();
        painter->setPen(QPen(m_labelColor, 1.0));
        painter->setFont(m_labelFont);
        if (!qFuzzyIsNull(m_tickLabelRotation) && m_orientation == Horizontal) {
            painter->translate(plotArea.center().x(), plotArea.bottom() + m_tickLength + 2);
            painter->rotate(m_tickLabelRotation);
            painter->translate(-plotArea.center().x(), -(plotArea.bottom() + m_tickLength + 2));
        }
        double labelWidth = (m_orientation == Horizontal) ? 120.0 : 55.0;
        for (int i = 0; i < m_tickValues.size(); ++i) {
            double pos = m_tickPixelPositions[i];
            if (m_orientation == Horizontal) {
                if (pos < plotArea.left() || pos > plotArea.right()) continue;
            } else {
                if (pos < plotArea.top() || pos > plotArea.bottom()) continue;
            }
            QString label = m_ticker ? m_ticker->getTickLabel(m_tickValuesDouble[i]) : QString();
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
        painter->restore();
    }

    painter->restore();
}

void DateTimeAxis::zoomRange(double factor, const QVariant &center)
{
    QDateTime c = center.toDateTime();
    if (!c.isValid()) return;

    double centerSecs = static_cast<double>(c.toSecsSinceEpoch());
    double dMin = (centerSecs - m_range.lower()) / factor;
    double dMax = (m_range.upper() - centerSecs) / factor;

    qint64 newMin = qint64(centerSecs) - qMax(qint64(qRound(dMin)), qint64(0));
    qint64 newMax = qint64(centerSecs) + qMax(qint64(qRound(dMax)), qint64(0));
    if (newMin >= newMax) { newMin = qint64(centerSecs); newMax = newMin + 1; }

    setRange(QDateTime::fromSecsSinceEpoch(newMin),
             QDateTime::fromSecsSinceEpoch(newMax));
}

void DateTimeAxis::panRange(double pixelDelta, double plotAreaSize)
{
    if (qFuzzyIsNull(plotAreaSize)) return;
    m_panRemainder += -pixelDelta / plotAreaSize * m_range.size();
    qint64 shift = static_cast<qint64>(m_panRemainder);
    if (shift != 0) {
        m_panRemainder -= shift;
        double s = static_cast<double>(shift);
        setRange(QDateTime::fromSecsSinceEpoch(qint64(m_range.lower() + s)),
                 QDateTime::fromSecsSinceEpoch(qint64(m_range.upper() + s)));
    }
}

// ============================================================================
// DateAxis
// ============================================================================

DateAxis::DateAxis(AxisOrientation orientation, QObject *parent)
    : BaseAxis(orientation, parent)
{
    setTicker(new DateTicker);
    qint64 today = QDate::currentDate().toJulianDay();
    m_range.set(double(today), double(today + 30));
}

QDate DateAxis::rangeMin() const { return QDate::fromJulianDay(qint64(m_range.lower())); }
QDate DateAxis::rangeMax() const { return QDate::fromJulianDay(qint64(m_range.upper())); }
QVariant DateAxis::tickValueToVariant(double v) const
{ return QVariant(QDate::fromJulianDay(qint64(v))); }

void DateAxis::setRange(const QDate &min, const QDate &max)
{
    qint64 lo = qMin(min, max).toJulianDay();
    qint64 hi = qMax(min, max).toJulianDay();
    if (lo == hi) hi = lo + 1;
    BaseAxis::setRange(double(lo), double(hi));
}

void DateAxis::setDateTimeFormat(DateTimeFormat f)
{
    if (auto *dt = dynamic_cast<DateTicker *>(m_ticker))
        dt->setDateTimeFormat(f);
}

DateTimeFormat DateAxis::dateTimeFormat() const
{
    if (auto *dt = dynamic_cast<DateTicker *>(m_ticker))
        return dt->dateTimeFormat();
    return DateTimeFormat::yyyyMMdd;
}

double DateAxis::valueToPixel(const QVariant &value) const
{
    QDate d = value.toDate();
    if (!d.isValid() || m_cachePlotArea.isNull())
        return 0.0;

    double r = m_range.size();
    if (qFuzzyIsNull(r)) return 0.0;
    double v = static_cast<double>(d.toJulianDay());

    if (m_orientation == Horizontal) {
        return m_cachePlotArea.left()
               + (v - m_range.lower()) / r * m_cachePlotArea.width();
    } else {
        return m_cachePlotArea.bottom()
               - (v - m_range.lower()) / r * m_cachePlotArea.height();
    }
}

QVariant DateAxis::pixelToValue(double pixel) const
{
    if (m_cachePlotArea.isNull())
        return QVariant();

    double r = m_range.size();
    if (qFuzzyIsNull(r))
        return QVariant(rangeMin());

    double fraction;
    if (m_orientation == Horizontal) {
        fraction = (pixel - m_cachePlotArea.left()) / m_cachePlotArea.width();
    } else {
        fraction = (m_cachePlotArea.bottom() - pixel) / m_cachePlotArea.height();
    }

    return QVariant(QDate::fromJulianDay(qint64(m_range.lower() + fraction * r)));
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
        drawAxisEnding(painter, m_lowerEnding, QPointF(plotArea.left(), plotArea.bottom()), 0.0);
        drawAxisEnding(painter, m_upperEnding, QPointF(plotArea.right(), plotArea.bottom()), 180.0);
    } else {
        painter->drawLine(QPointF(plotArea.left(), plotArea.top()),
                          QPointF(plotArea.left(), plotArea.bottom()));
        drawAxisEnding(painter, m_lowerEnding, QPointF(plotArea.left(), plotArea.bottom()), 90.0);
        drawAxisEnding(painter, m_upperEnding, QPointF(plotArea.left(), plotArea.top()), -90.0);
    }

    // Draw sub-ticks
    drawSubTicks(painter, plotArea);

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
        painter->save();
        painter->setPen(QPen(m_labelColor, 1.0));
        painter->setFont(m_labelFont);
        if (!qFuzzyIsNull(m_tickLabelRotation) && m_orientation == Horizontal) {
            painter->translate(plotArea.center().x(), plotArea.bottom() + m_tickLength + 2);
            painter->rotate(m_tickLabelRotation);
            painter->translate(-plotArea.center().x(), -(plotArea.bottom() + m_tickLength + 2));
        }
        double labelWidth = (m_orientation == Horizontal) ? 120.0 : 60.0;
        for (int i = 0; i < m_tickValues.size(); ++i) {
            double pos = m_tickPixelPositions[i];
            if (m_orientation == Horizontal) {
                if (pos < plotArea.left() || pos > plotArea.right()) continue;
            } else {
                if (pos < plotArea.top() || pos > plotArea.bottom()) continue;
            }
            QString label = m_ticker ? m_ticker->getTickLabel(m_tickValuesDouble[i]) : QString();
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
        painter->restore(); // end rotation save
    }

    painter->restore();
}

void DateAxis::zoomRange(double factor, const QVariant &center)
{
    QDate c = center.toDate();
    if (!c.isValid()) return;

    double centerJD = static_cast<double>(c.toJulianDay());
    double dMin = (centerJD - m_range.lower()) / factor;
    double dMax = (m_range.upper() - centerJD) / factor;

    qint64 newMin = qint64(centerJD) - qMax(qint64(qRound(dMin)), qint64(0));
    qint64 newMax = qint64(centerJD) + qMax(qint64(qRound(dMax)), qint64(0));
    if (newMin >= newMax) { newMin = qint64(centerJD); newMax = newMin + 1; }

    setRange(QDate::fromJulianDay(newMin), QDate::fromJulianDay(newMax));
}

void DateAxis::panRange(double pixelDelta, double plotAreaSize)
{
    if (qFuzzyIsNull(plotAreaSize)) return;
    m_panRemainder += -pixelDelta / plotAreaSize * m_range.size();
    qint64 shift = static_cast<qint64>(m_panRemainder);
    if (shift != 0) {
        m_panRemainder -= shift;
        double s = static_cast<double>(shift);
        setRange(QDate::fromJulianDay(qint64(m_range.lower() + s)),
                 QDate::fromJulianDay(qint64(m_range.upper() + s)));
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
    painter->setClipRect(pa);  // prevent grid lines from leaking outside plot area
    QPen gridPen(m_gridLineColor, 1.0, Qt::DotLine);
    painter->setPen(gridPen);

    if (m_showGridY && m_yAxis) {
        const auto &positions = m_yAxis->tickPixelPositions();
        for (double pos : positions) {
            if (pos < pa.top() || pos > pa.bottom()) continue;
            painter->drawLine(QPointF(pa.left(), pos),
                              QPointF(pa.right(), pos));
        }
    }

    if (m_showGridX && m_xAxis) {
        const auto &positions = m_xAxis->tickPixelPositions();
        for (double pos : positions) {
            if (pos < pa.left() || pos > pa.right()) continue;
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

        // Short click (not a drag) → emit click signal
        if ((event->pos() - m_panLastPos).manhattanLength() < 5)
            emit chartClicked(event->pos());

        m_panAxisRect = nullptr;
        setCursor(Qt::ArrowCursor);
        invalidateBuffer();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
