#include "ChartWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFontMetrics>
#include <QStandardPaths>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>

static constexpr double kPi = 3.14159265358979323846;
static constexpr double kMarginRatio = 0.08;

// ========================================================================
//  Axis
// ========================================================================

Axis::Axis(AxisPosition pos) : m_position(pos) {}
Axis::~Axis() {}

AxisPosition Axis::position() const { return m_position; }
AxisType     Axis::type() const { return m_type; }
void Axis::setType(AxisType t) { m_type = t; }

void Axis::setRange(double lo, double hi) { m_autoRange = false; m_min = lo; m_max = hi; }
void Axis::setAutoRange(bool on) { m_autoRange = on; }
bool Axis::isAutoRange() const { return m_autoRange; }
double Axis::min() const { return m_min; }
double Axis::max() const { return m_max; }

void    Axis::setTitle(const QString &t) { m_title = t; }
QString Axis::title() const { return m_title; }
void  Axis::setLabelFont(const QFont &f) { m_labelFont = f; }
QFont Axis::labelFont() const { return m_labelFont; }
void  Axis::setTitleFont(const QFont &f) { m_titleFont = f; }
QFont Axis::titleFont() const { return m_titleFont; }

void Axis::setTickCount(int n) { m_tickCount = qMax(2, n); }
int  Axis::tickCount() const { return m_tickCount; }
void Axis::setTicksVisible(bool on) { m_ticksVisible = on; }
bool Axis::isTicksVisible() const { return m_ticksVisible; }
void    Axis::setTickColor(const QColor &c) { m_tickColor = c; }
QColor  Axis::tickColor() const { return m_tickColor; }
void Axis::setTickDirection(TickDirection d) { m_tickDirection = d; }
TickDirection Axis::tickDirection() const { return m_tickDirection; }

void Axis::setSubTickCount(int n) { m_subTickCount = qMax(0, n); }
int  Axis::subTickCount() const { return m_subTickCount; }
void Axis::setSubTicksVisible(bool on) { m_subTicksVisible = on; }
bool Axis::isSubTicksVisible() const { return m_subTicksVisible; }
void    Axis::setSubTickColor(const QColor &c) { m_subTickColor = c; }
QColor  Axis::subTickColor() const { return m_subTickColor; }
void Axis::setSubTickDirection(TickDirection d) { m_subTickDirection = d; }
TickDirection Axis::subTickDirection() const { return m_subTickDirection; }

void Axis::setGridVisible(bool on) { m_gridVisible = on; }
bool Axis::isGridVisible() const { return m_gridVisible; }
void Axis::setHorizontalGridVisible(bool on) { m_horizontalGridVisible = on; }
bool Axis::isHorizontalGridVisible() const { return m_horizontalGridVisible; }
void Axis::setVerticalGridVisible(bool on) { m_verticalGridVisible = on; }
bool Axis::isVerticalGridVisible() const { return m_verticalGridVisible; }
void    Axis::setGridColor(const QColor &c) { m_gridColor = c; }
QColor  Axis::gridColor() const { return m_gridColor; }

void Axis::setNotation(NumericNotation n) { m_notation = n; }
NumericNotation Axis::notation() const { return m_notation; }
void Axis::setFormatPrecision(int p) { m_precision = p; }
int  Axis::formatPrecision() const { return m_precision; }

void Axis::setDateTimeFormat(DateTimeFormat f) { m_dateTimeFmt = f; }
DateTimeFormat Axis::dateTimeFormat() const { return m_dateTimeFmt; }
void Axis::setDateFormat(DateFormat f) { m_dateFmt = f; }
DateFormat Axis::dateFormat() const { return m_dateFmt; }

// ========================================================================
//  Series
// ========================================================================

Series::Series(const QString &name, SeriesType type)
    : m_name(name), m_type(type) {}
Series::~Series() {}

QString    Series::name() const { return m_name; }
SeriesType Series::type() const { return m_type; }
void Series::setVisible(bool on) { m_visible = on; }
bool Series::isVisible() const { return m_visible; }
void   Series::setColor(const QColor &c) { m_color = c; }
QColor Series::color() const { return m_color; }

// --- LineSeries ---
LineSeries::LineSeries(const QString &name) : Series(name, SeriesType::Line) {}
void LineSeries::append(double x, double y) { m_data.append(DataPoint(x, y)); }
void LineSeries::append(const QDateTime &tx, double y) { m_data.append(DataPoint(tx, y)); }
void LineSeries::append(const DataPoint &p) { m_data.append(p); }
void LineSeries::append(const QVector<DataPoint> &pts) { m_data.append(pts); }
void LineSeries::removeAt(int i) { if (i >= 0 && i < m_data.size()) m_data.removeAt(i); }

void LineSeries::removeBefore(int count)
{
    int n = qMin(count, m_data.size());
    if (n > 0) m_data.erase(m_data.begin(), m_data.begin() + n);
}

void LineSeries::keepLast(int maxCount)
{
    if (m_data.size() > maxCount)
        m_data.erase(m_data.begin(), m_data.begin() + m_data.size() - maxCount);
}
void LineSeries::clear() { m_data.clear(); }
const QVector<DataPoint>& LineSeries::data() const { return m_data; }
int LineSeries::dataCount() const { return m_data.size(); }

void   LineSeries::setLineWidth(double w) { m_lineWidth = w; }
double LineSeries::lineWidth() const { return m_lineWidth; }
void   LineSeries::setMarkerSize(double s) { m_markerSize = s; }
double LineSeries::markerSize() const { return m_markerSize; }
void         LineSeries::setScatterStyle(ScatterStyle s) { m_scatterStyle = s; }
ScatterStyle LineSeries::scatterStyle() const { return m_scatterStyle; }
void   LineSeries::setFillBrush(const QBrush &b) { m_fillBrush = b; }
QBrush LineSeries::fillBrush() const { return m_fillBrush; }
void   LineSeries::setFillEnabled(bool on) { m_fillEnabled = on; }
bool   LineSeries::isFillEnabled() const { return m_fillEnabled; }

// --- BarSeries ---
BarSeries::BarSeries(const QString &name) : Series(name, SeriesType::Bar) {}
void BarSeries::append(double v) { m_data.append(v); m_useXY = false; }
void BarSeries::appendXY(double x, double y) { m_xyData.append({x, y}); m_useXY = true; }
void BarSeries::removeAt(int i) {
    if (m_useXY) { if (i>=0 && i<m_xyData.size()) m_xyData.removeAt(i); }
    else         { if (i>=0 && i<m_data.size())   m_data.removeAt(i); }
}
void BarSeries::clear() { m_data.clear(); m_xyData.clear(); }
const QVector<double>& BarSeries::data() const { return m_data; }
const QVector<BarSeries::XY>& BarSeries::xyData() const { return m_xyData; }
bool BarSeries::useXY() const { return m_useXY; }
int  BarSeries::dataCount() const { return m_useXY ? m_xyData.size() : m_data.size(); }
void   BarSeries::setBarWidthRatio(double r) { m_barWidthRatio = qBound(0.1, r, 1.0); }
double BarSeries::barWidthRatio() const { return m_barWidthRatio; }

// --- StackedBarSeries ---
StackedBarSeries::StackedBarSeries(const QString &name)
    : Series(name, SeriesType::StackedBar) {}
void StackedBarSeries::append(double v) { m_data.append(v); }
void StackedBarSeries::removeAt(int i) { if (i>=0 && i<m_data.size()) m_data.removeAt(i); }
void StackedBarSeries::clear() { m_data.clear(); }
const QVector<double>& StackedBarSeries::data() const { return m_data; }
int StackedBarSeries::dataCount() const { return m_data.size(); }
void   StackedBarSeries::setBarWidthRatio(double r) { m_barWidthRatio = qBound(0.1, r, 1.0); }
double StackedBarSeries::barWidthRatio() const { return m_barWidthRatio; }

// ========================================================================
//  ChartModel
// ========================================================================

ChartModel::ChartModel(QObject *parent) : QObject(parent) {}
ChartModel::~ChartModel()
{
    qDeleteAll(m_series);
    m_series.clear();
    delete m_axisX; m_axisX = nullptr;
    delete m_axisY; m_axisY = nullptr;
}

void ChartModel::addAxis(Axis *axis)
{
    if (!axis) return;
    if (axis->position() == AxisPosition::Bottom) {
        delete m_axisX; m_axisX = axis;
    } else {
        delete m_axisY; m_axisY = axis;
    }
    emit axisAdded(axis);
    emit dataChanged();
}

void ChartModel::removeAxis(Axis *axis)
{
    if (axis == m_axisX) m_axisX = nullptr;
    if (axis == m_axisY) m_axisY = nullptr;
    emit axisRemoved(axis);
    emit dataChanged();
}

QList<Axis*> ChartModel::axes() const {
    QList<Axis*> a;
    if (m_axisX) a << m_axisX;
    if (m_axisY) a << m_axisY;
    return a;
}
Axis* ChartModel::axisX() const { return m_axisX; }
Axis* ChartModel::axisY() const { return m_axisY; }

void ChartModel::addSeries(Series *s)
{
    if (!s) return;
    if (!s->color().isValid()) s->setColor(nextColor());
    m_series.append(s);
    emit seriesAdded(s);
    emit dataChanged();
}

void ChartModel::removeSeries(Series *s)
{
    if (m_series.removeOne(s)) { emit seriesRemoved(s); emit dataChanged(); }
}

QList<Series*> ChartModel::seriesList() const { return m_series; }

void ChartModel::setCategories(const QStringList &c)
{ m_categories = c; emit categoriesChanged(); emit dataChanged(); }
QStringList ChartModel::categories() const { return m_categories; }

void ChartModel::setTitle(const QString &t)
{ m_title = t; emit titleChanged(); emit dataChanged(); }
QString ChartModel::title() const { return m_title; }

void ChartModel::setTheme(const ChartTheme &theme)
{ m_theme = theme; emit themeChanged(); emit dataChanged(); }
ChartTheme ChartModel::theme() const { return m_theme; }

QColor ChartModel::nextColor()
{
    const auto &pal = m_theme.seriesColors;
    if (pal.isEmpty()) return QColor(128, 128, 128);
    return pal[(m_colorIndex++) % pal.size()];
}

// ========================================================================
//  ChartLayout
// ========================================================================

ChartLayout::ChartLayout(ChartModel *model, QObject *parent)
    : QObject(parent), m_model(model)
{
    connect(m_model, &ChartModel::dataChanged, this, &ChartLayout::invalidate);
}

void ChartLayout::setOutsideLegendHeight(double h)
{
    m_outsideLegendHeight = h;
}

void ChartLayout::invalidate() { m_dirty = true; }
QRectF ChartLayout::plotArea() const { return m_plotArea; }
double ChartLayout::xMin() const { return m_xMin; }
double ChartLayout::xMax() const { return m_xMax; }
double ChartLayout::yMin() const { return m_yMin; }
double ChartLayout::yMax() const { return m_yMax; }
QVector<double> ChartLayout::xTicks() const { return m_xTicks; }
QVector<double> ChartLayout::yTicks() const { return m_yTicks; }

void ChartLayout::recalculate(int w, int h)
{
    if (!m_dirty) return;
    computeXRange();
    computeYRange();
    computeTicks();
    computeLayout(w, h);
    m_dirty = false;
}

void ChartLayout::computeXRange()
{
    Axis *ax = m_model->axisX();
    if (!ax) return;
    if (!ax->isAutoRange()) { m_xMin = ax->min(); m_xMax = ax->max(); return; }

    QStringList cats = m_model->categories();
    if (!cats.isEmpty()) {
        m_xMin = 0; m_xMax = qMax(1, cats.size() - 1);
        ax->setRange(m_xMin, m_xMax); return;
    }

    double lo = std::numeric_limits<double>::max();
    double hi = -lo;
    bool found = false;

    for (Series *s : m_model->seriesList()) {
        if (!s->isVisible()) continue;
        if (s->type() == SeriesType::Line) {
            for (const auto &pt : static_cast<LineSeries*>(s)->data()) {
                lo = qMin(lo, pt.x); hi = qMax(hi, pt.x); found = true;
            }
        } else if (s->type() == SeriesType::Bar) {
            auto *bs = static_cast<BarSeries*>(s);
            if (bs->useXY()) {
                for (const auto &xy : bs->xyData()) {
                    lo = qMin(lo, xy.x); hi = qMax(hi, xy.x); found = true;
                }
            }
        }
    }
    if (!found) { lo = 0; hi = 100; }
    if (qFuzzyCompare(lo, hi)) { lo -= 1; hi += 1; }
    ax->setRange(lo, hi);
    m_xMin = lo; m_xMax = hi;
}

void ChartLayout::computeYRange()
{
    Axis *ay = m_model->axisY();
    if (!ay) return;
    if (!ay->isAutoRange()) { m_yMin = ay->min(); m_yMax = ay->max(); return; }

    double lo = std::numeric_limits<double>::max();
    double hi = -lo;
    bool found = false;

    for (Series *s : m_model->seriesList()) {
        if (!s->isVisible()) continue;
        if (s->type() == SeriesType::Line) {
            for (const auto &pt : static_cast<LineSeries*>(s)->data()) {
                lo = qMin(lo, pt.y); hi = qMax(hi, pt.y); found = true;
            }
        } else if (s->type() == SeriesType::Bar) {
            auto *bs = static_cast<BarSeries*>(s);
            if (bs->useXY()) {
                for (const auto &xy : bs->xyData()) {
                    lo = qMin(lo, xy.y); hi = qMax(hi, xy.y); found = true;
                }
            } else {
                for (double v : bs->data()) { lo = qMin(lo, v); hi = qMax(hi, v); found = true; }
            }
        }
    }

    QList<StackedBarSeries*> stacked;
    for (Series *s : m_model->seriesList()) {
        if (s->isVisible() && s->type() == SeriesType::StackedBar)
            stacked.append(static_cast<StackedBarSeries*>(s));
    }
    if (!stacked.isEmpty()) {
        int maxCat = 0;
        for (auto *sb : stacked) maxCat = qMax(maxCat, sb->dataCount());
        for (int i = 0; i < maxCat; ++i) {
            double sp = 0, sn = 0;
            for (auto *sb : stacked) {
                if (i < sb->dataCount()) {
                    double v = sb->data().at(i);
                    if (v >= 0) sp += v; else sn += v;
                }
            }
            lo = qMin(lo, qMin(0.0, sn));
            hi = qMax(hi, qMax(0.0, sp));
            found = true;
        }
    }

    if (!found) { lo = 0; hi = 100; }
    if (qFuzzyCompare(lo, hi)) hi += 1;
    double margin = (hi - lo) * kMarginRatio;
    lo -= margin; hi += margin;
    if (lo > 0) lo = 0;
    ay->setRange(lo, hi);
    m_yMin = lo; m_yMax = hi;
}

void ChartLayout::computeTicks()
{
    Axis *ax = m_model->axisX();
    Axis *ay = m_model->axisY();
    if (ay) m_yTicks = computeNumericTicks(ay);

    QStringList cats = m_model->categories();
    if (!cats.isEmpty()) {
        m_xTicks.clear();
        for (int i = 0; i < cats.size(); ++i) m_xTicks.append(double(i));
    } else if (ax) {
        switch (ax->type()) {
        case AxisType::Numeric:  m_xTicks = computeNumericTicks(ax);   break;
        case AxisType::DateTime: m_xTicks = computeDateTimeTicks(ax);  break;
        case AxisType::Date:     m_xTicks = computeDateTicks(ax);      break;
        }
    }
}

QVector<double> ChartLayout::computeNumericTicks(Axis *axis) const
{
    QVector<double> ticks;
    double range = axis->max() - axis->min();
    if (range <= 0) return ticks;
    double step = niceStep(range, axis->tickCount());
    double v = std::ceil(axis->min() / step) * step;
    double eps = step * 1e-6;
    while (v <= axis->max() + eps) {
        if (v >= axis->min() - eps) ticks.append(v);
        v += step;
    }
    return ticks;
}

QVector<double> ChartLayout::computeDateTimeTicks(Axis *axis) const
{
    QVector<double> ticks;
    double range = axis->max() - axis->min();
    if (range <= 0) return ticks;
    static const qint64 intervals[] = {
        1,2,5,10,15,30,60,120,300,600,900,1800,
        3600,7200,14400,21600,43200,86400
    };
    int bestIdx = 17;
    for (int i = 0; i < 18; ++i) {
        if (range / double(intervals[i]) <= double(axis->tickCount()) * 1.5) { bestIdx = i; break; }
    }
    qint64 step = intervals[bestIdx];
    qint64 lo = qint64(axis->min()), hi = qint64(axis->max());
    qint64 start = (lo / step) * step;
    if (start < lo) start += step;
    for (qint64 t = start; t <= hi; t += step) ticks.append(double(t));
    return ticks;
}

QVector<double> ChartLayout::computeDateTicks(Axis *axis) const
{
    QVector<double> ticks;
    double range = axis->max() - axis->min();
    if (range <= 0) return ticks;
    qint64 days = qint64(range) / 86400;
    int stepDays = (days<=14)?1 : (days<=60)?3 : (days<=180)?7 : (days<=730)?30 : 90;
    qint64 stepSecs = qint64(stepDays) * 86400;
    qint64 lo = qint64(axis->min()), hi = qint64(axis->max());
    QDateTime startDt = QDateTime::fromSecsSinceEpoch(lo).date().startOfDay();
    qint64 t = qint64(startDt.toSecsSinceEpoch());
    if (t < lo) t += stepSecs;
    while (t <= hi) { ticks.append(double(t)); t += stepSecs; }
    return ticks;
}

void ChartLayout::computeLayout(int w, int h)
{
    Axis *ax = m_model->axisX();
    Axis *ay = m_model->axisY();
    const ChartTheme &theme = m_model->theme();

    QFontMetrics lfm(ay ? ay->labelFont() : theme.labelFont);
    QFontMetrics tfm(ax ? ax->titleFont() : theme.axisTitleFont);

    double left = m_margin, top = m_margin, right = m_margin, bottom = m_margin;

    if (m_outsideLegendHeight > 0) {
        top += m_outsideLegendHeight;
    }

    if (!m_model->title().isEmpty()) {
        QFontMetrics fm(theme.titleFont);
        top += fm.height() + 4;
    }
    if (ay) {
        double maxW = 0;
        for (double v : m_yTicks)
            maxW = qMax(maxW, double(lfm.width(formatAxisValue(ay, v))));
        left += maxW + 10;
        if (!ay->title().isEmpty()) left += tfm.height() + 4;
    }
    if (ax) {
        bottom += lfm.height() + 8;
        if (!ax->title().isEmpty()) bottom += tfm.height() + 4;
    }
    m_plotArea = QRectF(left, top, qMax(1.0, w-left-right), qMax(1.0, h-top-bottom));
}

QPointF ChartLayout::mapToPixel(double dx, double dy) const
{
    double xr = qFuzzyIsNull(m_xMax-m_xMin) ? 1.0 : m_xMax-m_xMin;
    double yr = qFuzzyIsNull(m_yMax-m_yMin) ? 1.0 : m_yMax-m_yMin;
    return QPointF(m_plotArea.left() + (dx-m_xMin)/xr*m_plotArea.width(),
                   m_plotArea.bottom() - (dy-m_yMin)/yr*m_plotArea.height());
}

double ChartLayout::pixelXToData(double px) const {
    double xr = m_xMax-m_xMin;
    return qFuzzyIsNull(xr) ? m_xMin : m_xMin + (px-m_plotArea.left())/m_plotArea.width()*xr;
}
double ChartLayout::pixelYToData(double py) const {
    double yr = m_yMax-m_yMin;
    return qFuzzyIsNull(yr) ? m_yMin : m_yMin + (m_plotArea.bottom()-py)/m_plotArea.height()*yr;
}

QString ChartLayout::formatAxisValue(Axis *axis, double value) const
{
    if (!axis) return QString::number(value, 'f', 2);
    if (axis->position() == AxisPosition::Bottom && !m_model->categories().isEmpty()) {
        int idx = int(value + 0.5);
        return m_model->categories().value(idx, QString::number(idx));
    }
    switch (axis->type()) {
    case AxisType::Numeric:  return formatNumericValue(axis, value);
    case AxisType::DateTime: return formatDateTimeValue(value, axis->dateTimeFormat());
    case AxisType::Date:     return formatDateValue(value, axis->dateFormat());
    }
    return QString::number(value, 'f', 2);
}

QString ChartLayout::formatNumericValue(Axis *axis, double v) const
{
    if (axis->notation() == NumericNotation::Scientific)
        return QString::number(v, 'e', qMax(1, axis->formatPrecision()<0 ? 2 : axis->formatPrecision()));
    int prec = axis->formatPrecision();
    if (prec < 0) prec = decimalsForStep(niceStep(axis->max()-axis->min(), axis->tickCount()));
    return QString::number(v, 'f', prec);
}

QString ChartLayout::formatDateTimeValue(double v, DateTimeFormat fmt) const
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(qint64(v));
    switch (fmt) {
    case DateTimeFormat::HHmmss:    return dt.toString("HH:mm:ss");
    case DateTimeFormat::HHmm:      return dt.toString("HH:mm");
    case DateTimeFormat::HHmmsszzz: return dt.toString("HH:mm:ss.zzz");
    }
    return dt.toString("HH:mm:ss");
}

QString ChartLayout::formatDateValue(double v, DateFormat fmt) const
{
    QDate d = QDateTime::fromSecsSinceEpoch(qint64(v)).date();
    switch (fmt) {
    case DateFormat::yyyyMMdd:   return d.toString("yyyy-MM-dd");
    case DateFormat::yyyy_MM_dd: return d.toString("yyyy/MM/dd");
    case DateFormat::MMdd:       return d.toString("MM-dd");
    case DateFormat::yyyyMM:     return d.toString("yyyy-MM");
    }
    return d.toString("yyyy-MM-dd");
}

double ChartLayout::niceStep(double range, int tickCount)
{
    if (range <= 0 || tickCount < 2) return 1.0;
    double rough = range / double(tickCount-1);
    double mag = std::pow(10.0, std::floor(std::log10(rough)));
    double norm = rough / mag;
    double nice = (norm<1.5)?1.0 : (norm<3.5)?2.0 : (norm<7.5)?5.0 : 10.0;
    return nice * mag;
}

int ChartLayout::decimalsForStep(double step)
{
    if (step >= 1.0) return 0;
    int d = 0; double s = step;
    while (s < 1.0 && d < 10) { s *= 10.0; ++d; }
    return d;
}

// ========================================================================
//  ChartRenderer
// ========================================================================

ChartRenderer::ChartRenderer(ChartModel *model, ChartLayout *layout)
    : m_model(model), m_layout(layout) {}

void ChartRenderer::render(QPainter &p, int w, int h)
{
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    drawBackground(p, w, h);
    drawPlotBackground(p);
    drawGrid(p);
    drawSeries(p);
    drawAxes(p);
    drawTitle(p);
}

void ChartRenderer::drawBackground(QPainter &p, int w, int h)
{
    p.fillRect(0, 0, w, h, m_model->theme().background);
}

void ChartRenderer::drawPlotBackground(QPainter &p)
{
    QRectF pa = m_layout->plotArea();
    p.fillRect(pa, m_model->theme().plotBackground);
    p.setPen(QPen(QColor(200,200,200), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(pa);
}

void ChartRenderer::drawGrid(QPainter &p)
{
    Axis *ax = m_model->axisX(), *ay = m_model->axisY();
    if (!ax || !ay) return;
    QRectF pa = m_layout->plotArea();
    const ChartTheme &t = m_model->theme();

    if (ay->isGridVisible() && ay->isHorizontalGridVisible()) {
        p.setPen(QPen(t.gridColor, t.gridWidth, t.gridStyle));
        for (double v : m_layout->yTicks()) {
            QPointF pt = m_layout->mapToPixel(m_layout->xMin(), v);
            if (pt.y() >= pa.top() && pt.y() <= pa.bottom())
                p.drawLine(QPointF(pa.left(), pt.y()), QPointF(pa.right(), pt.y()));
        }
    }
    if (ax->isGridVisible() && ax->isVerticalGridVisible()) {
        p.setPen(QPen(t.gridColor, t.gridWidth, t.gridStyle));
        for (double v : m_layout->xTicks()) {
            QPointF pt = m_layout->mapToPixel(v, m_layout->yMin());
            if (pt.x() >= pa.left() && pt.x() <= pa.right())
                p.drawLine(QPointF(pt.x(), pa.top()), QPointF(pt.x(), pa.bottom()));
        }
    }
}

void ChartRenderer::drawSeries(QPainter &p)
{
    drawStackedBar(p);
    for (Series *s : m_model->seriesList()) {
        if (s->isVisible() && s->type() == SeriesType::Bar)
            drawBar(p, static_cast<BarSeries*>(s));
    }
    for (Series *s : m_model->seriesList()) {
        if (s->isVisible() && s->type() == SeriesType::Line)
            drawLine(p, static_cast<LineSeries*>(s));
    }
}

void ChartRenderer::drawLine(QPainter &p, LineSeries *s)
{
    if (!s || s->dataCount() < 1) return;
    const auto &data = s->data();
    QRectF pa = m_layout->plotArea();
    QPen pen(s->color(), s->lineWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    p.save();
    p.setClipRect(pa.adjusted(-1, -1, 1, 1));

    // ========== 填充区域（修复渐变映射）==========
    if (s->isFillEnabled() && data.size() >= 2) {
        QPolygonF fp;
        for (const auto &pt : data)
            fp << m_layout->mapToPixel(pt.x, pt.y);
        fp << m_layout->mapToPixel(data.last().x, m_layout->yMin());
        fp << m_layout->mapToPixel(data.first().x, m_layout->yMin());
        fp << m_layout->mapToPixel(data.first().x, data.first().y);

        // 根据多边形实际边界重新映射渐变
        QRectF polyBounds = fp.boundingRect();

        // 从用户的 fillBrush 中提取渐变信息，重新映射到实际区域
        QBrush originalBrush = s->fillBrush();
        QBrush mappedBrush;

        if (originalBrush.style() == Qt::LinearGradientPattern ||
            originalBrush.style() == Qt::RadialGradientPattern ||
            originalBrush.style() == Qt::ConicalGradientPattern) {

            const QGradient *origGrad = originalBrush.gradient();

            // 创建新的线性渐变，从填充区域顶部到底部
            QLinearGradient mappedGrad(polyBounds.topLeft(), polyBounds.bottomLeft());

            // 复制原始渐变的所有色标
            QGradientStops stops = origGrad->stops();
            for (const auto &stop : stops) {
                mappedGrad.setColorAt(stop.first, stop.second);
            }

            // 复制渐变属性
            mappedGrad.setSpread(origGrad->spread());
            mappedBrush = QBrush(mappedGrad);
        } else {
            // 非渐变画刷（纯色等），直接使用
            mappedBrush = originalBrush;
        }

        p.setBrush(mappedBrush);
        p.setPen(Qt::NoPen);
        p.drawPolygon(fp);
    }

    // ========== 折线（不变）==========
    if (data.size() >= 2) {
        QPolygonF poly;
        for (const auto &pt : data)
            poly << m_layout->mapToPixel(pt.x, pt.y);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPolyline(poly);
    }

    // ========== 散点（不变）==========
    ScatterStyle ss = s->scatterStyle();
    double ms = s->markerSize();
    if (ss != ScatterStyle::None && ms > 0) {
        p.setPen(Qt::NoPen);
        if (ss == ScatterStyle::CustomPixmap && !s->pixmap().isNull()) {
            // ===== 新增：绘制自定义 pixmap =====
            QPixmap pm = s->pixmap();
            double scale = ms / 5.0;  // 以 markerSize=5 为原始大小
            QPixmap scaled = pm.scaled(int(pm.width() * scale),
                                       int(pm.height() * scale),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
            double hw = scaled.width() / 2.0;
            double hh = scaled.height() / 2.0;
            for (const auto &pt : data) {
                QPointF pos = m_layout->mapToPixel(pt.x, pt.y);
                p.drawPixmap(int(pos.x() - hw), int(pos.y() - hh), scaled);
            }
        } else {
            // 原有散点绘制
            for (const auto &pt : data)
                drawScatter(p, ss, m_layout->mapToPixel(pt.x, pt.y), ms, s->color());
        }
    }

    p.restore();
}

void ChartRenderer::drawBar(QPainter &p, BarSeries *s)
{
    if (!s || s->dataCount() == 0) return;
    QRectF pa = m_layout->plotArea();
    p.save();
    p.setClipRect(pa.adjusted(-1,-1,1,1));

    if (s->useXY()) {
        double barW = pa.width() * 0.03 * s->barWidthRatio();
        for (int i = 0; i < s->xyData().size(); ++i) {
            const auto &xy = s->xyData().at(i);
            QPointF top = m_layout->mapToPixel(xy.x, xy.y);
            QPointF bot = m_layout->mapToPixel(xy.x, qMax(m_layout->yMin(), 0.0));
            QRectF r(top.x()-barW/2.0, top.y(), barW, bot.y()-top.y());
            p.fillRect(r, s->color());
            p.setPen(s->color().darker(120));
            p.drawRect(r);
        }
    } else {
        const auto &vals = s->data();
        int nCat = m_model->categories().isEmpty() ? vals.size() : m_model->categories().size();
        if (nCat == 0) { p.restore(); return; }

        QList<BarSeries*> bars;
        for (Series *ss : m_model->seriesList())
            if (ss->isVisible() && ss->type() == SeriesType::Bar)
                bars.append(static_cast<BarSeries*>(ss));
        int totalBars = bars.size(), barIdx = qMax(0, bars.indexOf(s));
        double catLeftPx  = m_layout->mapToPixel(-0.5, 0).x();
        double catRightPx = m_layout->mapToPixel(0.5, 0).x();
        double catWidthPx = catRightPx - catLeftPx;
        double groupW  = catWidthPx * s->barWidthRatio();
        double singleW = groupW / qMax(1, totalBars);

        for (int i = 0; i < vals.size(); ++i) {
            // 用 mapToPixel 计算分类中心的像素 X 坐标
            QPointF centerPt = m_layout->mapToPixel(double(i), 0);
            double cx = centerPt.x();

            // 超出绘图区域则跳过
            if (cx < pa.left() - groupW || cx > pa.right() + groupW) continue;

            double barLeft = cx - groupW / 2.0 + barIdx * singleW;

            // 用 mapToPixel 计算柱顶和柱底的像素 Y 坐标
            double yClamped = qBound(m_layout->yMin(), vals[i], m_layout->yMax());
            QPointF topPt = m_layout->mapToPixel(0, vals[i]);
            QPointF botPt = m_layout->mapToPixel(0, qMax(m_layout->yMin(), 0.0));

            double barTop = qMin(topPt.y(), botPt.y());
            double barH   = qAbs(botPt.y() - topPt.y());
            if (barH < 1.0) barH = 1.0;

            QRectF r(barLeft, barTop, singleW - 1, barH);
            p.fillRect(r, s->color());
            p.setPen(s->color().darker(120));
            p.drawRect(r);
        }
    }
    p.restore();
}

void ChartRenderer::drawStackedBar(QPainter &p)
{
    QList<StackedBarSeries*> stacked;
    for (Series *s : m_model->seriesList())
        if (s->isVisible() && s->type() == SeriesType::StackedBar)
            stacked.append(static_cast<StackedBarSeries*>(s));
    if (stacked.isEmpty()) return;

    int nCat = 0;
    for (auto *sb : stacked) nCat = qMax(nCat, sb->dataCount());
    if (nCat == 0) return;

    QRectF pa = m_layout->plotArea();
    double catLeftPx  = m_layout->mapToPixel(-0.5, 0).x();
    double catRightPx = m_layout->mapToPixel(0.5, 0).x();
    double catWidthPx = catRightPx - catLeftPx;
    double barW = catWidthPx * stacked.first()->barWidthRatio();

    p.save();
    p.setClipRect(pa.adjusted(-1,-1,1,1));

    for (int i = 0; i < nCat; ++i) {
        // 用 mapToPixel 计算分类中心的像素 X 坐标
        QPointF centerPt = m_layout->mapToPixel(double(i), 0);
        double cx = centerPt.x();

        // 超出绘图区域则跳过
        if (cx < pa.left() - barW || cx > pa.right() + barW) continue;

        double barLeft = cx - barW / 2.0;
        double cumNeg = 0, cumPos = 0;
        for (auto *sb : stacked) {
            double val = (i < sb->dataCount()) ? sb->data().at(i) : 0;
            double bv, tv;
            if (val >= 0) { bv = cumPos; tv = cumPos+val; cumPos = tv; }
            else          { bv = cumNeg+val; tv = cumNeg; cumNeg = bv; }
            // 用 mapToPixel 计算像素 Y 坐标
            QPointF tp = m_layout->mapToPixel(0, tv);
            QPointF bp = m_layout->mapToPixel(0, bv);
            QRectF r(barLeft, tp.y(), barW, bp.y()-tp.y());
            p.fillRect(r, sb->color());
            p.setPen(sb->color().darker(120));
            p.drawRect(r);
        }
    }
    p.restore();
}

void ChartRenderer::drawScatter(QPainter &p, ScatterStyle style,
                                const QPointF &center, double size, const QColor &color)
{
    if (style == ScatterStyle::None || size <= 0) return;
    const double s = size, cx = center.x(), cy = center.y();
    p.setPen(Qt::NoPen); p.setBrush(color);

    switch (style) {
    case ScatterStyle::Circle:
        p.drawEllipse(center, s, s);
        p.setBrush(QColor(255,255,255));
        p.drawEllipse(center, s*0.45, s*0.45);
        break;
    case ScatterStyle::Dot:
        p.drawEllipse(center, s*0.6, s*0.6);
        break;
    case ScatterStyle::Square:
        p.drawRect(QRectF(cx-s, cy-s, s*2, s*2));
        p.setBrush(QColor(255,255,255));
        p.drawRect(QRectF(cx-s*0.4, cy-s*0.4, s*0.8, s*0.8));
        break;
    case ScatterStyle::Diamond: {
        QPolygonF poly;
        poly << QPointF(cx, cy-s*1.15) << QPointF(cx+s*0.85, cy)
             << QPointF(cx, cy+s*1.15) << QPointF(cx-s*0.85, cy);
        p.drawPolygon(poly); break;
    }
    case ScatterStyle::Triangle: {
        QPolygonF poly;
        poly << QPointF(cx, cy-s*1.1) << QPointF(cx+s*0.95, cy+s*0.65)
             << QPointF(cx-s*0.95, cy+s*0.65);
        p.drawPolygon(poly); break;
    }
    case ScatterStyle::Cross: {
        QPen pen(color, s*0.7, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawLine(QPointF(cx-s*0.7, cy-s*0.7), QPointF(cx+s*0.7, cy+s*0.7));
        p.drawLine(QPointF(cx+s*0.7, cy-s*0.7), QPointF(cx-s*0.7, cy+s*0.7));
        break;
    }
    case ScatterStyle::Plus: {
        QPen pen(color, s*0.7, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawLine(QPointF(cx-s*0.7, cy), QPointF(cx+s*0.7, cy));
        p.drawLine(QPointF(cx, cy-s*0.7), QPointF(cx, cy+s*0.7));
        break;
    }
    case ScatterStyle::Star: {
        QPolygonF poly;
        for (int i = 0; i < 5; ++i) {
            double a1 = -kPi/2.0 + i*2.0*kPi/5.0;
            double a2 = a1 + kPi/5.0;
            poly << QPointF(cx+s*1.1*std::cos(a1), cy+s*1.1*std::sin(a1));
            poly << QPointF(cx+s*0.45*std::cos(a2), cy+s*0.45*std::sin(a2));
        }
        p.drawPolygon(poly); break;
    }
    default: break;
    }
}

void ChartRenderer::drawAxes(QPainter &p)
{
    Axis *ax = m_model->axisX(), *ay = m_model->axisY();
    if (!ax || !ay) return;
    const ChartTheme &t = m_model->theme();
    QRectF pa = m_layout->plotArea();
    QFontMetrics lfm(ay->labelFont());
    QFontMetrics tfm(ax->titleFont());

    // ---- Y 轴 ----
    QVector<double> yt = m_layout->yTicks();
    if (ay->isTicksVisible()) {
        p.setFont(ay->labelFont());
        for (double v : yt) {
            QPointF pt = m_layout->mapToPixel(m_layout->xMin(), v);
            if (pt.y() < pa.top()-1 || pt.y() > pa.bottom()+1) continue;
            QString txt = m_layout->formatAxisValue(ay, v);
            p.setPen(t.textColor);
            p.drawText(QPointF(pa.left()-lfm.width(txt)-6, pt.y()+lfm.ascent()/2.0-1), txt);
            p.setPen(QPen(ay->tickColor(), 1));
            bool in = (ay->tickDirection() == TickDirection::Inside);
            if (in) p.drawLine(QPointF(pa.left(), pt.y()), QPointF(pa.left()+4, pt.y()));
            else    p.drawLine(QPointF(pa.left()-4, pt.y()), QPointF(pa.left(), pt.y()));
        }
    }
    if (ay->isSubTicksVisible() && ay->subTickCount()>0 && yt.size()>=2) {
        p.setPen(QPen(ay->subTickColor(), 1));
        bool in = (ay->subTickDirection() == TickDirection::Inside);
        for (int i = 0; i < yt.size()-1; ++i) {
            double step = (yt[i+1]-yt[i]) / double(ay->subTickCount()+1);
            for (int j = 1; j <= ay->subTickCount(); ++j) {
                QPointF pt = m_layout->mapToPixel(m_layout->xMin(), yt[i]+step*j);
                if (pt.y()<pa.top() || pt.y()>pa.bottom()) continue;
                if (in) p.drawLine(QPointF(pa.left(), pt.y()), QPointF(pa.left()+2.5, pt.y()));
                else    p.drawLine(QPointF(pa.left()-2.5, pt.y()), QPointF(pa.left(), pt.y()));
            }
        }
    }
    if (!ay->title().isEmpty()) {
        p.save(); p.setFont(ay->titleFont()); p.setPen(t.axisTitleColor);
        p.translate(QPointF(pa.left()-40, pa.center().y())); p.rotate(-90);
        QFontMetrics fm(ay->titleFont());
        p.drawText(QPointF(-fm.width(ay->title())/2.0, 0), ay->title());
        p.restore();
    }

    // ---- X 轴 ----
    QVector<double> xt = m_layout->xTicks();
    if (ax->isTicksVisible()) {
        p.setFont(ax->labelFont());
        for (double v : xt) {
            QPointF pt = m_layout->mapToPixel(v, m_layout->yMin());
            if (pt.x() < pa.left()-1 || pt.x() > pa.right()+1) continue;
            QString txt = m_layout->formatAxisValue(ax, v);
            p.setPen(t.textColor);
            p.drawText(QPointF(pt.x()-lfm.width(txt)/2.0, pa.bottom()+lfm.ascent()+4), txt);
            p.setPen(QPen(ax->tickColor(), 1));
            bool in = (ax->tickDirection() == TickDirection::Inside);
            if (in) p.drawLine(QPointF(pt.x(), pa.bottom()), QPointF(pt.x(), pa.bottom()-4));
            else    p.drawLine(QPointF(pt.x(), pa.bottom()), QPointF(pt.x(), pa.bottom()+4));
        }
    }
    if (ax->isSubTicksVisible() && ax->subTickCount()>0 && xt.size()>=2) {
        p.setPen(QPen(ax->subTickColor(), 1));
        bool in = (ax->subTickDirection() == TickDirection::Inside);
        for (int i = 0; i < xt.size()-1; ++i) {
            double step = (xt[i+1]-xt[i]) / double(ax->subTickCount()+1);
            for (int j = 1; j <= ax->subTickCount(); ++j) {
                QPointF pt = m_layout->mapToPixel(xt[i]+step*j, m_layout->yMin());
                if (pt.x()<pa.left() || pt.x()>pa.right()) continue;
                if (in) p.drawLine(QPointF(pt.x(), pa.bottom()), QPointF(pt.x(), pa.bottom()-2.5));
                else    p.drawLine(QPointF(pt.x(), pa.bottom()), QPointF(pt.x(), pa.bottom()+2.5));
            }
        }
    }
    if (!ax->title().isEmpty()) {
        p.setFont(ax->titleFont()); p.setPen(t.axisTitleColor);
        double tw = tfm.width(ax->title());
        p.drawText(QPointF(pa.center().x()-tw/2.0, pa.bottom()+lfm.ascent()+tfm.height()+8), ax->title());
    }
}

void ChartRenderer::drawTitle(QPainter &p)
{
    QString title = m_model->title();
    if (title.isEmpty()) return;
    const ChartTheme &t = m_model->theme();
    p.setFont(t.titleFont); p.setPen(t.titleColor);
    QFontMetrics fm(t.titleFont);
    QRectF pa = m_layout->plotArea();
    p.drawText(QPointF(pa.center().x()-fm.width(title)/2.0, pa.top()-8), title);
}

void ChartRenderer::drawLegend(QPainter &p, const QRectF &plotArea,
                               const QList<Series*> &visible, int pos, int ori)
{
    if (visible.isEmpty()) return;
    if (pos == 3 /* Hidden */ || pos == 4 /* OutsideTop */) return;

    const ChartTheme &theme = m_model->theme();
    p.setFont(theme.legendFont);
    QFontMetrics fm(theme.legendFont);

    const int swatchW = 14, swatchH = 10, gap = 6, itemGap = 14, pad = 8;
    int rowH = qMax(fm.height(), swatchH) + 4;
    int legendW, legendH;

    if (ori == 1) { // Horizontal
        int totalW = 0;
        for (Series *s : visible) totalW += swatchW + gap + fm.width(s->name());
        totalW += (visible.size()-1) * itemGap;
        legendW = totalW + pad*2; legendH = rowH + pad*2;
    } else {
        int maxItemW = 0;
        for (Series *s : visible) maxItemW = qMax(maxItemW, swatchW+gap+fm.width(s->name()));
        legendW = maxItemW + pad*2;
        legendH = visible.size()*rowH + (visible.size()-1)*2 + pad*2;
    }

    double lx, ly;
    switch (pos) {
    case 1: lx = plotArea.left()+(plotArea.width()-legendW)/2.0; ly = plotArea.top()+4; break;
    case 2: lx = plotArea.left()+(plotArea.width()-legendW)/2.0; ly = plotArea.bottom()-legendH-4; break;
    case 3: break; // Hidden
    default: lx = plotArea.right()-legendW-6; ly = plotArea.top()+4; break;
    }
    if (pos == 3) return; // Hidden

    QRectF lr(lx, ly, legendW, legendH);
    p.setPen(QPen(theme.legendBorder, 1));
    p.setBrush(theme.legendBackground);
    p.drawRoundedRect(lr, theme.legendRadius, theme.legendRadius);

    double cx = lx+pad, cy = ly+pad;
    if (ori == 1) { // Horizontal
        for (Series *s : visible) {
            QRectF sw(cx, cy+(rowH-swatchH)/2.0, swatchW, swatchH);
            p.fillRect(sw, s->color()); p.setPen(s->color().darker(130)); p.drawRect(sw);
            p.setPen(theme.textColor);
            p.drawText(QPointF(cx+swatchW+gap, cy+fm.ascent()+(rowH-fm.height())/2.0-1), s->name());
            cx += swatchW+gap+fm.width(s->name())+itemGap;
        }
    } else {
        for (Series *s : visible) {
            QRectF sw(cx, cy+(rowH-swatchH)/2.0, swatchW, swatchH);
            p.fillRect(sw, s->color()); p.setPen(s->color().darker(130)); p.drawRect(sw);
            p.setPen(theme.textColor);
            p.drawText(QPointF(cx+swatchW+gap, cy+fm.ascent()+(rowH-fm.height())/2.0-1), s->name());
            cy += rowH+2;
        }
    }
}

void ChartRenderer::drawTooltip(QPainter &p, const TooltipData &tip)
{
    if (!tip.visible || tip.text.isEmpty()) return;
    const ChartTheme &t = m_model->theme();
    p.setFont(t.tooltipFont);
    QFontMetrics fm(t.tooltipFont);

    QStringList rawLines = tip.text.split('\n');
    struct TL { QColor color; QString text; };
    QList<TL> lines;
    for (const QString &raw : rawLines) {
        TL tl; int bar = raw.indexOf('|');
        if (bar > 0 && raw.left(bar).startsWith('#') && raw.left(bar).length() == 7) {
            tl.color = QColor(raw.left(bar)); tl.text = raw.mid(bar+1);
        } else { tl.text = raw; }
        lines.append(tl);
    }

    const int padX=10, padY=6, spacing=3, swatchW=10, swatchH=10, swatchGap=6;
    int maxTW = 0;
    for (const TL &tl : lines) maxTW = qMax(maxTW, fm.width(tl.text));
    int textH = lines.size()*fm.height() + (lines.size()-1)*spacing;
    int tipW = maxTW + padX*2 + swatchW + swatchGap;
    int tipH = textH + padY*2;

    double tx = tip.position.x()+14, ty = tip.position.y()-tipH-10;
    QRectF pa = m_layout->plotArea();
    if (tx+tipW > pa.right()+50) tx = tip.position.x()-tipW-14;
    if (ty < 4) ty = tip.position.y()+14;

    QRectF tr(tx, ty, tipW, tipH);
    p.setPen(Qt::NoPen);
    p.setBrush(t.tooltipShadow);
    p.drawRoundedRect(tr.translated(2,2), t.tooltipRadius, t.tooltipRadius);
    p.setPen(QPen(t.tooltipBorder, 1));
    p.setBrush(t.tooltipBackground);
    p.drawRoundedRect(tr, t.tooltipRadius, t.tooltipRadius);

    double textX = tx+padX, textY = ty+padY+fm.ascent();
    for (int i = 0; i < lines.size(); ++i) {
        double ly = textY + i*(fm.height()+spacing);
        QColor c = lines[i].color;
        if (c.isValid()) {
            QRectF sw(textX, ly-fm.ascent()+(fm.height()-swatchH)/2.0, swatchW, swatchH);
            p.fillRect(sw, c); p.setPen(c.darker(140)); p.setBrush(Qt::NoBrush); p.drawRect(sw);
        }
        p.setPen(t.textColor);
        p.drawText(QPointF(textX+swatchW+swatchGap, ly), lines[i].text);
    }

    p.setPen(QPen(QColor(180,180,180), 1, Qt::DotLine));
    p.drawLine(tip.position,
               QPointF(tr.center().x(), tr.contains(tip.position) ? tr.bottom() : tr.top()));
}

// ========================================================================
//  ChartWidget
// ========================================================================

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent)
{
    ensureComponents();
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

ChartWidget::~ChartWidget()
{
    delete m_renderer;
    delete m_layout;
    delete m_model;
}

void ChartWidget::ensureComponents()
{
    if (!m_model) {
        m_model = new ChartModel(this);
        connect(m_model, &ChartModel::dataChanged, this, &ChartWidget::markDirty);
        connect(m_model, &ChartModel::themeChanged, this, &ChartWidget::markDirty);
    }
    if (!m_layout)  m_layout  = new ChartLayout(m_model, this);
    if (!m_renderer) m_renderer = new ChartRenderer(m_model, m_layout);
}

ChartModel*  ChartWidget::model()  const { return m_model; }
ChartLayout* ChartWidget::layout() const { return m_layout; }

void ChartWidget::addAxis(Axis *a)                           { m_model->addAxis(a); markDirty(); }
void ChartWidget::removeAxis(Axis *a)                        { m_model->removeAxis(a); markDirty(); }
void ChartWidget::addSeries(Series *s)                       { m_model->addSeries(s); markDirty(); }
void ChartWidget::removeSeries(Series *s)                    { m_model->removeSeries(s); markDirty(); }
void ChartWidget::setCategories(const QStringList &c)        { m_model->setCategories(c); markDirty(); }
void ChartWidget::setTitle(const QString &t)                 { m_model->setTitle(t); markDirty(); }
void ChartWidget::setTheme(const ChartTheme &t)              { m_model->setTheme(t); markDirty(); }

void ChartWidget::setRescaleMode(RescaleMode m)              { m_rescaleMode = m; markDirty(); }
ChartWidget::RescaleMode ChartWidget::rescaleMode() const    { return m_rescaleMode; }
void ChartWidget::fitToData()                                { m_rescaleMode = RescaleMode::FitVisible; markDirty(); }

void ChartWidget::zoom(double factor)
{
    Axis *ax = m_model->axisX(), *ay = m_model->axisY();
    if (!ax || !ay) return;
    QRectF pa = m_layout->plotArea();
    double xr = m_layout->xMax()-m_layout->xMin();
    double yr = m_layout->yMax()-m_layout->yMin();
    if (qFuzzyIsNull(xr) || qFuzzyIsNull(yr)) return;
    double cx = m_layout->xMin()+(pa.center().x()-pa.left())/pa.width()*xr;
    double cy = m_layout->yMin()+(pa.bottom()-pa.center().y())/pa.height()*yr;
    ax->setRange(cx-xr*factor/2.0, cx+xr*factor/2.0);
    ay->setRange(cy-yr*factor/2.0, cy+yr*factor/2.0);
    m_rescaleMode = RescaleMode::Manual;
    markDirty();
}

void ChartWidget::zoomTo(double x1, double x2, double y1, double y2)
{
    Axis *ax = m_model->axisX(), *ay = m_model->axisY();
    if (!ax || !ay) return;
    ax->setRange(x1, x2); ay->setRange(y1, y2);
    m_rescaleMode = RescaleMode::Manual;
    markDirty();
}

void ChartWidget::setLegendPosition(LegendPosition p)          { m_legendPosition = p; markDirty(); }
ChartWidget::LegendPosition ChartWidget::legendPosition() const { return m_legendPosition; }
void ChartWidget::setLegendOrientation(LegendOrientation o)    { m_legendOrientation = o; markDirty(); }
ChartWidget::LegendOrientation ChartWidget::legendOrientation() const { return m_legendOrientation; }

void ChartWidget::setTooltipEnabled(bool on) { m_tooltipEnabled = on; if (!on) { m_showTooltip = false; update(); } }
bool ChartWidget::isTooltipEnabled() const { return m_tooltipEnabled; }

QPixmap ChartWidget::exportToPixmap(const QSize &size) const
{
    QSize ps = size.isValid() ? size : this->size();
    QPixmap pm(ps);
    pm.fill(m_model->theme().background);
    QPainter p(&pm);
    const_cast<ChartLayout*>(m_layout)->recalculate(ps.width(), ps.height());
    m_renderer->render(p, ps.width(), ps.height());
    return pm;
}

void ChartWidget::refresh() { markDirty(); }

void ChartWidget::markDirty()
{
    m_bufferDirty = true;
    m_layout->invalidate();
    update();
}

void ChartWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_buffer = QPixmap(size());
    m_buffer.fill(m_model->theme().background);
    m_bufferDirty = true;
}

void ChartWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (m_bufferDirty || m_buffer.size() != size()) rebuildBuffer();
    painter.drawPixmap(0, 0, m_buffer);
}

// void ChartWidget::rebuildBuffer()
// {
//     // ===== 新增：计算外部图例高度并传递给布局 =====
//     QList<Series*> vis;
//     for (Series *s : m_model->seriesList()) if (s->isVisible()) vis.append(s);

//     double outsideLegendH = 0;
//     if (m_legendPosition == LegendPosition::OutsideTop && !vis.isEmpty()) {
//         const ChartTheme &theme = m_model->theme();
//         QFontMetrics fm(theme.legendFont);
//         const int swatchW = 14, gap = 6, pad = 8;
//         int maxItemW = 0;
//         for (Series *s : vis)
//             maxItemW = qMax(maxItemW, swatchW + gap + fm.width(s->name()));
//         outsideLegendH = fm.height() + pad * 2 + 4;  // 4px 与下方间距
//     }
//     m_layout->setOutsideLegendHeight(outsideLegendH);

//     m_buffer = QPixmap(size());
//     m_buffer.fill(m_model->theme().background);
//     QPainter p(&m_buffer);
//     m_layout->recalculate(width(), height());
//     m_renderer->render(p, width(), height());

//     // 图例
//     QList<Series*> vis;
//     for (Series *s : m_model->seriesList()) if (s->isVisible()) vis.append(s);
//     m_renderer->drawLegend(p, m_layout->plotArea(), vis,
//                            int(m_legendPosition), int(m_legendOrientation));

//     // 提示框
//     if (m_showTooltip) {
//         ChartRenderer::TooltipData tip;
//         tip.visible = true; tip.position = m_tooltipPos;
//         tip.text = m_tooltipText; tip.color = m_tooltipColor;
//         m_renderer->drawTooltip(p, tip);
//     }
//     m_bufferDirty = false;
// }

void ChartWidget::rebuildBuffer()
{
    // ===== 新增：计算外部图例高度并传递给布局 =====
    QList<Series*> vis;
    for (Series *s : m_model->seriesList()) if (s->isVisible()) vis.append(s);

    double outsideLegendH = 0;
    if (m_legendPosition == LegendPosition::OutsideTop && !vis.isEmpty()) {
        const ChartTheme &theme = m_model->theme();
        QFontMetrics fm(theme.legendFont);
        const int swatchW = 14, gap = 6, pad = 8;
        int maxItemW = 0;
        for (Series *s : vis)
            maxItemW = qMax(maxItemW, swatchW + gap + fm.width(s->name()));
        outsideLegendH = fm.height() + pad * 2 + 4;  // 4px 与下方间距
    }
    m_layout->setOutsideLegendHeight(outsideLegendH);

    m_buffer = QPixmap(size());
    m_buffer.fill(m_model->theme().background);
    QPainter p(&m_buffer);
    m_layout->recalculate(width(), height());
    m_renderer->render(p, width(), height());

    // ===== 新增：在图表上方绘制外部图例 =====
    if (m_legendPosition == LegendPosition::OutsideTop && !vis.isEmpty()) {
        const ChartTheme &theme = m_model->theme();
        p.setFont(theme.legendFont);
        QFontMetrics fm(theme.legendFont);

        const int swatchW = 14, swatchH = 10, gap = 6, itemGap = 14, pad = 8;
        int rowH = qMax(fm.height(), swatchH) + 4;

        // 计算图例总宽度
        int totalW = 0;
        for (Series *s : vis) totalW += swatchW + gap + fm.width(s->name());
        if (vis.size() > 1) totalW += (vis.size() - 1) * itemGap;
        int legendW = totalW + pad * 2;
        int legendH = rowH + pad * 2;

        // 水平居中，位于标题下方、绘图区域上方
        QRectF pa = m_layout->plotArea();
        double lx = pa.left() + (pa.width() - legendW) / 2.0;
        double ly = pa.top() - outsideLegendH;

        QRectF lr(lx, ly, legendW, legendH);
        p.setPen(QPen(theme.legendBorder, 1));
        p.setBrush(theme.legendBackground);
        p.drawRoundedRect(lr, theme.legendRadius, theme.legendRadius);

        double cx = lx + pad, cy = ly + pad;
        for (Series *s : vis) {
            QRectF sw(cx, cy + (rowH - swatchH) / 2.0, swatchW, swatchH);
            p.fillRect(sw, s->color());
            p.setPen(s->color().darker(130));
            p.setBrush(Qt::NoBrush);
            p.drawRect(sw);
            p.setPen(theme.textColor);
            p.drawText(QPointF(cx + swatchW + gap, cy + fm.ascent() + (rowH - fm.height()) / 2.0 - 1), s->name());
            cx += swatchW + gap + fm.width(s->name()) + itemGap;
        }
    }

    // ===== 修改：内部图例 — OutsideTop 时不绘制 =====
    if (m_legendPosition != LegendPosition::OutsideTop) {
        m_renderer->drawLegend(p, m_layout->plotArea(), vis,
                               int(m_legendPosition), int(m_legendOrientation));
    }

    // 工具提示（不变）
    if (m_showTooltip) {
        ChartRenderer::TooltipData tip;
        tip.visible = true; tip.position = m_tooltipPos;
        tip.text = m_tooltipText; tip.color = m_tooltipColor;
        m_renderer->drawTooltip(p, tip);
    }
    m_bufferDirty = false;
}


// ---- 鼠标交互 ----

void ChartWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_layout->plotArea().contains(e->pos())) {
        m_panning = true; m_panStart = e->pos();
        m_panXMin = m_layout->xMin(); m_panXMax = m_layout->xMax();
        m_panYMin = m_layout->yMin(); m_panYMax = m_layout->yMax();
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(e);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_panning) {
        Axis *ax = m_model->axisX(), *ay = m_model->axisY();
        if (ax && ay) {
            QPoint d = e->pos() - m_panStart;
            QRectF pa = m_layout->plotArea();
            double xr = m_panXMax-m_panXMin, yr = m_panYMax-m_panYMin;
            if (!qFuzzyIsNull(xr) && !qFuzzyIsNull(yr)) {
                ax->setRange(m_panXMin - d.x()/pa.width()*xr, m_panXMax - d.x()/pa.width()*xr);
                ay->setRange(m_panYMin + d.y()/pa.height()*yr, m_panYMax + d.y()/pa.height()*yr);
                m_rescaleMode = RescaleMode::Manual;
                markDirty();
            }
        }
    }
    findNearest(e->pos());
    update();
    QWidget::mouseMoveEvent(e);
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) { m_panning = false; setCursor(Qt::ArrowCursor); }
    QWidget::mouseReleaseEvent(e);
}

void ChartWidget::mouseDoubleClickEvent(QMouseEvent *) { fitToData(); }

void ChartWidget::wheelEvent(QWheelEvent *e)
{
    Axis *ax = m_model->axisX(), *ay = m_model->axisY();
    if (!ax || !ay) return;
    QRectF pa = m_layout->plotArea();
    if (!pa.contains(e->pos())) { QWidget::wheelEvent(e); return; }

    double factor = (e->angleDelta().y() > 0) ? 0.85 : 1.18;
    double xr = m_layout->xMax()-m_layout->xMin();
    double yr = m_layout->yMax()-m_layout->yMin();
    if (qFuzzyIsNull(xr) || qFuzzyIsNull(yr)) return;

    double dx = m_layout->pixelXToData(e->pos().x());
    double dy = m_layout->pixelYToData(e->pos().y());
    double rx = (dx-m_layout->xMin())/xr;
    double ry = (dy-m_layout->yMin())/yr;
    double nxr = xr*factor, nyr = yr*factor;
    ax->setRange(dx-rx*nxr, dx-rx*nxr+nxr);
    ay->setRange(dy-ry*nyr, dy-ry*nyr+nyr);
    m_rescaleMode = RescaleMode::Manual;
    markDirty();
    e->accept();
}

void ChartWidget::contextMenuEvent(QContextMenuEvent *e)
{
    Axis *ax = m_model->axisX(), *ay = m_model->axisY();
    QMenu menu(this);
    menu.addAction("Fit to Data", this, &ChartWidget::fitToData);
    menu.addSeparator();

    auto addCheck = [&](QMenu *m, const QString &text, bool chk) -> QAction* {
        QAction *a = m->addAction(text); a->setCheckable(true); a->setChecked(chk); return a;
    };

    QMenu *legM = menu.addMenu("Legend");
    QAction *aTR = addCheck(legM, "Top Right", m_legendPosition==LegendPosition::TopRight);
    QAction *aTP = addCheck(legM, "Top",       m_legendPosition==LegendPosition::Top);
    QAction *aBT = addCheck(legM, "Bottom",    m_legendPosition==LegendPosition::Bottom);
    QAction *aOT = addCheck(legM, "Above Chart",    m_legendPosition==LegendPosition::OutsideTop);  // 新增
    QAction *aHD = addCheck(legM, "Hidden",    m_legendPosition==LegendPosition::Hidden);
    QMenu *oriM = menu.addMenu("Layout");
    QAction *aH = addCheck(oriM, "Horizontal", m_legendOrientation==LegendOrientation::Horizontal);
    QAction *aV = addCheck(oriM, "Vertical",   m_legendOrientation==LegendOrientation::Vertical);
    menu.addSeparator();
    QMenu *grM = menu.addMenu("Grid");
    QAction *aYG = addCheck(grM, "Horizontal (Y)", ay && ay->isGridVisible() && ay->isHorizontalGridVisible());
    QAction *aXG = addCheck(grM, "Vertical (X)",   ax && ax->isGridVisible() && ax->isVerticalGridVisible());
    menu.addSeparator();
    menu.addAction("Zoom In",  this, [this](){ zoom(0.7); });
    menu.addAction("Zoom Out", this, [this](){ zoom(1.4); });

    QAction *ch = menu.exec(e->globalPos());
    if (!ch) return;
    if      (ch==aTR) setLegendPosition(LegendPosition::TopRight);
    else if (ch==aTP) setLegendPosition(LegendPosition::Top);
    else if (ch==aBT) setLegendPosition(LegendPosition::Bottom);
    else if (ch==aOT) setLegendPosition(LegendPosition::OutsideTop);  // 新增
    else if (ch==aHD) setLegendPosition(LegendPosition::Hidden);
    else if (ch==aH)  setLegendOrientation(LegendOrientation::Horizontal);
    else if (ch==aV)  setLegendOrientation(LegendOrientation::Vertical);
    else if (ch==aYG && ay) { bool on=!ay->isHorizontalGridVisible(); ay->setGridVisible(on); ay->setHorizontalGridVisible(on); markDirty(); }
    else if (ch==aXG && ax) { bool on=!ax->isVerticalGridVisible(); ax->setGridVisible(on); ax->setVerticalGridVisible(on); markDirty(); }
}

void ChartWidget::leaveEvent(QEvent *) { m_showTooltip = false; update(); }

// ---- 提示框 ----

double ChartWidget::pointDist(const QPointF &a, const QPointF &b) const
{ double dx=a.x()-b.x(), dy=a.y()-b.y(); return std::sqrt(dx*dx+dy*dy); }

void ChartWidget::findNearest(QPoint mousePos)
{
    m_showTooltip = false; m_tooltipText.clear();
    if (!m_tooltipEnabled || !m_layout->plotArea().contains(mousePos)) return;

    double bestDist = 15.0;
    Series *bestSeries = nullptr;
    int bestPt=-1, bestCat=-1, bestBar=-1;

    QStringList cats = m_model->categories();
    if (!cats.isEmpty()) {
        QRectF pa = m_layout->plotArea();
        double catWidth = pa.width() / double(cats.size());
        for (int c = 0; c < cats.size(); ++c) {
            double cx = pa.left() + (double(c)+0.5)*catWidth;
            if (qAbs(mousePos.x()-cx) >= catWidth*0.45) continue;
            // stacked bar
            for (Series *s : m_model->seriesList()) {
                if (!s->isVisible() || s->type()!=SeriesType::StackedBar) continue;
                if (c < static_cast<StackedBarSeries*>(s)->dataCount()) {
                    double d = qAbs(mousePos.x()-cx);
                    if (d < bestDist) { bestDist=d; bestCat=c; bestSeries=s; bestBar=-2; bestPt=-1; }
                }
            }
            // bar
            QList<BarSeries*> bars;
            for (Series *s : m_model->seriesList())
                if (s->isVisible() && s->type()==SeriesType::Bar && !static_cast<BarSeries*>(s)->useXY())
                    bars.append(static_cast<BarSeries*>(s));
            if (!bars.isEmpty()) {
                double groupW = catWidth*bars.first()->barWidthRatio();
                double singleW = groupW/bars.size();
                for (int bi=0; bi<bars.size(); ++bi) {
                    if (c >= bars[bi]->dataCount()) continue;
                    double barL = cx-groupW/2.0+bi*singleW;
                    double val = bars[bi]->data().at(c);
                    QPointF top=m_layout->mapToPixel(0,val), bot=m_layout->mapToPixel(0,qMax(m_layout->yMin(),0.0));
                    double barT=qMin(top.y(),bot.y()), barB=qMax(top.y(),bot.y());
                    if (mousePos.x()>=barL && mousePos.x()<=barL+singleW-1 && mousePos.y()>=barT && mousePos.y()<=barB) {
                        double d = pointDist(mousePos, QPointF(cx,(barT+barB)/2.0));
                        if (d<bestDist) { bestDist=d; bestCat=c; bestSeries=bars[bi]; bestBar=bi; bestPt=-1; }
                    }
                }
            }
            break;
        }
    }

    // line
    for (Series *s : m_model->seriesList()) {
        if (!s->isVisible() || s->type()!=SeriesType::Line) continue;
        auto *ls = static_cast<LineSeries*>(s);
        for (int i=0; i<ls->dataCount(); ++i) {
            double d = pointDist(mousePos, m_layout->mapToPixel(ls->data()[i].x, ls->data()[i].y));
            if (d<bestDist) { bestDist=d; bestSeries=s; bestPt=i; bestCat=-1; bestBar=-1; }
        }
    }

    // xy bar
    for (Series *s : m_model->seriesList()) {
        if (!s->isVisible() || s->type()!=SeriesType::Bar) continue;
        auto *bs = static_cast<BarSeries*>(s);
        if (!bs->useXY()) continue;
        double barW = m_layout->plotArea().width()*0.03*bs->barWidthRatio();
        for (int i=0; i<bs->xyData().size(); ++i) {
            QPointF top=m_layout->mapToPixel(bs->xyData()[i].x, bs->xyData()[i].y);
            QPointF bot=m_layout->mapToPixel(bs->xyData()[i].x, qMax(m_layout->yMin(),0.0));
            QRectF r(top.x()-barW/2.0, top.y(), barW, bot.y()-top.y());
            if (r.adjusted(-4,-4,4,4).contains(mousePos)) {
                double d = pointDist(mousePos, QPointF(top.x(),(top.y()+bot.y())/2.0));
                if (d<bestDist) { bestDist=d; bestSeries=s; bestPt=i; bestCat=-1; bestBar=-1; }
            }
        }
    }

    if (!bestSeries) return;
    m_showTooltip = true; m_mouseScreen = mousePos; m_tooltipColor = bestSeries->color();

    if (bestSeries->type()==SeriesType::StackedBar && bestCat>=0) buildTooltipForStackedBar(bestCat);
    else if (bestSeries->type()==SeriesType::Bar && bestCat>=0 && bestBar>=0) buildTooltipForBar(bestCat, bestBar);
    else if (bestSeries->type()==SeriesType::Line && bestPt>=0) buildTooltipForLine(static_cast<LineSeries*>(bestSeries), bestPt);

    emit dataPointHovered(bestSeries, bestPt, QPointF());
}

void ChartWidget::buildTooltipForLine(LineSeries *ls, int ptIdx)
{
    if (ptIdx<0 || ptIdx>=ls->dataCount()) return;
    const DataPoint &dp = ls->data().at(ptIdx);
    m_tooltipPos = m_layout->mapToPixel(dp.x, dp.y);
    m_tooltipText = QString("%1|%2\n#000000|X: %3\n#000000|Y: %4")
                        .arg(ls->color().name()).arg(ls->name())
                        .arg(m_layout->formatAxisValue(m_model->axisX(), dp.x))
                        .arg(dp.y, 0, 'f', 2);
}

void ChartWidget::buildTooltipForBar(int catIdx, int barIdx)
{
    QList<BarSeries*> bars;
    for (Series *s : m_model->seriesList())
        if (s->isVisible() && s->type()==SeriesType::Bar && !static_cast<BarSeries*>(s)->useXY())
            bars.append(static_cast<BarSeries*>(s));
    if (barIdx<0 || barIdx>=bars.size() || catIdx>=bars[barIdx]->dataCount()) return;
    BarSeries *bs = bars[barIdx];
    QString label = m_model->categories().value(catIdx, QString::number(catIdx));
    m_tooltipText = QString("%1|%2\n#000000|%3: %4")
                        .arg(bs->color().name()).arg(bs->name()).arg(label).arg(bs->data().at(catIdx), 0, 'f', 2);
    QRectF pa = m_layout->plotArea();
    double catWidth = pa.width()/double(m_model->categories().size());
    m_tooltipPos = QPointF(pa.left()+(double(catIdx)+0.5)*catWidth, m_layout->mapToPixel(0, bs->data().at(catIdx)).y());
}

void ChartWidget::buildTooltipForStackedBar(int catIdx)
{
    QList<StackedBarSeries*> stacked;
    for (Series *s : m_model->seriesList())
        if (s->isVisible() && s->type()==SeriesType::StackedBar)
            stacked.append(static_cast<StackedBarSeries*>(s));
    QString label = m_model->categories().value(catIdx, QString::number(catIdx));
    double sum = 0;
    QStringList lines;
    lines << QString("#000000|Category: %1").arg(label);
    for (auto *sb : stacked) {
        double v = (catIdx<sb->dataCount()) ? sb->data().at(catIdx) : 0;
        lines << QString("%1|%2: %3").arg(sb->color().name()).arg(sb->name()).arg(v,0,'f',2);
        sum += v;
    }
    lines << QString("#000000|Total: %1").arg(sum,0,'f',2);
    m_tooltipText = lines.join("\n");
    QRectF pa = m_layout->plotArea();
    double catWidth = pa.width()/double(m_model->categories().size());
    m_tooltipPos = QPointF(pa.left()+(double(catIdx)+0.5)*catWidth, m_layout->mapToPixel(0,sum).y());
}
