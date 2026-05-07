#include "chartwidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPainterPath>
#include <QLineF>
#include <QPolygon>
#include <QtMath>
#include <algorithm>

// ------------------------------------------------------------------------
// Color palette for series
// ------------------------------------------------------------------------
static const QColor kDefaultPalette[] = {
    QColor( 54, 162, 235),   // blue
    QColor(255,  99, 132),   // red
    QColor( 75, 192, 192),   // green
    QColor(255, 159,  64),   // orange
    QColor(153, 102, 255),   // purple
    QColor(255, 205,  86),   // yellow
    QColor(201, 203, 207),   // grey
    QColor( 34, 198, 129),   // teal
};
static const int kPaletteSize = sizeof(kDefaultPalette) / sizeof(kDefaultPalette[0]);
static int s_colorIndex = 0;

static QColor nextColor() {
    return kDefaultPalette[s_colorIndex++ % kPaletteSize];
}

// ------------------------------------------------------------------------
// Series
// ------------------------------------------------------------------------
Series::Series(const QString &name, QObject *parent)
    : QObject(parent), m_name(name), m_color(nextColor())
{
}

Series::~Series() = default;

void Series::setName(const QString &name) { m_name = name; }
void Series::setColor(const QColor &c) { m_color = c; emit dataChanged(); }
void Series::setVisible(bool v) { m_visible = v; emit dataChanged(); }

// ------------------------------------------------------------------------
// LineSeries
// ------------------------------------------------------------------------
LineSeries::LineSeries(const QString &name, QObject *parent)
    : Series(name, parent) {}

void LineSeries::append(double key, double value) {
    m_data.append({key, value});
    emit dataChanged();
}

void LineSeries::append(const QDateTime &time, double value) {
    append(time.toSecsSinceEpoch(), value);
}

void LineSeries::removeAt(int index) {
    if (index >= 0 && index < m_data.size()) {
        m_data.removeAt(index);
        emit dataChanged();
    }
}

// ------------------------------------------------------------------------
// BarSeries
// ------------------------------------------------------------------------
BarSeries::BarSeries(const QString &name, QObject *parent)
    : Series(name, parent) {}

// ------------------------------------------------------------------------
// StackedBarSeries
// ------------------------------------------------------------------------
StackedBarSeries::StackedBarSeries(const QString &name, QObject *parent)
    : Series(name, parent) {}

// ------------------------------------------------------------------------
// RangeBarSeries
// ------------------------------------------------------------------------
RangeBarSeries::RangeBarSeries(const QString &name, QObject *parent)
    : Series(name, parent) {}

// ------------------------------------------------------------------------
// Axis
// ------------------------------------------------------------------------
Axis::Axis(bool vertical, QObject *parent)
    : QObject(parent), m_vertical(vertical) {}

void Axis::setRange(double min, double max) {
    if (qFuzzyCompare(min, max))
        max = min + 1.0;
    m_min = qMin(min, max);
    m_max = qMax(min, max);
    recalculateTicks();
    emit rangeChanged();
}

double Axis::coordToPixel(double value) const {
    if (qFuzzyCompare(m_max, m_min))
        return m_rect.left();
    double ratio = (value - m_min) / (m_max - m_min);
    if (m_vertical) {
        // Y axis: data min → bottom of rect, data max → top of rect
        return m_rect.bottom() - ratio * m_rect.height();
    } else {
        return m_rect.left() + ratio * m_rect.width();
    }
}

double Axis::pixelToCoord(double pixel) const {
    if (qFuzzyCompare(m_max, m_min))
        return m_min;
    double ratio;
    if (m_vertical) {
        ratio = (m_rect.bottom() - pixel) / m_rect.height();
    } else {
        ratio = (pixel - m_rect.left()) / m_rect.width();
    }
    return m_min + ratio * (m_max - m_min);
}

void Axis::recalculateTicks() {
    m_ticks.clear();
    m_subTicks.clear();
    m_tickLabels.clear();

    double range = m_max - m_min;
    if (range <= 0 || m_tickCount < 2) return;

    if (m_type == AxisType::Date) {
        // Date axis: nice time intervals
        double step;
        if (range < 300)            step = 30;         // 30 sec
        else if (range < 1800)      step = 300;        // 5 min
        else if (range < 7200)      step = 1800;       // 30 min
        else if (range < 43200)     step = 7200;       // 2 hr
        else if (range < 172800)    step = 43200;      // 12 hr
        else if (range < 604800)    step = 86400;      // 1 day
        else if (range < 2592000)   step = 604800;     // 1 week
        else if (range < 31536000)  step = 2592000;    // ~1 month
        else                        step = 31536000;   // ~1 year

        double start = ceil(m_min / step) * step;
        if (start - step > m_min) start -= step;
        for (double t = start; t <= m_max; t += step)
            m_ticks.append(t);

        // Sub-ticks: divide step into m_subTickCount+1 parts
        double subStep = step / (m_subTickCount + 1);
        for (double t = m_min - std::fmod(m_min, step) + subStep; t < m_max; t += subStep) {
            if (t > m_min)
                m_subTicks.append(t);
        }
    } else {
        // Numeric axis: nice-number algorithm
        double roughStep = range / qMax(m_tickCount - 1, 1);
        double magnitude = std::pow(10.0, std::floor(std::log10(roughStep)));
        double normalized = roughStep / magnitude;
        double niceStep;
        if (normalized < 1.5)      niceStep = 1.0;
        else if (normalized < 3.5) niceStep = 2.0;
        else if (normalized < 7.5) niceStep = 5.0;
        else                       niceStep = 10.0;
        niceStep *= magnitude;

        double start = std::ceil(m_min / niceStep) * niceStep;
        for (double t = start; t <= m_max + niceStep * 0.001; t += niceStep)
            m_ticks.append(t);

        // Sub-ticks
        double subStep = niceStep / (m_subTickCount + 1);
        if (subStep > 0) {
            for (double t = start - niceStep + subStep; t <= m_max; t += subStep) {
                if (t > m_min)
                    m_subTicks.append(t);
            }
        }
    }

    // Generate labels
    for (double t : m_ticks)
        m_tickLabels.append(tickLabelText(t));
}

QString Axis::tickLabelText(double value) const {
    if (m_type == AxisType::Date) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(qint64(value));
        switch (m_dateFormat) {
        case DateFormat::MMdd:     return dt.toString("MM-dd");
        case DateFormat::HHmm:     return dt.toString("HH:mm");
        case DateFormat::MMddHHmm: return dt.toString("MM-dd HH:mm");
        }
    }
    // Numeric
    return QString::number(value, 'f', 2);
}

void Axis::drawGrid(QPainter *p) const {
    p->save();
    QPen gridPen(m_gridColor, 0.5);
    p->setPen(gridPen);
    QRectF gr = m_gridRect;
    for (double t : m_ticks) {
        if (m_vertical) {
            double y = coordToPixel(t);
            p->drawLine(QPointF(gr.left(), y), QPointF(gr.right(), y));
        } else {
            double x = coordToPixel(t);
            p->drawLine(QPointF(x, gr.top()), QPointF(x, gr.bottom()));
        }
    }
    p->restore();
}

void Axis::drawSubGrid(QPainter *p) const {
    p->save();
    QPen subPen(QColor(m_gridColor.red(), m_gridColor.green(), m_gridColor.blue(), 100), 0.5);
    p->setPen(subPen);
    QRectF gr = m_gridRect;
    for (double t : m_subTicks) {
        if (m_vertical) {
            double y = coordToPixel(t);
            p->drawLine(QPointF(gr.left(), y), QPointF(gr.right(), y));
        } else {
            double x = coordToPixel(t);
            p->drawLine(QPointF(x, gr.top()), QPointF(x, gr.bottom()));
        }
    }
    p->restore();
}

void Axis::drawAxis(QPainter *p) const {
    p->save();
    // Axis line
    QPen axisPen(m_tickColor, 1.0);
    p->setPen(axisPen);
    if (m_vertical) {
        p->drawLine(QPointF(m_rect.right(), m_rect.top()),
                    QPointF(m_rect.right(), m_rect.bottom()));
        // Tick marks (right side for left Y axis)
        double tickLen = 5;
        for (double t : m_ticks) {
            double y = coordToPixel(t);
            p->drawLine(QPointF(m_rect.right() - tickLen, y),
                        QPointF(m_rect.right(), y));
        }
        // Sub-ticks
        double subTickLen = 3;
        QPen subPen(m_subTickColor, 1.0);
        p->setPen(subPen);
        for (double t : m_subTicks) {
            double y = coordToPixel(t);
            p->drawLine(QPointF(m_rect.right() - subTickLen, y),
                        QPointF(m_rect.right(), y));
        }
    } else {
        p->drawLine(QPointF(m_rect.left(), m_rect.top()),
                    QPointF(m_rect.right(), m_rect.top()));
        double tickLen = 5;
        for (double t : m_ticks) {
            double x = coordToPixel(t);
            p->drawLine(QPointF(x, m_rect.top()),
                        QPointF(x, m_rect.top() + tickLen));
        }
        double subTickLen = 3;
        QPen subPen(m_subTickColor, 1.0);
        p->setPen(subPen);
        for (double t : m_subTicks) {
            double x = coordToPixel(t);
            p->drawLine(QPointF(x, m_rect.top()),
                        QPointF(x, m_rect.top() + subTickLen));
        }
    }
    p->restore();
}

void Axis::drawLabels(QPainter *p) const {
    p->save();
    p->setPen(m_tickColor);
    QFont labelFont("Arial", 8);
    p->setFont(labelFont);
    for (int i = 0; i < m_ticks.size(); ++i) {
        const QString &text = m_tickLabels[i];
        QFontMetrics fm(labelFont);
        if (m_vertical) {
            double y = coordToPixel(m_ticks[i]);
            QRectF textRect(m_rect.left() - 5, y - fm.height() / 2.0,
                            m_rect.width() - 5, fm.height());
            p->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);
        } else {
            double x = coordToPixel(m_ticks[i]);
            QRectF textRect(x - 40, m_rect.top() + 6, 80, fm.height());
            p->drawText(textRect, Qt::AlignCenter, text);
        }
    }
    p->restore();
}

void Axis::drawTitle(QPainter *p) const {
    if (m_title.isEmpty()) return;
    p->save();
    p->setPen(m_titleColor);
    QFont titleFont("Arial", 9, QFont::Bold);
    p->setFont(titleFont);
    QFontMetrics fm(titleFont);
    if (m_vertical) {
        // Draw Y axis title vertically
        p->translate(m_rect.left() - fm.height() - 10, m_rect.center().y());
        p->rotate(-90);
        p->drawText(QRectF(-500, -fm.height() / 2.0, 1000, fm.height()),
                    Qt::AlignCenter, m_title);
    } else {
        p->drawText(QRectF(m_rect.left(), m_rect.top() + 25,
                           m_rect.width(), fm.height()),
                    Qt::AlignCenter, m_title);
    }
    p->restore();
}

// ------------------------------------------------------------------------
// ChartModel
// ------------------------------------------------------------------------
ChartModel::ChartModel(QObject *parent)
    : QObject(parent)
{
}

ChartModel::~ChartModel() {
    qDeleteAll(m_series);
    m_series.clear();
    qDeleteAll(m_axes);
    m_axes.clear();
}

void ChartModel::addSeries(Series *series) {
    if (!series || m_series.contains(series)) return;
    series->setParent(this);
    series->setIndex(m_series.size());
    m_series.append(series);
    connectSeries(series);
    emit seriesAdded(series);
    emit dataChanged();
}

void ChartModel::removeSeries(Series *series) {
    if (!series || !m_series.contains(series)) return;
    m_series.removeOne(series);
    series->setParent(nullptr);
    emit seriesRemoved(series);
    emit dataChanged();
}

void ChartModel::clearSeries() {
    for (auto *s : m_series) {
        emit seriesRemoved(s);
        delete s;
    }
    m_series.clear();
    emit dataChanged();
}

void ChartModel::addAxis(Axis *axis) {
    if (!axis || m_axes.contains(axis)) return;
    m_axes.append(axis);
    emit axisAdded(axis);
}

void ChartModel::removeAxis(Axis *axis) {
    if (!axis || !m_axes.contains(axis)) return;
    m_axes.removeOne(axis);
    emit axisRemoved(axis);
}

void ChartModel::setTheme(const ChartTheme &theme) {
    m_theme = theme;
    emit themeChanged();
    emit dataChanged();
}

void ChartModel::connectSeries(Series *s) {
    connect(s, &Series::dataChanged, this, &ChartModel::dataChanged);
}

// ------------------------------------------------------------------------
// ChartLayout
// ------------------------------------------------------------------------
ChartLayout::ChartLayout(QObject *parent)
    : QObject(parent) {}

ChartLayout::LayoutInfo ChartLayout::calculate(
    const QSize &widgetSize,
    const QString &title,
    const QFont &titleFont,
    Axis *xAxis,
    Axis *yAxis,
    const Legend &legend,
    const QList<Series*> &series) const
{
    ChartLayout::LayoutInfo info;
    Q_UNUSED(xAxis)
    Q_UNUSED(yAxis)

    double w = widgetSize.width();
    double h = widgetSize.height();
    info.margins = QMarginsF(5, 5, 5, 5);

    // Title area
    double top = info.margins.top();
    if (!title.isEmpty()) {
        QFontMetrics fm(titleFont);
        int th = fm.height() + 8;
        info.titleRect = QRectF(5, top, w - 10, th);
        top = info.titleRect.bottom() + 5;
    }

    // Estimate axis label sizes
    int yLabelWidth = 55;  // enough for typical numbers
    int xLabelHeight = 22; // enough for date/number labels
    int yTitleWidth = yAxis && !yAxis->title().isEmpty() ? 18 : 0;
    int xTitleHeight = xAxis && !xAxis->title().isEmpty() ? 18 : 0;

    // Legend size estimate
    QSizeF legendSize = measureLegend(legend, series);

    // Legend above chart
    if (legend.position == Legend::AboveChart && !series.isEmpty()) {
        info.legendRect = QRectF(
            (w - legendSize.width()) / 2.0,
            top + m_legendMargin,
            legendSize.width(),
            legendSize.height());
        top = info.legendRect.bottom() + m_legendMargin;
    }

    // Plot area
    double left = info.margins.left() + yLabelWidth + yTitleWidth + m_plotMargin;
    double bottom = h - info.margins.bottom() - xLabelHeight - xTitleHeight - m_plotMargin;

    info.yAxisRect = QRectF(0, top, left, bottom - top);
    info.xAxisRect = QRectF(left, bottom, w - left - info.margins.right(), xLabelHeight + xTitleHeight + 5);
    info.chartArea = QRectF(left, top, w - left - info.margins.right(), bottom - top);

    // Legend below chart
    if (legend.position == Legend::BelowChart && !series.isEmpty()) {
        info.legendRect = QRectF(
            (w - legendSize.width()) / 2.0,
            bottom + 10,
            legendSize.width(),
            legendSize.height());
    }

    // Set axis geometries (for drawing grid/labels)
    if (xAxis) {
        xAxis->setRect(info.xAxisRect);
        xAxis->setGridRect(info.chartArea);
    }
    if (yAxis) {
        yAxis->setRect(info.yAxisRect);
        yAxis->setGridRect(info.chartArea);
    }

    return info;
}

QSizeF ChartLayout::measureLegend(const Legend &legend, const QList<Series*> &series) const {
    if (series.isEmpty()) return QSizeF(0, 0);

    QFontMetrics fm(legend.font);
    double maxTextWidth = 0;
    int iconWidth = 16;
    int itemSpacing = 4;
    int visibleCount = 0;
    for (auto *s : series) {
        if (!s->isVisible() || !s->showInLegend()) continue;
        ++visibleCount;
        double tw = fm.horizontalAdvance(s->name());
        if (tw > maxTextWidth) maxTextWidth = tw;
    }
    if (visibleCount == 0) return QSizeF(0, 0);

    double itemWidth = iconWidth + itemSpacing + maxTextWidth + 10;
    double itemHeight = fm.height() + 4;

    if (legend.orientation == Legend::Horizontal) {
        double totalW = visibleCount * itemWidth + 10;
        return QSizeF(totalW, itemHeight + 6);
    } else {
        double totalH = visibleCount * itemHeight + 6;
        return QSizeF(itemWidth + 10, totalH);
    }
}

// ------------------------------------------------------------------------
// PaintBuffer
// ------------------------------------------------------------------------
PaintBuffer::PaintBuffer() {}

PaintBuffer::~PaintBuffer() {
    endPainting();
}

void PaintBuffer::resize(const QSize &size, double devicePixelRatio) {
    if (m_pixmap.size() == size)
        return;
    m_pixmap = QPixmap(size * devicePixelRatio);
    m_pixmap.setDevicePixelRatio(devicePixelRatio);
}

QPainter *PaintBuffer::beginPainting(const QColor &clearColor) {
    endPainting();
    m_pixmap.fill(clearColor);
    m_activePainter = new QPainter(&m_pixmap);
    m_activePainter->setRenderHint(QPainter::Antialiasing);
    return m_activePainter;
}

void PaintBuffer::endPainting() {
    if (m_activePainter) {
        m_activePainter->end();
        delete m_activePainter;
        m_activePainter = nullptr;
    }
}

void PaintBuffer::paint(QPainter *target) const {
    if (!m_pixmap.isNull())
        target->drawPixmap(0, 0, m_pixmap);
}

// ------------------------------------------------------------------------
// ChartWidget
// ------------------------------------------------------------------------
ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    setMinimumSize(300, 200);

    m_model  = new ChartModel(this);
    m_layout = new ChartLayout(this);
    m_buffer = new PaintBuffer();

    m_xAxis = new Axis(false, this);
    m_yAxis = new Axis(true, this);
    m_xAxis->setRange(0, 100);
    m_yAxis->setRange(0, 100);

    m_model->addAxis(m_xAxis);
    m_model->addAxis(m_yAxis);

    // Connect model signals → refresh
    connect(m_model, &ChartModel::dataChanged, this, [this]() {
        m_dirty = true;
        update();
    });
    connect(m_model, &ChartModel::titleChanged, this, [this]() {
        m_dirty = true;
        update();
    });
}

ChartWidget::~ChartWidget() {
    delete m_buffer;
}

void ChartWidget::addSeries(Series *series) {
    m_model->addSeries(series);
}

void ChartWidget::removeSeries(Series *series) {
    m_model->removeSeries(series);
}

void ChartWidget::clearSeries() {
    m_model->clearSeries();
    m_hoveredSeriesIdx = -1;
    m_hoveredPointIdx = -1;
    m_dirty = true;
    update();
}

void ChartWidget::setTheme(const ChartTheme &theme) {
    m_model->setTheme(theme);
}

void ChartWidget::refresh() {
    m_dirty = true;
    update();
}

QPixmap ChartWidget::exportToPixmap(const QSize &size) {
    QPixmap pm(size);
    pm.fill(m_model->theme().background);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);
    renderChart(painter, size);
    painter.end();
    return pm;
}

// ------------------------------------------------------------------------
// Paint Event
// ------------------------------------------------------------------------
void ChartWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    if (m_dirty || m_buffer->size() != size()) {
        m_buffer->resize(size(), devicePixelRatio());
        QPainter *bufPainter = m_buffer->beginPainting(m_model->theme().background);
        if (bufPainter && bufPainter->isActive()) {
            bufPainter->setRenderHint(QPainter::Antialiasing);
            renderChart(*bufPainter, size());
        }
        m_buffer->endPainting();
        m_dirty = false;
    }
    QPainter widgetPainter(this);
    widgetPainter.setRenderHint(QPainter::Antialiasing);
    m_buffer->paint(&widgetPainter);
}

// ------------------------------------------------------------------------
// Resize
// ------------------------------------------------------------------------
void ChartWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_dirty = true;
}

// ------------------------------------------------------------------------
// renderChart — main rendering pipeline
// ------------------------------------------------------------------------
void ChartWidget::renderChart(QPainter &p, const QSize &size) {
    // 1. Calculate layout
    auto info = m_layout->calculate(
        size, m_model->title(), m_titleFont,
        m_xAxis, m_yAxis, m_legend, m_model->seriesList());
    QRectF plotArea = info.chartArea;
    if (plotArea.isEmpty()) return;

    // 2. Draw grid (behind everything)
    renderGrid(p, plotArea);

    // 3. Draw line series (fills first, then lines/scatters)
    for (auto *s : m_model->seriesList()) {
        if (!s->isVisible()) continue;
        if (auto *ls = qobject_cast<LineSeries*>(s))
            renderLineSeries(p, plotArea, ls);
    }

    // 4. Draw stacked bars
    renderStackedBarSeries(p, plotArea);

    // 5. Draw grouped bars
    renderBarSeries(p, plotArea);

    // 6. Draw range bars
    renderRangeBarSeries(p, plotArea);

    // 7. Draw axes on top
    renderAxes(p, plotArea);

    // 7. Draw title
    renderTitle(p, info.titleRect);

    // 8. Draw legend
    renderLegend(p, info.legendRect);
}

// ------------------------------------------------------------------------
// Render Background
// ------------------------------------------------------------------------
void ChartWidget::renderBackground(QPainter &p, const QRectF &area) {
    p.fillRect(area, m_model->theme().background);
}

// ------------------------------------------------------------------------
// Render Title
// ------------------------------------------------------------------------
void ChartWidget::renderTitle(QPainter &p, const QRectF &rect) {
    QString title = m_model->title();
    if (title.isEmpty() || rect.isEmpty()) return;
    p.save();
    p.setPen(m_model->theme().titleColor);
    p.setFont(m_titleFont);
    p.drawText(rect, Qt::AlignCenter, title);
    p.restore();
}

// ------------------------------------------------------------------------
// Render Legend
// ------------------------------------------------------------------------
void ChartWidget::renderLegend(QPainter &p, const QRectF &rect) {
    if (rect.isEmpty()) return;
    auto series = m_model->seriesList();
    if (series.isEmpty()) return;

    p.save();
    // Background
    p.setBrush(m_legend.backgroundColor);
    p.setPen(QPen(m_legend.borderColor, 1));
    p.drawRoundedRect(rect, 4, 4);

    p.setFont(m_legend.font);
    QFontMetrics fm(m_legend.font);
    int iconW = 14;
    int spacing = 4;
    int x0 = int(rect.left() + 6);
    int y0 = int(rect.top() + 4);

    int visibleCount = 0;
    for (auto *s : series)
        if (s->isVisible() && s->showInLegend()) ++visibleCount;

    if (m_legend.orientation == Legend::Horizontal) {
        int x = x0;
        for (auto *s : series) {
            if (!s->isVisible() || !s->showInLegend()) continue;
            // Icon
            p.fillRect(QRect(x, y0 + 2, iconW, iconW), s->color());
            // Text
            p.setPen(m_legend.textColor);
            int tw = fm.horizontalAdvance(s->name());
            QRectF tr(x + iconW + spacing, y0, tw + 4, fm.height() + 2);
            p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, s->name());
            x += iconW + spacing + tw + 10;
        }
    } else {
        int y = y0;
        for (auto *s : series) {
            if (!s->isVisible() || !s->showInLegend()) continue;
            p.fillRect(QRect(x0, y + 2, iconW, iconW), s->color());
            p.setPen(m_legend.textColor);
            QRectF tr(x0 + iconW + spacing, y, rect.right() - x0 - iconW - spacing - 4, fm.height() + 2);
            p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, s->name());
            y += fm.height() + 6;
        }
    }
    p.restore();
}

// ------------------------------------------------------------------------
// Render Grid
// ------------------------------------------------------------------------
void ChartWidget::renderGrid(QPainter &p, const QRectF &plotArea) {
    Q_UNUSED(plotArea)
    // Sub-grid
    m_xAxis->drawSubGrid(&p);
    m_yAxis->drawSubGrid(&p);

    // Major grid
    m_xAxis->drawGrid(&p);
    m_yAxis->drawGrid(&p);
}

// ------------------------------------------------------------------------
// Render Axes
// ------------------------------------------------------------------------
void ChartWidget::renderAxes(QPainter &p, const QRectF &plotArea) {
    Q_UNUSED(plotArea)
    m_xAxis->drawAxis(&p);
    m_xAxis->drawLabels(&p);
    m_xAxis->drawTitle(&p);

    m_yAxis->drawAxis(&p);
    m_yAxis->drawLabels(&p);
    m_yAxis->drawTitle(&p);
}

// ------------------------------------------------------------------------
// Adaptive polyline simplification (vertical band filter)
// Keeps one point per distinct pixel column. For dense data this preserves
// visual shape while dramatically reducing vertex count.
// ------------------------------------------------------------------------
static QVector<QPointF> simplifyPolyline(const QVector<QPointF> &pts, int pixelWidth) {
    if (pts.size() <= pixelWidth * 2)
        return pts;
    QVector<QPointF> result;
    result.reserve(pixelWidth * 2);
    int lastCol = int(pts.first().x());
    result.append(pts.first());
    for (int i = 1; i < pts.size() - 1; ++i) {
        int col = int(pts[i].x());
        if (col == lastCol) continue;
        lastCol = col;
        result.append(pts[i]);
    }
    result.append(pts.last());
    return result;
}

// ------------------------------------------------------------------------
// Generate pixel coordinates for line series (view-culled, sorted/unsorted)
// ------------------------------------------------------------------------
QVector<QPointF> ChartWidget::generateLineSeriesPixels(
    const QRectF &plotArea,
    const QVector<LineSeries::DataPoint> &data,
    int dataSize) const
{
    double xMin = m_xAxis->min(), xMax = m_xAxis->max();
    int b = 0, e = dataSize;
    bool sorted = (dataSize < 2 || data.last().key >= data.first().key);
    if (sorted) {
        auto lessDP = [](const LineSeries::DataPoint &dp, double key) { return dp.key < key; };
        auto lessKey = [](double key, const LineSeries::DataPoint &dp) { return key < dp.key; };
        b = int(std::lower_bound(data.begin(), data.end(), xMin, lessDP) - data.begin());
        e = int(std::upper_bound(data.begin(), data.end(), xMax, lessKey) - data.begin());
        if (b >= e) return {};
    }

    QVector<QPointF> px;
    px.reserve(sorted ? (e - b) : dataSize);
    if (sorted) {
        for (int i = b; i < e; ++i)
            px.append(QPointF(m_xAxis->coordToPixel(data[i].key),
                              m_yAxis->coordToPixel(data[i].value)));
    } else {
        for (const auto &dp : data) {
            if (dp.key < xMin || dp.key > xMax) continue;
            px.append(QPointF(m_xAxis->coordToPixel(dp.key),
                              m_yAxis->coordToPixel(dp.value)));
        }
    }
    return px;
}

// ------------------------------------------------------------------------
// Render LineSeries — 含 5 项绘制策略优化
//   1. 可见范围裁剪 (view culling)
//   2. 二分查找定位数据范围 (binary search for sorted data)
//   3. drawPolyline 批量绘制 (替代逐段 drawLine)
//   4. 自适应降采样 (adaptive simplification)
//   5. 散点密度阈值 (scatter density guard)
// ------------------------------------------------------------------------
void ChartWidget::renderLineSeries(QPainter &p, const QRectF &plotArea, LineSeries *series) {
    if (!series || series->dataCount() < 1) return;

    p.save();
    p.setClipRect(plotArea);

    const auto &data = series->allData();
    int dataSize = data.size();

    // Generate pixel coordinates from visible data range
    QVector<QPointF> pixels = generateLineSeriesPixels(plotArea, data, dataSize);

    if (pixels.size() < 1) { p.restore(); return; }

    // 3. Adaptive simplification for dense data
    int pw = qMax(1, int(plotArea.width()));
    if (pixels.size() > pw * 2)
        pixels = simplifyPolyline(pixels, pw);

    //  4. Fill (under curve to y=0 baseline)
    if (series->fillEnabled() && pixels.size() >= 2) {
        double baseY = m_yAxis->coordToPixel(0);
        QPainterPath fillPath;
        fillPath.moveTo(pixels.first().x(), baseY);
        for (const auto &pt : pixels)
            fillPath.lineTo(pt);
        fillPath.lineTo(pixels.last().x(), baseY);
        fillPath.closeSubpath();

        if (auto *grad = series->fillBrush().gradient()) {
            if (grad->type() == QGradient::LinearGradient) {
                QLinearGradient g = *static_cast<const QLinearGradient*>(grad);
                g.setCoordinateMode(QGradient::ObjectBoundingMode);
                g.setStart(0, 0);
                g.setFinalStop(0, 1);
                p.setBrush(QBrush(g));
            } else {
                p.setBrush(series->fillBrush());
            }
        } else {
            p.setBrush(series->fillBrush());
        }
        p.setPen(Qt::NoPen);
        p.drawPath(fillPath);
    }

    // 5. Line  single batch drawPolyline
    if (pixels.size() >= 2) {
        QPen linePen(series->color(), series->lineWidth());
        p.setPen(linePen);
        p.setBrush(Qt::NoBrush);
        p.drawPolyline(pixels);
    }

    // 6. Scatter markers — skip if too dense
    if (series->scatterStyle() != ScatterStyle::None && pixels.size() <= 500) {
        double r = series->markerSize();
        p.setBrush(series->color());
        p.setPen(QPen(series->color().darker(120), 1));
        for (const auto &pt : pixels) {
            switch (series->scatterStyle()) {
            case ScatterStyle::Circle:
                p.drawEllipse(pt, r, r); break;
            case ScatterStyle::Square:
                p.drawRect(QRectF(pt.x() - r, pt.y() - r, r * 2, r * 2)); break;
            case ScatterStyle::Diamond: {
                QPolygonF d;
                d << QPointF(pt.x(), pt.y() - r) << QPointF(pt.x() + r, pt.y())
                  << QPointF(pt.x(), pt.y() + r) << QPointF(pt.x() - r, pt.y());
                p.drawPolygon(d); break;
            }
            case ScatterStyle::Triangle: {
                QPolygonF t;
                t << QPointF(pt.x(), pt.y() - r)
                  << QPointF(pt.x() + r * 0.866, pt.y() + r * 0.5)
                  << QPointF(pt.x() - r * 0.866, pt.y() + r * 0.5);
                p.drawPolygon(t); break;
            }
            default: break;
            }
        }
    }

    //  7. Hover highlight
    if (m_hoveredSeriesIdx >= 0 && m_hoveredPointIdx >= 0) {
        if (qobject_cast<LineSeries*>(m_model->seriesList().value(m_hoveredSeriesIdx)) == series) {
            if (m_hoveredPointIdx >= 0 && m_hoveredPointIdx < dataSize) {
                double hx = m_xAxis->coordToPixel(data[m_hoveredPointIdx].key);
                double hy = m_yAxis->coordToPixel(data[m_hoveredPointIdx].value);
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(series->color());
                p.drawEllipse(QPointF(hx, hy), 6, 6);
            }
        }
    }

    p.restore();
}

// ------------------------------------------------------------------------
// Render BarSeries (grouped by X key using axis transform)
// ------------------------------------------------------------------------
void ChartWidget::renderBarSeries(QPainter &p, const QRectF &plotArea) {
    auto bars = m_model->seriesByType<BarSeries>();
    if (bars.isEmpty()) return;

    // Collect unique sorted keys for bar spacing calculation
    QVector<double> keys;
    for (auto *bs : bars) {
        if (!bs->isVisible()) continue;
        for (int i = 0; i < bs->dataCount(); ++i)
            keys.append(bs->dataAt(i).key);
    }
    if (keys.isEmpty()) return;
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    int nBars = 0;
    for (auto *bs : bars) if (bs->isVisible()) ++nBars;
    if (nBars == 0) return;

    p.save();
    p.setClipRect(plotArea);

    // Calculate bar width from minimum key spacing
    double minKeySpan = keys.size() >= 2 ? (keys[1] - keys[0]) : 1.0;
    for (int i = 2; i < keys.size(); ++i)
        minKeySpan = qMin(minKeySpan, keys[i] - keys[i-1]);
    double totalBarW = qAbs(m_xAxis->coordToPixel(keys.first() + minKeySpan)
                              - m_xAxis->coordToPixel(keys.first())) * 0.7;
    double barW = totalBarW / nBars;
    double baseY = m_yAxis->coordToPixel(0);

    int bi = 0;
    for (auto *bs : bars) {
        if (!bs->isVisible()) { ++bi; continue; }
        p.setBrush(bs->color());
        p.setPen(QPen(bs->color().darker(130), 1));

        for (int i = 0; i < bs->dataCount(); ++i) {
            auto dp = bs->dataAt(i);
            double xCenter = m_xAxis->coordToPixel(dp.key);
            double x0 = xCenter - totalBarW / 2 + bi * barW;
            double y1 = m_yAxis->coordToPixel(dp.value);
            double y0 = qMax(baseY, plotArea.top());
            QRectF barRect(x0 + 1, qMin(y0, y1), barW - 2, qAbs(y1 - y0));
            p.drawRect(barRect);
        }
        ++bi;
    }
    p.restore();
}

// ------------------------------------------------------------------------
// Render StackedBarSeries (stacked by X key using axis transform)
// ------------------------------------------------------------------------
void ChartWidget::renderStackedBarSeries(QPainter &p, const QRectF &plotArea) {
    auto stacked = m_model->seriesByType<StackedBarSeries>();
    if (stacked.isEmpty()) return;

    // Collect unique sorted keys
    QVector<double> keys;
    for (auto *ss : stacked) {
        if (!ss->isVisible()) continue;
        for (int i = 0; i < ss->dataCount(); ++i)
            keys.append(ss->dataAt(i).key);
    }
    if (keys.isEmpty()) return;
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    p.save();
    p.setClipRect(plotArea);

    double minKeySpan = keys.size() >= 2 ? (keys[1] - keys[0]) : 1.0;
    for (int i = 2; i < keys.size(); ++i)
        minKeySpan = qMin(minKeySpan, keys[i] - keys[i-1]);
    double barW = qAbs(m_xAxis->coordToPixel(keys.first() + minKeySpan)
                        - m_xAxis->coordToPixel(keys.first())) * 0.7;
    double baseY = m_yAxis->coordToPixel(0);

    for (double key : keys) {
        double xCenter = m_xAxis->coordToPixel(key);
        double x0 = xCenter - barW / 2;
        double cumulativePos = 0;
        double cumulativeNeg = 0;

        for (auto *ss : stacked) {
            if (!ss->isVisible()) continue;
            // Find data point matching this key
            double val = 0;
            bool found = false;
            for (int i = 0; i < ss->dataCount(); ++i) {
                if (ss->dataAt(i).key == key) {
                    val = ss->dataAt(i).value;
                    found = true;
                    break;
                }
            }
            if (!found) continue;

            double yTop, yBottom;
            if (val >= 0) {
                yTop = m_yAxis->coordToPixel(cumulativePos + val);
                yBottom = m_yAxis->coordToPixel(cumulativePos);
                cumulativePos += val;
            } else {
                yTop = m_yAxis->coordToPixel(cumulativeNeg);
                yBottom = m_yAxis->coordToPixel(cumulativeNeg + val);
                cumulativeNeg += val;
            }

            QRectF barRect(x0 + 1, qMin(yTop, yBottom), barW - 2, qAbs(yBottom - yTop));
            p.setBrush(ss->color());
            p.setPen(QPen(ss->color().darker(130), 1));
            p.drawRect(barRect);
        }
    }
    p.restore();
}

// ------------------------------------------------------------------------
// Render RangeBarSeries (min-max range bars positioned by key)
// ------------------------------------------------------------------------
void ChartWidget::renderRangeBarSeries(QPainter &p, const QRectF &plotArea) {
    auto ranges = m_model->seriesByType<RangeBarSeries>();
    if (ranges.isEmpty()) return;

    // Collect unique sorted keys
    QVector<double> keys;
    for (auto *rs : ranges) {
        if (!rs->isVisible()) continue;
        for (int i = 0; i < rs->dataCount(); ++i)
            keys.append(rs->dataAt(i).key);
    }
    if (keys.isEmpty()) return;
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    int nRanges = 0;
    for (auto *rs : ranges) if (rs->isVisible()) ++nRanges;
    if (nRanges == 0) return;

    p.save();
    p.setClipRect(plotArea);

    double minKeySpan = keys.size() >= 2 ? (keys[1] - keys[0]) : 1.0;
    for (int i = 2; i < keys.size(); ++i)
        minKeySpan = qMin(minKeySpan, keys[i] - keys[i-1]);
    double totalBarW = qAbs(m_xAxis->coordToPixel(keys.first() + minKeySpan)
                              - m_xAxis->coordToPixel(keys.first())) * 0.7;
    double barW = totalBarW / nRanges;

    int ri = 0;
    for (auto *rs : ranges) {
        if (!rs->isVisible()) { ++ri; continue; }
        QColor c = rs->color();
        p.setBrush(c);
        p.setPen(QPen(c.darker(140), 1));

        for (int i = 0; i < rs->dataCount(); ++i) {
            auto dp = rs->dataAt(i);
            double xCenter = m_xAxis->coordToPixel(dp.key);
            double x0 = xCenter - totalBarW / 2 + ri * barW;
            double yTop = m_yAxis->coordToPixel(dp.maxValue);
            double yBottom = m_yAxis->coordToPixel(dp.minValue);
            QRectF barRect(x0 + 1, qMin(yTop, yBottom), barW - 2, qAbs(yBottom - yTop));
            p.drawRect(barRect);
        }
        ++ri;
    }
    // Hover highlight
    if (m_hoveredSeriesIdx >= 0 && m_hoveredPointIdx >= 0) {
        auto *hitSeries = qobject_cast<RangeBarSeries*>(
            m_model->seriesList().value(m_hoveredSeriesIdx));
        if (hitSeries && hitSeries->isVisible()
            && m_hoveredPointIdx < hitSeries->dataCount()) {
            // Find the bar position & draw highlight border
            int hitRi = 0;
            for (auto *rs : ranges) {
                if (!rs->isVisible()) continue;
                if (rs == hitSeries) break;
                ++hitRi;
            }
            auto dp = hitSeries->dataAt(m_hoveredPointIdx);
            double xCenter = m_xAxis->coordToPixel(dp.key);
            double x0 = xCenter - totalBarW / 2 + hitRi * barW;
            double yTop = m_yAxis->coordToPixel(dp.maxValue);
            double yBottom = m_yAxis->coordToPixel(dp.minValue);
            QRectF barRect(x0 + 1, qMin(yTop, yBottom), barW - 2, qAbs(yBottom - yTop));
            p.setPen(QPen(Qt::white, 3));
            p.setBrush(Qt::NoBrush);
            p.drawRect(barRect);
            p.setPen(QPen(hitSeries->color().lighter(150), 2));
            p.drawRect(barRect.adjusted(1, 1, -1, -1));
        }
    }
    p.restore();
}

// ------------------------------------------------------------------------
// Mouse Events
// ------------------------------------------------------------------------
void ChartWidget::mouseMoveEvent(QMouseEvent *event) {
    QPointF pos = event->pos();

    if (m_dragging) {
        // Drag the plot
        QPointF delta = pos - m_lastMousePos;
        auto info = m_layout->calculate(
            size(), m_model->title(), m_titleFont,
            m_xAxis, m_yAxis, m_legend, m_model->seriesList());
        QRectF plotArea = info.chartArea;
        if (plotArea.isEmpty()) return;

        double dxRange = (m_xAxis->max() - m_xAxis->min()) * (-delta.x() / plotArea.width());
        double dyRange = (m_yAxis->max() - m_yAxis->min()) * (delta.y() / plotArea.height());

        double xMin = m_dragStartXMin + dxRange;
        double xMax = m_dragStartXMax + dxRange;
        double yMin = m_dragStartYMin + dyRange;
        double yMax = m_dragStartYMax + dyRange;

        m_xAxis->setRange(xMin, xMax);
        m_yAxis->setRange(yMin, yMax);
        emit rangeChanged(xMin, xMax, yMin, yMax);
        m_dragStartXMin = xMin;
        m_dragStartXMax = xMax;
        m_dragStartYMin = yMin;
        m_dragStartYMax = yMax;
        m_dirty = true;
        update();
    } else {
        // Hover detection
        Series *hitSeries = nullptr;
        int hitIdx = hitTestDataPoint(pos, &hitSeries);
        if (hitIdx >= 0 && hitSeries) {
            int si = m_model->seriesList().indexOf(hitSeries);
            if (si != m_hoveredSeriesIdx || hitIdx != m_hoveredPointIdx) {
                m_hoveredSeriesIdx = si;
                m_hoveredPointIdx = hitIdx;
                m_dirty = true;
                update();

                // Emit hover signal
                if (auto *ls = qobject_cast<LineSeries*>(hitSeries)) {
                    if (hitIdx < ls->dataCount()) {
                        const auto &dp = ls->dataAt(hitIdx);
                        emit dataPointHovered(hitSeries, hitIdx, QPointF(dp.key, dp.value));
                    }
                } else if (auto *rs = qobject_cast<RangeBarSeries*>(hitSeries)) {
                    if (hitIdx < rs->dataCount()) {
                        const auto &dp = rs->dataAt(hitIdx);
                        emit dataPointHovered(hitSeries, hitIdx, QPointF(dp.key, dp.maxValue));
                    }
                }
            }
        } else {
            if (m_hoveredSeriesIdx >= 0 || m_hoveredPointIdx >= 0) {
                m_hoveredSeriesIdx = -1;
                m_hoveredPointIdx = -1;
                m_dirty = true;
                update();
            }
        }
    }
    m_lastMousePos = pos;
}

void ChartWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = event->pos();
        m_mousePressPos = event->pos();
        m_dragStartXMin = m_xAxis->min();
        m_dragStartXMax = m_xAxis->max();
        m_dragStartYMin = m_yAxis->min();
        m_dragStartYMax = m_yAxis->max();
    }
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;

        // Check if it was a click (not drag)
        if ((event->pos() - m_mousePressPos).manhattanLength() < 5) {
            Series *hitSeries = nullptr;
            int hitIdx = hitTestDataPoint(event->pos(), &hitSeries);
            if (hitIdx >= 0 && hitSeries) {
                if (auto *ls = qobject_cast<LineSeries*>(hitSeries)) {
                    if (hitIdx < ls->dataCount()) {
                        const auto &dp = ls->dataAt(hitIdx);
                        emit dataPointClicked(hitSeries, hitIdx, QPointF(dp.key, dp.value));
                    }
                } else if (auto *rs = qobject_cast<RangeBarSeries*>(hitSeries)) {
                    if (hitIdx < rs->dataCount()) {
                        const auto &dp = rs->dataAt(hitIdx);
                        emit dataPointClicked(hitSeries, hitIdx, QPointF(dp.key, dp.maxValue));
                    }
                }
            }
        }
    }
}

void ChartWidget::wheelEvent(QWheelEvent *event) {
    double factor = (event->angleDelta().y() > 0) ? 0.85 : 1.18;

    auto info = m_layout->calculate(
        size(), m_model->title(), m_titleFont,
        m_xAxis, m_yAxis, m_legend, m_model->seriesList());
    QRectF plotArea = info.chartArea;
    if (plotArea.isEmpty()) return;

    // Zoom around cursor position
    double mouseX = m_xAxis->pixelToCoord(event->position().x());
    double mouseY = m_yAxis->pixelToCoord(event->position().y());

    double xMin = mouseX + (m_xAxis->min() - mouseX) * factor;
    double xMax = mouseX + (m_xAxis->max() - mouseX) * factor;
    double yMin = mouseY + (m_yAxis->min() - mouseY) * factor;
    double yMax = mouseY + (m_yAxis->max() - mouseY) * factor;

    m_xAxis->setRange(xMin, xMax);
    m_yAxis->setRange(yMin, yMax);
    emit rangeChanged(xMin, xMax, yMin, yMax);
    m_dirty = true;
    update();
}

// ------------------------------------------------------------------------
// Hit testing — find nearest data point (with binary search for sorted data)
// ------------------------------------------------------------------------
int ChartWidget::hitTestDataPoint(const QPointF &widgetPos, Series **outSeries) const {
    *outSeries = nullptr;

    auto info = m_layout->calculate(
        size(), m_model->title(), m_titleFont,
        m_xAxis, m_yAxis, m_legend, m_model->seriesList());
    QRectF plotArea = info.chartArea;
    if (plotArea.isEmpty()) return -1;

    double threshold = 15.0; // pixel threshold

    for (auto *s : m_model->seriesList()) {
        if (!s->isVisible()) continue;

        // LineSeries: nearest-point distance
        if (auto *ls = qobject_cast<LineSeries*>(s)) {
            const auto &data = ls->allData();
            int dataSize = data.size();
            if (dataSize == 0) continue;

            int beginIdx = 0, endIdx = dataSize;
            if (dataSize > 100) {
                double mouseKey = m_xAxis->pixelToCoord(widgetPos.x());
                double keyRange = (m_xAxis->max() - m_xAxis->min()) * (threshold / plotArea.width());
                bool sorted = (data.last().key >= data.first().key);
                if (sorted) {
                    auto lessDP = [](const LineSeries::DataPoint &dp, double key) { return dp.key < key; };
                    auto lessKey = [](double key, const LineSeries::DataPoint &dp) { return key < dp.key; };
                    beginIdx = int(std::lower_bound(data.begin(), data.end(), mouseKey - keyRange, lessDP) - data.begin());
                    endIdx   = int(std::upper_bound(data.begin(), data.end(), mouseKey + keyRange, lessKey) - data.begin());
                }
            }

            double bestDist = threshold;
            int bestIdx = -1;
            for (int i = beginIdx; i < endIdx; ++i) {
                double px = m_xAxis->coordToPixel(data[i].key);
                double py = m_yAxis->coordToPixel(data[i].value);
                double dist = QLineF(widgetPos, QPointF(px, py)).length();
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = i;
                }
            }
            if (bestIdx >= 0) {
                *outSeries = s;
                return bestIdx;
            }
            continue;
        }

        // RangeBarSeries: bar-rect hit test
        if (auto *rs = qobject_cast<RangeBarSeries*>(s)) {
            const auto &data = rs->allData();
            if (data.isEmpty()) continue;

            double barHalfW = 10.0;
            if (data.size() >= 2) {
                double pxFirst = m_xAxis->coordToPixel(data.first().key);
                double pxLast  = m_xAxis->coordToPixel(data.last().key);
                double pixelSpan = qAbs(pxLast - pxFirst);
                barHalfW = pixelSpan / data.size() * 0.25;
            }

            for (int i = 0; i < data.size(); ++i) {
                double px = m_xAxis->coordToPixel(data[i].key);
                double pyMin = m_yAxis->coordToPixel(data[i].minValue);
                double pyMax = m_yAxis->coordToPixel(data[i].maxValue);
                QRectF barRect(px - barHalfW, qMin(pyMin, pyMax),
                               barHalfW * 2, qAbs(pyMax - pyMin));
                if (barRect.adjusted(-threshold / 2, -threshold / 2,
                                     threshold / 2, threshold / 2).contains(widgetPos)) {
                    *outSeries = s;
                    return i;
                }
            }
            continue;
        }
    }
    return -1;
}
