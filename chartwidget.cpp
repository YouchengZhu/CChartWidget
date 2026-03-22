#include "ChartWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QtMath>
#include <algorithm>
#include <cmath>

/* ═══════════════════════════════════════════════════════════════
 *  Axis 基类
 * ═══════════════════════════════════════════════════════════════ */

Axis::Axis(Position pos, QObject *parent)
    : QObject(parent), m_position(pos)
{
    m_labelFont = QFont("Microsoft YaHei", 9);
    m_titleFont = QFont("Microsoft YaHei", 10, QFont::Bold);
    m_gridColor = QColor(210, 210, 210);
}
Axis::~Axis() {}
Axis::Position Axis::position() const { return m_position; }
void Axis::setRange(double lo, double hi) { m_autoRange = false; m_min = lo; m_max = hi; }
void Axis::setAutoRange(bool on) { m_autoRange = on; }
bool Axis::isAutoRange() const { return m_autoRange; }
double Axis::min() const { return m_min; }
double Axis::max() const { return m_max; }
void Axis::setTitle(const QString &t) { m_title = t; }
QString Axis::title() const { return m_title; }
void Axis::setTickCount(int n) { m_tickCount = qMax(2, n); }
int  Axis::tickCount() const { return m_tickCount; }
void Axis::setLabelFont(const QFont &f) { m_labelFont = f; }
QFont Axis::labelFont() const { return m_labelFont; }
void Axis::setTitleFont(const QFont &f) { m_titleFont = f; }
QFont Axis::titleFont() const { return m_titleFont; }
void Axis::setGridVisible(bool on) { m_gridVisible = on; }
bool Axis::isGridVisible() const { return m_gridVisible; }
void Axis::setGridColor(const QColor &c) { m_gridColor = c; }
QColor Axis::gridColor() const { return m_gridColor; }

double Axis::niceStep(double range, int tickCount)
{
    if (range <= 0 || tickCount < 2) return 1.0;
    double rough = range / double(tickCount - 1);
    double mag   = std::pow(10.0, std::floor(std::log10(rough)));
    double norm  = rough / mag;
    double nice;
    if      (norm < 1.5) nice = 1.0;
    else if (norm < 3.5) nice = 2.0;
    else if (norm < 7.5) nice = 5.0;
    else                 nice = 10.0;
    return nice * mag;
}
int Axis::decimalsForStep(double step)
{
    if (step >= 1.0) return 0;
    int d = 0; double s = step;
    while (s < 1.0 && d < 10) { s *= 10.0; ++d; }
    return d;
}

/* ═══════════════════════════════════════════════════════════════
 *  NumericAxis
 * ═══════════════════════════════════════════════════════════════ */

NumericAxis::NumericAxis(Position pos, QObject *parent) : Axis(pos, parent) {}
void NumericAxis::setNotation(Notation n) { m_notation = n; }
NumericAxis::Notation NumericAxis::notation() const { return m_notation; }
void NumericAxis::setFormatPrecision(int p) { m_precision = p; }
int  NumericAxis::formatPrecision() const { return m_precision; }

QString NumericAxis::formatValue(double v) const
{
    if (m_notation == Scientific)
        return QString::number(v, 'e', qMax(1, m_precision < 0 ? 2 : m_precision));
    int prec = m_precision;
    if (prec < 0) {
        double step = niceStep(m_max - m_min, m_tickCount);
        prec = decimalsForStep(step);
    }
    return QString::number(v, 'f', prec);
}
QVector<double> NumericAxis::computeTicks() const
{
    QVector<double> ticks;
    double range = m_max - m_min;
    if (range <= 0) return ticks;
    double step = niceStep(range, m_tickCount);
    double v = std::ceil(m_min / step) * step;
    double eps = step * 1e-6;
    while (v <= m_max + eps) { if (v >= m_min - eps) ticks.append(v); v += step; }
    return ticks;
}

/* ═══════════════════════════════════════════════════════════════
 *  DateTimeAxis
 * ═══════════════════════════════════════════════════════════════ */

DateTimeAxis::DateTimeAxis(Position pos, QObject *parent) : Axis(pos, parent) {}
void DateTimeAxis::setFormat(Format f) { m_fmt = f; }
DateTimeAxis::Format DateTimeAxis::format() const { return m_fmt; }
double DateTimeAxis::dt2d(const QDateTime &dt) const { return static_cast<double>(dt.toSecsSinceEpoch()); }
QDateTime DateTimeAxis::d2dt(double v) const { return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(v)); }
void DateTimeAxis::setRange(const QDateTime &lo, const QDateTime &hi) { Axis::setRange(dt2d(lo), dt2d(hi)); }
QDateTime DateTimeAxis::minDateTime() const { return d2dt(m_min); }
QDateTime DateTimeAxis::maxDateTime() const { return d2dt(m_max); }

QString DateTimeAxis::formatValue(double v) const
{
    QDateTime dt = d2dt(v);
    switch (m_fmt) {
    case HHmmss:    return dt.toString("HH:mm:ss");
    case HHmm:      return dt.toString("HH:mm");
    case HHmmsszzz: return dt.toString("HH:mm:ss.zzz");
    }
    return dt.toString("HH:mm:ss");
}
QVector<double> DateTimeAxis::computeTicks() const
{
    QVector<double> ticks;
    double range = m_max - m_min;
    if (range <= 0) return ticks;
    static const qint64 intervals[] = {
        1,2,5,10,15,30, 60,120,300,600,900,1800,
        3600,7200,14400,21600,43200, 86400
    };
    int bestIdx = 0;
    for (int i = 0; i < 18; ++i) {
        if (range / double(intervals[i]) <= double(m_tickCount) * 1.5) { bestIdx = i; break; }
        bestIdx = i;
    }
    qint64 step = intervals[bestIdx];
    qint64 lo = static_cast<qint64>(m_min), hi = static_cast<qint64>(m_max);
    qint64 start = (lo / step) * step;
    if (start < lo) start += step;
    for (qint64 t = start; t <= hi; t += step) ticks.append(static_cast<double>(t));
    return ticks;
}

/* ═══════════════════════════════════════════════════════════════
 *  DateAxis
 * ═══════════════════════════════════════════════════════════════ */

DateAxis::DateAxis(Position pos, QObject *parent) : Axis(pos, parent) {}
void DateAxis::setFormat(Format f) { m_fmt = f; }
DateAxis::Format DateAxis::format() const { return m_fmt; }
double DateAxis::dt2d(const QDateTime &dt) const { return static_cast<double>(dt.toSecsSinceEpoch()); }
QDateTime DateAxis::d2dt(double v) const { return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(v)); }
void DateAxis::setRange(const QDateTime &lo, const QDateTime &hi) { Axis::setRange(dt2d(lo), dt2d(hi)); }
QDateTime DateAxis::minDateTime() const { return d2dt(m_min); }
QDateTime DateAxis::maxDateTime() const { return d2dt(m_max); }

QString DateAxis::formatValue(double v) const
{
    QDate d = d2dt(v).date();
    switch (m_fmt) {
    case yyyyMMdd:   return d.toString("yyyy-MM-dd");
    case yyyy_MM_dd: return d.toString("yyyy/MM/dd");
    case MMdd:       return d.toString("MM-dd");
    case yyyyMM:     return d.toString("yyyy-MM");
    }
    return d.toString("yyyy-MM-dd");
}
QVector<double> DateAxis::computeTicks() const
{
    QVector<double> ticks;
    double range = m_max - m_min;
    if (range <= 0) return ticks;
    qint64 rangeDays = static_cast<qint64>(range) / 86400;
    int stepDays;
    if      (rangeDays <= 14)  stepDays = 1;
    else if (rangeDays <= 60)  stepDays = 3;
    else if (rangeDays <= 180) stepDays = 7;
    else if (rangeDays <= 730) stepDays = 30;
    else                       stepDays = 90;
    qint64 stepSecs = qint64(stepDays) * 86400;
    QDateTime startDt = d2dt(m_min).date().startOfDay();
    qint64 lo = static_cast<qint64>(m_min), hi = static_cast<qint64>(m_max);
    qint64 t = static_cast<qint64>(dt2d(startDt));
    if (t < lo) t += stepSecs;
    while (t <= hi) { ticks.append(static_cast<double>(t)); t += stepSecs; }
    return ticks;
}

/* ═══════════════════════════════════════════════════════════════
 *  Series 基类
 * ═══════════════════════════════════════════════════════════════ */

Series::Series(const QString &name, Type type, QObject *parent)
    : QObject(parent), m_name(name), m_type(type) {}
Series::~Series() {}
QString Series::name() const { return m_name; }
Series::Type Series::type() const { return m_type; }
void Series::setVisible(bool on) { m_visible = on; }
bool Series::isVisible() const { return m_visible; }
void Series::setColor(const QColor &c) { m_color = c; }
QColor Series::color() const { return m_color; }

/* ═══════════════════════════════════════════════════════════════
 *  LineSeries
 * ═══════════════════════════════════════════════════════════════ */

LineSeries::LineSeries(const QString &name, QObject *parent) : Series(name, Line, parent) {}
void LineSeries::append(double x, double y) { m_data.append(DataPoint(x, y)); }
void LineSeries::append(const QDateTime &tx, double y) { m_data.append(DataPoint(tx, y)); }
void LineSeries::append(const DataPoint &p) { m_data.append(p); }
void LineSeries::append(const QVector<DataPoint> &pts) { m_data.append(pts); }
void LineSeries::removeAt(int i) { if (i >= 0 && i < m_data.size()) m_data.removeAt(i); }
void LineSeries::clear() { m_data.clear(); }
const QVector<DataPoint>& LineSeries::data() const { return m_data; }
int LineSeries::dataCount() const { return m_data.size(); }
void LineSeries::setLineWidth(double w) { m_lineWidth = w; }
double LineSeries::lineWidth() const { return m_lineWidth; }
void LineSeries::setMarkerSize(double s) { m_markerSize = s; }
double LineSeries::markerSize() const { return m_markerSize; }
void LineSeries::setScatterStyle(ScatterStyle s) { m_scatterStyle = s; }
ScatterStyle LineSeries::scatterStyle() const { return m_scatterStyle; }

/* ═══════════════════════════════════════════════════════════════
 *  BarSeries
 * ═══════════════════════════════════════════════════════════════ */

BarSeries::BarSeries(const QString &name, QObject *parent) : Series(name, Bar, parent) {}
void BarSeries::append(double v) { m_data.append(v); m_useXY = false; }
void BarSeries::removeAt(int i) {
    if (m_useXY) { if (i>=0 && i<m_xyData.size()) m_xyData.removeAt(i); }
    else         { if (i>=0 && i<m_data.size())    m_data.removeAt(i); }
}
void BarSeries::clear() { m_data.clear(); m_xyData.clear(); }
const QVector<double>& BarSeries::data() const { return m_data; }
int BarSeries::dataCount() const { return m_useXY ? m_xyData.size() : m_data.size(); }
void BarSeries::appendXY(double x, double y) { m_xyData.append({x,y}); m_useXY = true; }
const QVector<BarSeries::XY>& BarSeries::xyData() const { return m_xyData; }
bool BarSeries::useXY() const { return m_useXY; }
void BarSeries::setBarWidthRatio(double r) { m_barWidthRatio = qBound(0.1, r, 1.0); }
double BarSeries::barWidthRatio() const { return m_barWidthRatio; }

/* ═══════════════════════════════════════════════════════════════
 *  StackedBarSeries
 * ═══════════════════════════════════════════════════════════════ */

StackedBarSeries::StackedBarSeries(const QString &name, QObject *parent)
    : Series(name, StackedBar, parent) {}
void StackedBarSeries::append(double v) { m_data.append(v); }
void StackedBarSeries::removeAt(int i) { if (i>=0 && i<m_data.size()) m_data.removeAt(i); }
void StackedBarSeries::clear() { m_data.clear(); }
const QVector<double>& StackedBarSeries::data() const { return m_data; }
int StackedBarSeries::dataCount() const { return m_data.size(); }
void StackedBarSeries::setBarWidthRatio(double r) { m_barWidthRatio = qBound(0.1, r, 1.0); }
double StackedBarSeries::barWidthRatio() const { return m_barWidthRatio; }

/* ═══════════════════════════════════════════════════════════════
 *  默认调色板
 * ═══════════════════════════════════════════════════════════════ */

static const QColor s_palette[] = {
    QColor(255, 77, 79),  QColor(54, 162, 235), QColor(255, 206, 86),
    QColor(75, 192, 192), QColor(153, 102, 255), QColor(255, 159, 64),
    QColor(46, 204, 113), QColor(231, 76, 60),  QColor(52, 152, 219),
    QColor(241, 196, 15), QColor(26, 188, 156),  QColor(155, 89, 182),
};
static const int s_paletteCount = 12;

/* ═══════════════════════════════════════════════════════════════
 *  ChartWidget 构造
 * ═══════════════════════════════════════════════════════════════ */

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent)
{
    m_bgColor     = QColor(252, 252, 252);
    m_plotBgColor = QColor(255, 255, 255);
    m_titleFont   = QFont("Microsoft YaHei", 13, QFont::Bold);
    m_tooltipFont = QFont("Microsoft YaHei", 9);
    m_legendFont  = QFont("Microsoft YaHei", 9);
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}
ChartWidget::~ChartWidget() {}

/* ═══════════════════════════════════════════════════════════════
 *  公共接口
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::addAxis(Axis *axis)
{
    if (!axis) return;
    if (axis->position() == Axis::Bottom) {
        if (m_axisX && m_axisX->parent() == this) m_axisX->setParent(nullptr);
        m_axisX = axis;
    } else {
        if (m_axisY && m_axisY->parent() == this) m_axisY->setParent(nullptr);
        m_axisY = axis;
    }
    axis->setParent(this);
    m_layoutDirty = true;
}
void ChartWidget::removeAxis(Axis *axis)
{
    if (axis == m_axisX) m_axisX = nullptr;
    if (axis == m_axisY) m_axisY = nullptr;
    if (axis && axis->parent() == this) axis->setParent(nullptr);
    m_layoutDirty = true;
}
QList<Axis*> ChartWidget::axes() const {
    QList<Axis*> a;
    if (m_axisX) a << m_axisX;
    if (m_axisY) a << m_axisY;
    return a;
}
void ChartWidget::addSeries(Series *s)
{
    if (!s) return;
    if (!s->color().isValid())
        s->setColor(s_palette[m_series.size() % s_paletteCount]);
    m_series.append(s);
    s->setParent(this);
    m_layoutDirty = true;
}
void ChartWidget::removeSeries(Series *s)
{
    m_series.removeOne(s);
    if (s && s->parent() == this) s->setParent(nullptr);
    m_layoutDirty = true;
}
QList<Series*> ChartWidget::seriesList() const { return m_series; }
void ChartWidget::setCategories(const QStringList &c) { m_categories = c; m_layoutDirty = true; }
QStringList ChartWidget::categories() const { return m_categories; }
void ChartWidget::setTitle(const QString &t) { m_title = t; }
QString ChartWidget::title() const { return m_title; }
void ChartWidget::setTitleFont(const QFont &f) { m_titleFont = f; }
void ChartWidget::setBackgroundColor(const QColor &c) { m_bgColor = c; }
void ChartWidget::setPlotBackgroundColor(const QColor &c) { m_plotBgColor = c; }
void ChartWidget::setMargin(int m) { m_margin = m; }
void ChartWidget::refresh() { m_layoutDirty = true; update(); }

void ChartWidget::setRescaleMode(RescaleMode m) { m_rescaleMode = m; m_layoutDirty = true; }
ChartWidget::RescaleMode ChartWidget::rescaleMode() const { return m_rescaleMode; }
void ChartWidget::fitToData() { m_rescaleMode = FitVisible; m_layoutDirty = true; update(); }

void ChartWidget::zoom(double factor)
{
    if (!m_axisX || !m_axisY) return;
    QPointF center(m_plotArea.center().x(), m_plotArea.center().y());
    double xr = m_xMax - m_xMin, yr = m_yMax - m_yMin;
    if (qFuzzyIsNull(xr) || qFuzzyIsNull(yr)) return;
    double cx = m_xMin + (center.x() - m_plotArea.left()) / m_plotArea.width() * xr;
    double cy = m_yMin + (m_plotArea.bottom() - center.y()) / m_plotArea.height() * yr;
    m_axisX->setRange(cx - xr * factor / 2.0, cx + xr * factor / 2.0);
    m_axisY->setRange(cy - yr * factor / 2.0, cy + yr * factor / 2.0);
    syncRangeFromAxes();
    m_rescaleMode = Manual;
    m_layoutDirty = true;
    update();
}
void ChartWidget::zoomTo(double xMin, double xMax, double yMin, double yMax)
{
    if (!m_axisX || !m_axisY) return;
    m_axisX->setRange(xMin, xMax); m_axisY->setRange(yMin, yMax);
    syncRangeFromAxes();
    m_rescaleMode = Manual;
    m_layoutDirty = true;
    update();
}

void ChartWidget::setLegendPosition(LegendPosition p) { m_legendPosition = p; m_layoutDirty = true; }
ChartWidget::LegendPosition ChartWidget::legendPosition() const { return m_legendPosition; }
void ChartWidget::setLegendOrientation(LegendOrientation o) { m_legendOrientation = o; m_layoutDirty = true; }
ChartWidget::LegendOrientation ChartWidget::legendOrientation() const { return m_legendOrientation; }
void ChartWidget::setLegendFont(const QFont &f) { m_legendFont = f; }
void ChartWidget::setTooltipEnabled(bool on) { m_tooltipEnabled = on; if (!on) m_showTooltip = false; }
bool ChartWidget::isTooltipEnabled() const { return m_tooltipEnabled; }
void ChartWidget::setTooltipFont(const QFont &f) { m_tooltipFont = f; }

/* ═══════════════════════════════════════════════════════════════
 *  范围计算
 * ═══════════════════════════════════════════════════════════════ */

double ChartWidget::xToDouble(double raw) const { return raw; }
void ChartWidget::syncRangeFromAxes()
{
    if (m_axisX) { m_xMin = m_axisX->min(); m_xMax = m_axisX->max(); }
    if (m_axisY) { m_yMin = m_axisY->min(); m_yMax = m_axisY->max(); }
}

void ChartWidget::computeXRange()
{
    if (!m_axisX) return;
    if (m_rescaleMode == Manual) { syncRangeFromAxes(); return; }
    if (!m_categories.isEmpty()) {
        m_xMin = 0; m_xMax = qMax(1, m_categories.size() - 1);
        m_axisX->setRange(m_xMin, m_xMax);
        return;
    }
    double lo =  std::numeric_limits<double>::max();
    double hi = -std::numeric_limits<double>::max();
    bool found = false;
    for (Series *s : m_series) {
        if (m_rescaleMode == FitVisible && !s->isVisible()) continue;
        if (auto ls = qobject_cast<LineSeries*>(s)) {
            for (auto &pt : ls->data()) { double v = xToDouble(pt.x); lo = qMin(lo, v); hi = qMax(hi, v); found = true; }
        } else if (auto bs = qobject_cast<BarSeries*>(s)) {
            if (bs->useXY())
                for (auto &xy : bs->xyData()) { double v = xToDouble(xy.x); lo = qMin(lo, v); hi = qMax(hi, v); found = true; }
        }
    }
    if (!found) { lo = 0; hi = 100; }
    if (qFuzzyCompare(lo, hi)) { lo -= 1; hi += 1; }
    m_axisX->setRange(lo, hi); m_xMin = lo; m_xMax = hi;
}

void ChartWidget::computeYRange()
{
    if (!m_axisY) return;
    if (m_rescaleMode == Manual) { syncRangeFromAxes(); return; }
    double lo =  std::numeric_limits<double>::max();
    double hi = -std::numeric_limits<double>::max();
    bool found = false;
    for (Series *s : m_series) {
        if (m_rescaleMode == FitVisible && !s->isVisible()) continue;
        if (auto ls = qobject_cast<LineSeries*>(s)) {
            for (auto &pt : ls->data()) { lo = qMin(lo, pt.y); hi = qMax(hi, pt.y); found = true; }
        } else if (auto bs = qobject_cast<BarSeries*>(s)) {
            if (bs->useXY()) { for (auto &xy : bs->xyData()) { lo = qMin(lo, xy.y); hi = qMax(hi, xy.y); found = true; } }
            else { for (double v : bs->data()) { lo = qMin(lo, v); hi = qMax(hi, v); found = true; } }
        }
    }
    QList<StackedBarSeries*> stacked;
    for (Series *s : m_series)
        if (m_rescaleMode != FitVisible || s->isVisible())
            if (s->type() == Series::StackedBar) stacked.append(qobject_cast<StackedBarSeries*>(s));
    if (!stacked.isEmpty()) {
        int maxCat = 0;
        for (auto *sb : stacked) maxCat = qMax(maxCat, sb->dataCount());
        for (int i = 0; i < maxCat; ++i) {
            double sum = 0;
            for (auto *sb : stacked) if (i < sb->dataCount()) sum += sb->data().at(i);
            lo = qMin(lo, 0.0); hi = qMax(hi, sum); found = true;
        }
    }
    if (!found) { lo = 0; hi = 100; }
    if (qFuzzyCompare(lo, hi)) hi += 1;
    double margin = (hi - lo) * 0.08;
    lo -= margin; hi += margin;
    if (lo > 0) lo = 0;
    m_axisY->setRange(lo, hi); m_yMin = lo; m_yMax = hi;
}

/* ═══════════════════════════════════════════════════════════════
 *  布局 & 映射
 * ═══════════════════════════════════════════════════════════════ */

QRectF ChartWidget::plotRect() const { return m_plotArea; }

void ChartWidget::updateLayout()
{
    QFontMetrics lfm(m_axisY ? m_axisY->labelFont() : QFont());
    QFontMetrics tfm(m_axisX ? m_axisX->titleFont() : QFont());
    double left = m_margin, top = m_margin, right = m_margin, bottom = m_margin;

    if (!m_title.isEmpty()) { QFontMetrics fm(m_titleFont); top += fm.height() + 4; }
    if (m_axisY) {
        double maxW = 0;
        for (double v : m_axisY->computeTicks()) maxW = qMax(maxW, double(lfm.width(m_axisY->formatValue(v))));
        left += maxW + 10;
        if (!m_axisY->title().isEmpty()) left += tfm.height() + 4;
    }
    if (m_axisX) {
        bottom += lfm.height() + 8;
        if (!m_axisX->title().isEmpty()) bottom += tfm.height() + 4;
    }
    m_plotArea = QRectF(left, top, qMax(1.0, width() - left - right), qMax(1.0, height() - top - bottom));
}

QPointF ChartWidget::mapToPixel(double dataX, double dataY) const
{
    double xr = m_xMax - m_xMin; if (qFuzzyIsNull(xr)) xr = 1.0;
    double yr = m_yMax - m_yMin; if (qFuzzyIsNull(yr)) yr = 1.0;
    double px = m_plotArea.left()   + (xToDouble(dataX) - m_xMin) / xr * m_plotArea.width();
    double py = m_plotArea.bottom() - (dataY - m_yMin) / yr * m_plotArea.height();
    return QPointF(px, py);
}

/* ═══════════════════════════════════════════════════════════════
 *  绘制总入口
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::paintEvent(QPaintEvent *)
{
    if (m_layoutDirty) { computeXRange(); computeYRange(); updateLayout(); m_layoutDirty = false; }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    drawBackground(p);
    drawPlotBackground(p);
    drawGrid(p);
    drawSeries(p);
    drawAxes(p);
    drawTitle(p);
    drawLegend(p);
    drawTooltip(p);
}

void ChartWidget::drawBackground(QPainter &p) { p.fillRect(rect(), m_bgColor); }

void ChartWidget::drawPlotBackground(QPainter &p)
{
    p.fillRect(m_plotArea, m_plotBgColor);
    p.setPen(QPen(QColor(200, 200, 200), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(m_plotArea);
}

void ChartWidget::drawGrid(QPainter &p)
{
    if (!m_axisX || !m_axisY) return;
    if (m_axisY->isGridVisible()) {
        QPen pen(m_axisY->gridColor(), 1, Qt::DashLine);
        p.setPen(pen);
        for (double v : m_axisY->computeTicks()) {
            QPointF pt = mapToPixel(m_xMin, v);
            if (pt.y() >= m_plotArea.top() && pt.y() <= m_plotArea.bottom())
                p.drawLine(QPointF(m_plotArea.left(), pt.y()), QPointF(m_plotArea.right(), pt.y()));
        }
    }
    if (m_axisX->isGridVisible()) {
        QPen pen(m_axisX->gridColor(), 1, Qt::DashLine);
        p.setPen(pen);
        QVector<double> ticks;
        if (!m_categories.isEmpty()) { for (int i = 0; i < m_categories.size(); ++i) ticks.append(double(i)); }
        else { ticks = m_axisX->computeTicks(); }
        for (double v : ticks) {
            QPointF pt = mapToPixel(v, m_yMin);
            if (pt.x() >= m_plotArea.left() && pt.x() <= m_plotArea.right())
                p.drawLine(QPointF(pt.x(), m_plotArea.top()), QPointF(pt.x(), m_plotArea.bottom()));
        }
    }
}

void ChartWidget::drawSeries(QPainter &p)
{
    drawStackedBar(p);
    for (Series *s : m_series)
        if (s->isVisible() && s->type() == Series::Bar)
            drawBar(p, qobject_cast<BarSeries*>(s));
    for (Series *s : m_series)
        if (s->isVisible() && s->type() == Series::Line)
            drawLine(p, qobject_cast<LineSeries*>(s));
}

/* ═══════════════════════════════════════════════════════════════
 *  散点绘制辅助函数
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::drawScatter(QPainter &p, ScatterStyle style,
                              const QPointF &center, double size, const QColor &color)
{
    if (style == ScatterNone || size <= 0) return;

    const double s = size;
    const double cx = center.x(), cy = center.y();

    p.setPen(Qt::NoPen);
    p.setBrush(color);

    switch (style) {
    case ScatterCircle:
        p.drawEllipse(center, s, s);
        /* 白色内点 */
        p.setBrush(QColor(255, 255, 255));
        p.drawEllipse(center, s * 0.45, s * 0.45);
        break;

    case ScatterDot:
        p.drawEllipse(center, s * 0.6, s * 0.6);
        break;

    case ScatterSquare: {
        QRectF r(cx - s, cy - s, s * 2, s * 2);
        p.drawRect(r);
        p.setBrush(QColor(255, 255, 255));
        QRectF ri(cx - s * 0.4, cy - s * 0.4, s * 0.8, s * 0.8);
        p.drawRect(ri);
        break;
    }

    case ScatterDiamond: {
        QPolygonF poly;
        poly << QPointF(cx, cy - s * 1.15)
             << QPointF(cx + s * 0.85, cy)
             << QPointF(cx, cy + s * 1.15)
             << QPointF(cx - s * 0.85, cy);
        p.drawPolygon(poly);
        /* 内部小菱形 */
        p.setBrush(QColor(255, 255, 255));
        QPolygonF inner;
        inner << QPointF(cx, cy - s * 0.45)
              << QPointF(cx + s * 0.35, cy)
              << QPointF(cx, cy + s * 0.45)
              << QPointF(cx - s * 0.35, cy);
        p.drawPolygon(inner);
        break;
    }

    case ScatterTriangle: {
        QPolygonF poly;
        poly << QPointF(cx, cy - s * 1.1)
             << QPointF(cx + s * 0.95, cy + s * 0.65)
             << QPointF(cx - s * 0.95, cy + s * 0.65);
        p.drawPolygon(poly);
        p.setBrush(QColor(255, 255, 255));
        QPolygonF inner;
        inner << QPointF(cx, cy - s * 0.4)
              << QPointF(cx + s * 0.38, cy + s * 0.28)
              << QPointF(cx - s * 0.38, cy + s * 0.28);
        p.drawPolygon(inner);
        break;
    }

    case ScatterCross: {
        double w = s * 0.35;
        QPen pen(color, w * 2.0, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        /* 两条对角线 */
        p.drawLine(QPointF(cx - s * 0.7, cy - s * 0.7), QPointF(cx + s * 0.7, cy + s * 0.7));
        p.drawLine(QPointF(cx + s * 0.7, cy - s * 0.7), QPointF(cx - s * 0.7, cy + s * 0.7));
        break;
    }

    case ScatterPlus: {
        double w = s * 0.35;
        QPen pen(color, w * 2.0, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawLine(QPointF(cx - s * 0.7, cy), QPointF(cx + s * 0.7, cy));
        p.drawLine(QPointF(cx, cy - s * 0.7), QPointF(cx, cy + s * 0.7));
        break;
    }

    case ScatterStar: {
        /* 五角星 */
        QPolygonF poly;
        for (int i = 0; i < 5; ++i) {
            double angleOuter = -M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
            double angleInner = angleOuter + M_PI / 5.0;
            poly << QPointF(cx + s * 1.1 * std::cos(angleOuter),
                            cy + s * 1.1 * std::sin(angleOuter));
            poly << QPointF(cx + s * 0.45 * std::cos(angleInner),
                            cy + s * 0.45 * std::sin(angleInner));
        }
        p.drawPolygon(poly);
        p.setBrush(QColor(255, 255, 255));
        p.drawEllipse(center, s * 0.25, s * 0.25);
        break;
    }

    default: break;
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  折线图绘制（使用 drawScatter）
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::drawLine(QPainter &p, LineSeries *s)
{
    if (!s || s->dataCount() < 1) return;
    const auto &data = s->data();
    QPen pen(s->color(), s->lineWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen); p.setBrush(Qt::NoBrush);
    p.save();
    p.setClipRect(m_plotArea.adjusted(-1, -1, 1, 1));

    /* 折线 */
    if (data.size() >= 2) {
        QPolygonF poly;
        for (int i = 0; i < data.size(); ++i) poly << mapToPixel(data[i].x, data[i].y);
        p.drawPolyline(poly);
    }

    /* 散点标记 */
    ScatterStyle ss = s->scatterStyle();
    double ms = s->markerSize();
    if (ss != ScatterNone && ms > 0) {
        p.setPen(Qt::NoPen);
        for (int i = 0; i < data.size(); ++i) {
            QPointF pt = mapToPixel(data[i].x, data[i].y);
            drawScatter(p, ss, pt, ms, s->color());
        }
    }

    p.restore();
}

/* ═══════════════════════════════════════════════════════════════
 *  柱状图 & 堆叠柱状图绘制（与之前一致）
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::drawBar(QPainter &p, BarSeries *s)
{
    if (!s || s->dataCount() == 0) return;
    p.save();
    p.setClipRect(m_plotArea.adjusted(-1, -1, 1, 1));
    if (s->useXY()) {
        const auto &xy = s->xyData();
        double barW = m_plotArea.width() * 0.03 * s->barWidthRatio();
        for (int i = 0; i < xy.size(); ++i) {
            QPointF top = mapToPixel(xy[i].x, xy[i].y);
            QPointF bot = mapToPixel(xy[i].x, qMax(m_yMin, 0.0));
            QRectF r(top.x() - barW/2, top.y(), barW, bot.y() - top.y());
            p.fillRect(r, s->color());
            p.setPen(s->color().darker(120)); p.drawRect(r);
        }
    } else {
        const auto &vals = s->data();
        int nCat = m_categories.isEmpty() ? vals.size() : m_categories.size();
        if (nCat == 0) { p.restore(); return; }
        QList<BarSeries*> barsInChart;
        for (Series *ss : m_series)
            if (ss->isVisible() && ss->type() == Series::Bar)
                barsInChart.append(qobject_cast<BarSeries*>(ss));
        int totalBars = barsInChart.size();
        int barIdx = barsInChart.indexOf(s);
        if (barIdx < 0) barIdx = 0;
        double catWidth = m_plotArea.width() / double(nCat);
        double groupW = catWidth * s->barWidthRatio();
        double singleW = groupW / qMax(1, totalBars);
        for (int i = 0; i < vals.size(); ++i) {
            double cx = m_plotArea.left() + (double(i) + 0.5) * catWidth;
            double barLeft = cx - groupW / 2.0 + barIdx * singleW;
            double val = vals[i];
            QPointF top = mapToPixel(0, val);
            QPointF bot = mapToPixel(0, qMax(m_yMin, 0.0));
            double barH = bot.y() - top.y();
            if (val < 0) { std::swap(top, bot); barH = -barH; }
            QRectF r(barLeft, top.y(), singleW - 1, barH);
            p.fillRect(r, s->color());
            p.setPen(s->color().darker(120)); p.drawRect(r);
        }
    }
    p.restore();
}

void ChartWidget::drawStackedBar(QPainter &p)
{
    QList<StackedBarSeries*> stacked;
    for (Series *s : m_series)
        if (s->isVisible() && s->type() == Series::StackedBar)
            stacked.append(qobject_cast<StackedBarSeries*>(s));
    if (stacked.isEmpty()) return;
    int nCat = 0;
    for (auto *sb : stacked) nCat = qMax(nCat, sb->dataCount());
    if (nCat == 0) return;
    double catWidth = m_plotArea.width() / double(nCat);
    double barW = catWidth * stacked.first()->barWidthRatio();
    p.save();
    p.setClipRect(m_plotArea.adjusted(-1, -1, 1, 1));
    for (int i = 0; i < nCat; ++i) {
        double cx = m_plotArea.left() + (double(i) + 0.5) * catWidth;
        double barLeft = cx - barW / 2.0;
        double cumNeg = 0, cumPos = 0;
        for (int si = 0; si < stacked.size(); ++si) {
            double val = (i < stacked[si]->dataCount()) ? stacked[si]->data().at(i) : 0;
            double bottomVal, topVal;
            if (val >= 0) { bottomVal = cumPos; topVal = cumPos + val; cumPos = topVal; }
            else { bottomVal = cumNeg + val; topVal = cumNeg; cumNeg = bottomVal; }
            QPointF topP = mapToPixel(0, topVal);
            QPointF botP = mapToPixel(0, bottomVal);
            QRectF r(barLeft, topP.y(), barW, botP.y() - topP.y());
            p.fillRect(r, stacked[si]->color());
            p.setPen(stacked[si]->color().darker(120)); p.drawRect(r);
        }
    }
    p.restore();
}

/* ═══════════════════════════════════════════════════════════════
 *  坐标轴绘制
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::drawAxes(QPainter &p)
{
    if (!m_axisX || !m_axisY) return;
    QFontMetrics lfm(m_axisY->labelFont());
    QFontMetrics tfm(m_axisX->titleFont());

    p.setFont(m_axisY->labelFont());
    p.setPen(QColor(60, 60, 60));
    for (double v : m_axisY->computeTicks()) {
        QPointF pt = mapToPixel(m_xMin, v);
        if (pt.y() < m_plotArea.top() - 1 || pt.y() > m_plotArea.bottom() + 1) continue;
        QString txt = m_axisY->formatValue(v);
        double tw = lfm.width(txt);
        p.drawText(QPointF(m_plotArea.left() - tw - 6, pt.y() + lfm.ascent() / 2.0 - 1), txt);
        p.setPen(QPen(QColor(180, 180, 180), 1));
        p.drawLine(QPointF(m_plotArea.left() - 3, pt.y()), QPointF(m_plotArea.left(), pt.y()));
        p.setPen(QColor(60, 60, 60));
    }
    if (!m_axisY->title().isEmpty()) {
        p.save(); p.setFont(m_axisY->titleFont()); p.setPen(QColor(50, 50, 50));
        p.translate(QPointF(m_margin, m_plotArea.center().y())); p.rotate(-90);
        QFontMetrics fm(m_axisY->titleFont());
        p.drawText(QPointF(-fm.width(m_axisY->title()) / 2.0, 0), m_axisY->title());
        p.restore();
    }

    p.setFont(m_axisX->labelFont());
    p.setPen(QColor(60, 60, 60));
    QVector<double> xTicks;
    if (!m_categories.isEmpty()) { for (int i = 0; i < m_categories.size(); ++i) xTicks.append(double(i)); }
    else { xTicks = m_axisX->computeTicks(); }
    for (double v : xTicks) {
        QPointF pt = mapToPixel(v, m_yMin);
        if (pt.x() < m_plotArea.left() - 1 || pt.x() > m_plotArea.right() + 1) continue;
        QString txt;
        if (!m_categories.isEmpty()) { int idx = int(v + 0.5); if (idx >= 0 && idx < m_categories.size()) txt = m_categories[idx]; }
        else { txt = m_axisX->formatValue(v); }
        double tw = lfm.width(txt);
        p.drawText(QPointF(pt.x() - tw / 2.0, m_plotArea.bottom() + lfm.ascent() + 4), txt);
        p.setPen(QPen(QColor(180, 180, 180), 1));
        p.drawLine(QPointF(pt.x(), m_plotArea.bottom()), QPointF(pt.x(), m_plotArea.bottom() + 3));
        p.setPen(QColor(60, 60, 60));
    }
    if (!m_axisX->title().isEmpty()) {
        p.setFont(m_axisX->titleFont()); p.setPen(QColor(50, 50, 50));
        double tw = tfm.width(m_axisX->title());
        p.drawText(QPointF(m_plotArea.center().x() - tw / 2.0, height() - m_margin), m_axisX->title());
    }
}

void ChartWidget::drawTitle(QPainter &p)
{
    if (m_title.isEmpty()) return;
    p.setFont(m_titleFont); p.setPen(QColor(40, 40, 40));
    QFontMetrics fm(m_titleFont);
    p.drawText(QPointF(m_plotArea.center().x() - fm.width(m_title) / 2.0, m_margin + fm.ascent()), m_title);
}

/* ═══════════════════════════════════════════════════════════════
 *  图例绘制（支持水平 / 垂直布局）
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::drawLegend(QPainter &p)
{
    if (m_legendPosition == LegendHidden) return;

    QList<Series*> visible;
    for (Series *s : m_series) if (s->isVisible()) visible.append(s);
    if (visible.isEmpty()) return;

    p.setFont(m_legendFont);
    QFontMetrics fm(m_legendFont);
    const int swatchW = 14, swatchH = 10, gap = 6, itemGap = 14, pad = 8;

    /* ── 计算每个条目的宽度 ── */
    QVector<int> itemWidths;
    for (Series *s : visible)
        itemWidths.append(swatchW + gap + fm.width(s->name()));

    int rowH = qMax(fm.height(), swatchH) + 4;
    int legendW, legendH;

    if (m_legendOrientation == LegendHorizontal) {
        int totalW = 0;
        for (int w : itemWidths) totalW += w;
        totalW += (visible.size() - 1) * itemGap;
        legendW = totalW + pad * 2;
        legendH = rowH + pad * 2;
    } else {
        /* 垂直：宽 = 最宽条目 + padding */
        int maxItemW = 0;
        for (int w : itemWidths) maxItemW = qMax(maxItemW, w);
        legendW = maxItemW + pad * 2;
        legendH = visible.size() * rowH + (visible.size() - 1) * 2 + pad * 2;
    }

    /* ── 定位 ── */
    double lx, ly;
    switch (m_legendPosition) {
    case LegendTop:
        lx = m_plotArea.left() + (m_plotArea.width() - legendW) / 2.0;
        ly = m_plotArea.top() + 4;
        break;
    case LegendBottom:
        lx = m_plotArea.left() + (m_plotArea.width() - legendW) / 2.0;
        ly = m_plotArea.bottom() - legendH - 4;
        break;
    case LegendTopRight: default:
        lx = m_plotArea.right() - legendW - 6;
        ly = m_plotArea.top() + 4;
        break;
    }

    QRectF legendRect(lx, ly, legendW, legendH);
    p.setPen(QPen(QColor(180, 180, 180), 1));
    p.setBrush(QColor(255, 255, 255, 235));
    p.drawRoundedRect(legendRect, 4, 4);

    /* ── 绘制各项 ── */
    if (m_legendOrientation == LegendHorizontal) {
        double cx = lx + pad;
        double cy = ly + pad;
        for (Series *s : visible) {
            QRectF swatch(cx, cy + (rowH - swatchH) / 2.0, swatchW, swatchH);
            p.fillRect(swatch, s->color());
            p.setPen(s->color().darker(130)); p.drawRect(swatch);
            p.setPen(QColor(50, 50, 50));
            p.drawText(QPointF(cx + swatchW + gap, cy + fm.ascent() + (rowH - fm.height()) / 2.0 - 1), s->name());
            cx += swatchW + gap + fm.width(s->name()) + itemGap;
        }
    } else {
        /* 垂直排列 */
        double cx = lx + pad;
        double cy = ly + pad;
        for (Series *s : visible) {
            QRectF swatch(cx, cy + (rowH - swatchH) / 2.0, swatchW, swatchH);
            p.fillRect(swatch, s->color());
            p.setPen(s->color().darker(130)); p.drawRect(swatch);
            p.setPen(QColor(50, 50, 50));
            p.drawText(QPointF(cx + swatchW + gap, cy + fm.ascent() + (rowH - fm.height()) / 2.0 - 1), s->name());
            cy += rowH + 2;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Tooltip — 查找最近数据点
 * ═══════════════════════════════════════════════════════════════ */

double ChartWidget::pixelDist(const QPointF &a, const QPointF &b) const
{
    double dx = a.x() - b.x(), dy = a.y() - b.y();
    return std::sqrt(dx * dx + dy * dy);
}

void ChartWidget::findNearest(QPoint mousePos)
{
    m_showTooltip = false;
    m_tooltipText.clear();
    m_tooltipColor = QColor();
    if (!m_tooltipEnabled || !m_plotArea.contains(mousePos)) return;

    double bestDist = 15.0;
    int bestCat = -1, bestBar = -1, bestPt = -1;
    Series *bestSeries = nullptr;

    /* ── 分类柱状图 & 堆叠柱 ── */
    int nCat = m_categories.size();
    if (nCat > 0) {
        double catWidth = m_plotArea.width() / double(nCat);
        for (int c = 0; c < nCat; ++c) {
            double cx = m_plotArea.left() + (double(c) + 0.5) * catWidth;
            if (qAbs(mousePos.x() - cx) < catWidth * 0.45) {
                /* 堆叠柱 */
                for (Series *s : m_series) {
                    if (!s->isVisible() || s->type() != Series::StackedBar) continue;
                    auto *sb = qobject_cast<StackedBarSeries*>(s);
                    if (c < sb->dataCount()) {
                        double dist = qAbs(mousePos.x() - cx);
                        if (dist < bestDist) { bestDist = dist; bestCat = c; bestSeries = s; bestBar = -2; bestPt = -1; }
                    }
                }
                /* 分类柱 */
                QList<BarSeries*> bars;
                for (Series *s : m_series)
                    if (s->isVisible() && s->type() == Series::Bar && !qobject_cast<BarSeries*>(s)->useXY())
                        bars.append(qobject_cast<BarSeries*>(s));
                if (!bars.isEmpty()) {
                    double groupW = catWidth * bars.first()->barWidthRatio();
                    double singleW = groupW / bars.size();
                    for (int bi = 0; bi < bars.size(); ++bi) {
                        if (c >= bars[bi]->dataCount()) continue;
                        double barL = cx - groupW / 2.0 + bi * singleW;
                        double val = bars[bi]->data().at(c);
                        QPointF top = mapToPixel(0, val);
                        QPointF bot = mapToPixel(0, qMax(m_yMin, 0.0));
                        double barT = qMin(top.y(), bot.y()), barB = qMax(top.y(), bot.y());
                        if (mousePos.x() >= barL && mousePos.x() <= barL + singleW - 1 &&
                            mousePos.y() >= barT && mousePos.y() <= barB) {
                            double dist = pixelDist(mousePos, QPointF(cx, (barT + barB) / 2.0));
                            if (dist < bestDist) { bestDist = dist; bestCat = c; bestSeries = bars[bi]; bestBar = bi; bestPt = -1; }
                        }
                    }
                }
                break;
            }
        }
    }

    /* ── 折线图 ── */
    for (Series *s : m_series) {
        if (!s->isVisible() || s->type() != Series::Line) continue;
        auto *ls = qobject_cast<LineSeries*>(s);
        for (int i = 0; i < ls->dataCount(); ++i) {
            QPointF pp = mapToPixel(ls->data()[i].x, ls->data()[i].y);
            double d = pixelDist(mousePos, pp);
            if (d < bestDist) { bestDist = d; bestSeries = s; bestPt = i; bestCat = -1; bestBar = -1; }
        }
    }

    /* ── 数值柱 ── */
    for (Series *s : m_series) {
        if (!s->isVisible() || s->type() != Series::Bar) continue;
        auto *bs = qobject_cast<BarSeries*>(s);
        if (!bs->useXY()) continue;
        double barW = m_plotArea.width() * 0.03 * bs->barWidthRatio();
        for (int i = 0; i < bs->xyData().size(); ++i) {
            QPointF top = mapToPixel(bs->xyData()[i].x, bs->xyData()[i].y);
            QPointF bot = mapToPixel(bs->xyData()[i].x, qMax(m_yMin, 0.0));
            QRectF r(top.x() - barW/2, top.y(), barW, bot.y() - top.y());
            if (r.adjusted(-4, -4, 4, 4).contains(mousePos)) {
                double d = pixelDist(mousePos, QPointF(top.x(), (top.y() + bot.y()) / 2.0));
                if (d < bestDist) { bestDist = d; bestSeries = s; bestPt = i; bestCat = -1; bestBar = -1; }
            }
        }
    }

    if (!bestSeries) return;
    m_showTooltip = true;
    m_mouseScreen = mousePos;
    m_tooltipColor = bestSeries->color();

    if (bestSeries->type() == Series::StackedBar && bestCat >= 0) {
        updateTooltipForStackedBar(bestCat);
    } else if (bestSeries->type() == Series::Bar && bestCat >= 0 && bestBar >= 0) {
        updateTooltipForBar(bestCat, bestBar);
    } else if (bestSeries->type() == Series::Bar && bestPt >= 0) {
        auto *bs = qobject_cast<BarSeries*>(bestSeries);
        if (bs->useXY() && bestPt < bs->xyData().size()) {
            m_tooltipPos = mapToPixel(bs->xyData()[bestPt].x, bs->xyData()[bestPt].y);
            m_tooltipText = QString("%1\nX: %2\nY: %3")
                                .arg(bestSeries->name())
                                .arg(xToDouble(bs->xyData()[bestPt].x), 0, 'f', 2)
                                .arg(bs->xyData()[bestPt].y, 0, 'f', 2);
        }
    } else if (bestSeries->type() == Series::Line && bestPt >= 0) {
        auto *ls = qobject_cast<LineSeries*>(bestSeries);
        if (bestPt < ls->dataCount()) updateTooltipForLine(ls, bestPt);
    }
}

void ChartWidget::updateTooltipForStackedBar(int catIdx)
{
    QList<StackedBarSeries*> stacked;
    for (Series *s : m_series)
        if (s->isVisible() && s->type() == Series::StackedBar)
            stacked.append(qobject_cast<StackedBarSeries*>(s));
    QString label = m_categories.value(catIdx, QString::number(catIdx));
    double sum = 0;
    QStringList lines;
    lines << QString("【%1】").arg(label);
    for (auto *sb : stacked) {
        double v = (catIdx < sb->dataCount()) ? sb->data().at(catIdx) : 0;
        lines << QString("%1|%2: %3").arg(sb->color().name()).arg(sb->name()).arg(v, 0, 'f', 2);
        sum += v;
    }
    lines << QString("#000000|合计: %1").arg(sum, 0, 'f', 2);
    m_tooltipText = lines.join("\n");

    double topVal = sum;
    QPointF topP = mapToPixel(0, topVal);
    int nCat = m_categories.size();
    double catWidth = m_plotArea.width() / double(nCat);
    m_tooltipPos = QPointF(m_plotArea.left() + (double(catIdx) + 0.5) * catWidth, topP.y());
}

void ChartWidget::updateTooltipForBar(int catIdx, int barIdx)
{
    QList<BarSeries*> bars;
    for (Series *s : m_series)
        if (s->isVisible() && s->type() == Series::Bar && !qobject_cast<BarSeries*>(s)->useXY())
            bars.append(qobject_cast<BarSeries*>(s));
    if (barIdx < 0 || barIdx >= bars.size()) return;
    BarSeries *bs = bars[barIdx];
    if (catIdx >= bs->dataCount()) return;
    QString label = m_categories.value(catIdx, QString::number(catIdx));
    double val = bs->data().at(catIdx);
    m_tooltipText = QString("%1|%2\n#000000|%3: %4").arg(bs->color().name()).arg(bs->name()).arg(label).arg(val, 0, 'f', 2);

    QPointF top = mapToPixel(0, val);
    int nCat = m_categories.size();
    double catWidth = m_plotArea.width() / double(nCat);
    m_tooltipPos = QPointF(m_plotArea.left() + (double(catIdx) + 0.5) * catWidth, top.y());
}

void ChartWidget::updateTooltipForLine(LineSeries *ls, int ptIdx)
{
    if (ptIdx < 0 || ptIdx >= ls->dataCount()) return;
    const DataPoint &dp = ls->data().at(ptIdx);
    m_tooltipPos = mapToPixel(dp.x, dp.y);
    m_tooltipColor = ls->color();

    QString xStr;
    if (!m_categories.isEmpty()) {
        int ci = int(dp.x + 0.5);
        xStr = m_categories.value(ci, QString::number(ci));
    } else if (qobject_cast<DateTimeAxis*>(m_axisX)) {
        xStr = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(dp.x)).toString("HH:mm:ss");
    } else if (qobject_cast<DateAxis*>(m_axisX)) {
        xStr = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(dp.x)).date().toString("yyyy-MM-dd");
    } else {
        xStr = QString::number(dp.x, 'f', 2);
    }

    /* 每行格式: "#颜色值|文字" */
    m_tooltipText = QString("%1|%2\n#000000|X: %3\n#000000|Y: %4")
                        .arg(ls->color().name()).arg(ls->name()).arg(xStr).arg(dp.y, 0, 'f', 2);
}

/* ═══════════════════════════════════════════════════════════════
 *  Tooltip 绘制（每行前带颜色方块）
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::drawTooltip(QPainter &p)
{
    if (!m_showTooltip || m_tooltipText.isEmpty()) return;

    p.setFont(m_tooltipFont);
    QFontMetrics fm(m_tooltipFont);
    QStringList rawLines = m_tooltipText.split('\n');

    /* 解析每行的 "#颜色|文字" 或 "纯文字" */
    struct TLine { QColor color; QString text; };
    QList<TLine> lines;
    for (const QString &raw : rawLines) {
        TLine tl;
        int barPos = raw.indexOf('|');
        if (barPos > 0 && raw.left(barPos).startsWith('#') && raw.left(barPos).length() == 7) {
            tl.color = QColor(raw.left(barPos));
            tl.text  = raw.mid(barPos + 1);
        } else {
            tl.color = QColor();   // 无颜色
            tl.text  = raw;
        }
        lines.append(tl);
    }

    /* 计算尺寸 */
    const int padX = 10, padY = 6, spacing = 3;
    const int swatchW = 10, swatchH = 10, swatchGap = 6;

    int maxTextW = 0;
    for (const TLine &tl : lines)
        maxTextW = qMax(maxTextW, fm.width(tl.text));

    int textH = lines.size() * fm.height() + (lines.size() - 1) * spacing;
    int tipW  = maxTextW + padX * 2 + swatchW + swatchGap;
    int tipH  = textH + padY * 2;

    /* 定位 */
    double tx = m_tooltipPos.x() + 14;
    double ty = m_tooltipPos.y() - tipH - 10;
    if (tx + tipW > width() - 4)  tx = m_tooltipPos.x() - tipW - 14;
    if (ty < 4)                    ty = m_tooltipPos.y() + 14;
    if (tx < 4)                    tx = 4;
    if (ty + tipH > height() - 4)  ty = height() - tipH - 4;

    QRectF tipRect(tx, ty, tipW, tipH);

    /* 阴影 */
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawRoundedRect(tipRect.translated(2, 2), 6, 6);

    /* 背景 */
    p.setPen(QPen(QColor(160, 160, 160), 1));
    p.setBrush(QColor(255, 255, 255, 245));
    p.drawRoundedRect(tipRect, 6, 6);

    /* 每行：颜色方块 + 文字 */
    double textX = tx + padX;
    double textY = ty + padY + fm.ascent();
    for (int i = 0; i < lines.size(); ++i) {
        double lineY = textY + i * (fm.height() + spacing);

        /* 颜色方块 */
        QColor c = lines[i].color;
        if (c.isValid()) {
            QRectF swatch(textX, lineY - fm.ascent() + (fm.height() - swatchH) / 2.0,
                          swatchW, swatchH);
            p.fillRect(swatch, c);
            p.setPen(c.darker(140));
            p.setBrush(Qt::NoBrush);
            p.drawRect(swatch);
        }

        /* 文字 */
        p.setPen(QColor(40, 40, 40));
        p.drawText(QPointF(textX + swatchW + swatchGap, lineY), lines[i].text);
    }

    /* 指示线 */
    p.setPen(QPen(QColor(180, 180, 180), 1, Qt::DotLine));
    p.drawLine(m_tooltipPos, QPointF(tipRect.center().x(),
                                     tipRect.contains(m_tooltipPos) ? tipRect.bottom() : tipRect.top()));
}

/* ═══════════════════════════════════════════════════════════════
 *  鼠标事件
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_plotArea.contains(event->pos())) {
        m_panning = true;
        m_panStart = event->pos();
        m_panXMin = m_xMin; m_panXMax = m_xMax;
        m_panYMin = m_yMin; m_panYMax = m_yMax;
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning && m_axisX && m_axisY) {
        QPoint delta = event->pos() - m_panStart;
        double xr = m_panXMax - m_panXMin, yr = m_panYMax - m_panYMin;
        if (!qFuzzyIsNull(xr) && !qFuzzyIsNull(yr)) {
            double dxData = -delta.x() / m_plotArea.width() * xr;
            double dyData =  delta.y() / m_plotArea.height() * yr;
            m_axisX->setRange(m_panXMin + dxData, m_panXMax + dxData);
            m_axisY->setRange(m_panYMin + dyData, m_panYMax + dyData);
            syncRangeFromAxes();
            m_rescaleMode = Manual;
            m_layoutDirty = true;
        }
    }
    findNearest(event->pos());
    update();
    QWidget::mouseMoveEvent(event);
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseReleaseEvent(event);
}

void ChartWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    fitToData();
}

void ChartWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_axisX || !m_axisY) return;
    if (!m_plotArea.contains(event->pos())) { QWidget::wheelEvent(event); return; }
    double factor = (event->angleDelta().y() > 0) ? 0.85 : 1.18;
    double xr = m_xMax - m_xMin, yr = m_yMax - m_yMin;
    if (qFuzzyIsNull(xr) || qFuzzyIsNull(yr)) return;
    double dataX = m_xMin + (event->pos().x() - m_plotArea.left()) / m_plotArea.width() * xr;
    double dataY = m_yMin + (m_plotArea.bottom() - event->pos().y()) / m_plotArea.height() * yr;
    double newXr = xr * factor, ratioX = (dataX - m_xMin) / xr;
    double newXMin = dataX - ratioX * newXr, newXMax = newXMin + newXr;
    double newYr = yr * factor, ratioY = (dataY - m_yMin) / yr;
    double newYMin = dataY - ratioY * newYr, newYMax = newYMin + newYr;
    m_axisX->setRange(newXMin, newXMax); m_axisY->setRange(newYMin, newYMax);
    syncRangeFromAxes();
    m_rescaleMode = Manual;
    m_layoutDirty = true;
    update();
    event->accept();
}

/* ═══════════════════════════════════════════════════════════════
 *  右键菜单（图例布局切换）
 * ═══════════════════════════════════════════════════════════════ */

void ChartWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    menu.addAction("适配数据 (Fit to Data)", this, SLOT(fitToData()));
    menu.addSeparator();

    /* 图例位置 */
    QMenu *legendPosMenu = menu.addMenu("图例位置");
    QAction *actLegTR  = legendPosMenu->addAction("右上角");
    QAction *actLegTop = legendPosMenu->addAction("顶部居中");
    QAction *actLegBot = legendPosMenu->addAction("底部居中");
    QAction *actLegHid = legendPosMenu->addAction("隐藏");
    actLegTR->setCheckable(true);  actLegTop->setCheckable(true);
    actLegBot->setCheckable(true); actLegHid->setCheckable(true);
    switch (m_legendPosition) {
    case LegendTopRight: actLegTR->setChecked(true); break;
    case LegendTop:      actLegTop->setChecked(true); break;
    case LegendBottom:   actLegBot->setChecked(true); break;
    case LegendHidden:   actLegHid->setChecked(true); break;
    }

    /* 图例布局 */
    QMenu *legendOriMenu = menu.addMenu("图例布局");
    QAction *actHori = legendOriMenu->addAction("水平排列");
    QAction *actVert = legendOriMenu->addAction("垂直排列");
    actHori->setCheckable(true);
    actVert->setCheckable(true);
    actHori->setChecked(m_legendOrientation == LegendHorizontal);
    actVert->setChecked(m_legendOrientation == LegendVertical);

    menu.addSeparator();

    QAction *actGrid = menu.addAction("显示网格");
    actGrid->setCheckable(true);
    actGrid->setChecked(m_axisY ? m_axisY->isGridVisible() : true);

    menu.addSeparator();
    menu.addAction("放大", [this]{ zoom(0.7); });
    menu.addAction("缩小", [this]{ zoom(1.4); });

    QAction *chosen = menu.exec(event->globalPos());
    if (!chosen) return;

    if      (chosen == actLegTR)  { setLegendPosition(LegendTopRight); refresh(); }
    else if (chosen == actLegTop) { setLegendPosition(LegendTop);      refresh(); }
    else if (chosen == actLegBot) { setLegendPosition(LegendBottom);   refresh(); }
    else if (chosen == actLegHid) { setLegendPosition(LegendHidden);   refresh(); }
    else if (chosen == actHori)   { setLegendOrientation(LegendHorizontal); refresh(); }
    else if (chosen == actVert)   { setLegendOrientation(LegendVertical);   refresh(); }
    else if (chosen == actGrid)   {
        bool vis = actGrid->isChecked();
        if (m_axisX) m_axisX->setGridVisible(vis);
        if (m_axisY) m_axisY->setGridVisible(vis);
        update();
    }
}

void ChartWidget::leaveEvent(QEvent *event)
{
    m_showTooltip = false;
    update();
    QWidget::leaveEvent(event);
}
