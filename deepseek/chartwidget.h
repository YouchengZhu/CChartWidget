#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>
#include <QPair>
#include <QSet>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QMarginsF>
#include <QVariant>
#include <QDateTime>
#include <QDate>
#include <QPainterPath>
#include <QtMath>
#include <QObject>
#include <algorithm>
#include <limits>

// ============================================================================
// Forward declarations
// ============================================================================

class GraphBase;
class BaseAxis;

// ============================================================================
// GridLayoutItem — base class for grid-layout items
// ============================================================================

class GridLayoutItem
{
public:
    GridLayoutItem() = default;
    virtual ~GridLayoutItem() = default;

    void setGridPosition(int row, int col, int rowSpan = 1, int colSpan = 1);
    int row() const { return m_row; }
    int col() const { return m_col; }
    int rowSpan() const { return m_rowSpan; }
    int colSpan() const { return m_colSpan; }

    virtual QSize sizeHint() const = 0;
    virtual void setGeometry(const QRect &rect) = 0;
    virtual QRect geometry() const = 0;
    virtual void render(QPainter *painter) = 0;

private:
    int m_row = 0;
    int m_col = 0;
    int m_rowSpan = 1;
    int m_colSpan = 1;
};

// ============================================================================
// GraphBase — non-template base for all graph types
// ============================================================================

class GraphBase
{
public:
    GraphBase() = default;
    virtual ~GraphBase() = default;

    virtual void draw(QPainter *painter, BaseAxis *xAxis, BaseAxis *yAxis,
                      const QRectF &plotArea) = 0;

    virtual bool nearestPoint(BaseAxis *xAxis, BaseAxis *yAxis,
                              const QRectF &plotArea, const QPointF &pixel,
                              QVariant &key, QVariant &value,
                              double &distance) const = 0;

    void setLayer(int layer) { m_layer = layer; }
    int layer() const { return m_layer; }

    void setName(const QString &name) { m_name = name; }
    QString name() const { return m_name; }

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

private:
    int m_layer = 0;
    QString m_name;
    bool m_visible = true;
};

// ============================================================================
// ChartData — template data types
// ============================================================================

template<typename TKey, typename TValue>
struct ChartDataPoint {
    TKey key;
    TValue value;

    bool operator<(const ChartDataPoint &other) const { return key < other.key; }
};

// ============================================================================
// ScatterFormat
// ============================================================================

enum class ScatterShape {
    None,
    Circle,
    Square,
    Diamond,
    Triangle,
    Cross,
    Plus
};

class ScatterFormat
{
public:
    ScatterShape shape = ScatterShape::None;
    int size = 6;
    QColor color = Qt::black;
    QColor fillColor = Qt::white;
    double borderWidth = 1.0;

    bool isVisible() const { return shape != ScatterShape::None; }
};

// ============================================================================
// Layer
// ============================================================================

enum class LayerType {
    Background,
    Axis,
    Graph
};

class Layer
{
public:
    Layer(const QString &name = QString(), int order = 0);

    int order() const { return m_order; }
    void setOrder(int order) { m_order = order; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    LayerType type() const { return m_type; }
    void setType(LayerType type) { m_type = type; }

    void addGraph(GraphBase *graph);
    void removeGraph(GraphBase *graph);
    const QVector<GraphBase *> &graphs() const { return m_graphs; }
    void clearGraphs();

private:
    QString m_name;
    int m_order = 0;
    LayerType m_type = LayerType::Graph;
    QVector<GraphBase *> m_graphs;
};

// ============================================================================
// BaseAxis — abstract axis
// ============================================================================

class BaseAxis : public QObject
{
    Q_OBJECT

public:
    enum AxisOrientation {
        Horizontal,
        Vertical
    };

    explicit BaseAxis(AxisOrientation orientation, QObject *parent = nullptr);
    ~BaseAxis() override = default;

    AxisOrientation orientation() const { return m_orientation; }

    virtual double valueToPixel(const QVariant &value) const = 0;
    virtual QVariant pixelToValue(double pixel) const = 0;
    virtual void calculateTicks() = 0;
    virtual QString formatTickLabel(const QVariant &value) const = 0;
    virtual void draw(QPainter *painter, const QRectF &axisRect,
                      const QRectF &plotArea) = 0;
    virtual void zoomRange(double factor, const QVariant &center) = 0;
    virtual void panRange(double pixelDelta, double plotAreaSize) = 0;

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    void setZoomEnabled(bool enabled) { m_zoomEnabled = enabled; }
    bool isZoomEnabled() const { return m_zoomEnabled; }

    void setPanEnabled(bool enabled) { m_panEnabled = enabled; }
    bool isPanEnabled() const { return m_panEnabled; }

    void setTickCount(int count) { m_tickCount = qMax(2, count); }
    int tickCount() const { return m_tickCount; }

    void setShowTickLabels(bool show) { m_showTickLabels = show; }
    bool showTickLabels() const { return m_showTickLabels; }

    void setAxisColor(const QColor &color) { m_axisColor = color; }
    QColor axisColor() const { return m_axisColor; }

    void setTickColor(const QColor &color) { m_tickColor = color; }
    QColor tickColor() const { return m_tickColor; }

    void setLabelColor(const QColor &color) { m_labelColor = color; }
    QColor labelColor() const { return m_labelColor; }

    void setLabelFont(const QFont &font) { m_labelFont = font; }
    QFont labelFont() const { return m_labelFont; }

    void setAxisLineWidth(double width) { m_axisLineWidth = width; }
    double axisLineWidth() const { return m_axisLineWidth; }

    void setTickLength(double length) { m_tickLength = length; }
    double tickLength() const { return m_tickLength; }

    QVector<QVariant> tickValues() const { return m_tickValues; }
    QVector<double> tickPixelPositions() const { return m_tickPixelPositions; }

protected:
    AxisOrientation m_orientation;
    bool m_visible = true;
    bool m_zoomEnabled = true;
    bool m_panEnabled = true;
    int m_tickCount = 5;
    bool m_showTickLabels = true;
    QColor m_axisColor = Qt::black;
    QColor m_tickColor = Qt::black;
    QColor m_labelColor = Qt::black;
    QFont m_labelFont;
    double m_axisLineWidth = 1.5;
    double m_tickLength = 5.0;

    QVector<QVariant> m_tickValues;
    QVector<double> m_tickPixelPositions;

    double niceNumber(double x, bool roundUp) const;
};

// ============================================================================
// NumericAxis
// ============================================================================

class NumericAxis : public BaseAxis
{
    Q_OBJECT

public:
    explicit NumericAxis(AxisOrientation orientation = Horizontal, QObject *parent = nullptr);

    void setRange(double min, double max);
    double rangeMin() const { return m_min; }
    double rangeMax() const { return m_max; }

    void setDecimalPlaces(int places) { m_decimalPlaces = places; }
    int decimalPlaces() const { return m_decimalPlaces; }

    void setLabelFormat(const QString &format);
    QString labelFormat() const { return m_labelFormat; }

    double valueToPixel(const QVariant &value) const override;
    QVariant pixelToValue(double pixel) const override;
    void calculateTicks() override;
    QString formatTickLabel(const QVariant &value) const override;
    void draw(QPainter *painter, const QRectF &axisRect,
              const QRectF &plotArea) override;
    void zoomRange(double factor, const QVariant &center) override;
    void panRange(double pixelDelta, double plotAreaSize) override;

private:
    double m_min = 0.0;
    double m_max = 100.0;
    int m_decimalPlaces = 0;
    QString m_labelFormat;
    QRectF m_cachePlotArea;
};

// ============================================================================
// DateTimeAxis — minute/second precision
// ============================================================================

class DateTimeAxis : public BaseAxis
{
    Q_OBJECT

public:
    explicit DateTimeAxis(AxisOrientation orientation = Horizontal,
                          QObject *parent = nullptr);

    void setRange(const QDateTime &min, const QDateTime &max);
    QDateTime rangeMin() const { return m_min; }
    QDateTime rangeMax() const { return m_max; }

    void setLabelFormat(const QString &format);
    QString labelFormat() const { return m_labelFormat; }

    double valueToPixel(const QVariant &value) const override;
    QVariant pixelToValue(double pixel) const override;
    void calculateTicks() override;
    QString formatTickLabel(const QVariant &value) const override;
    void draw(QPainter *painter, const QRectF &axisRect,
              const QRectF &plotArea) override;
    void zoomRange(double factor, const QVariant &center) override;
    void panRange(double pixelDelta, double plotAreaSize) override;
    void resetPanRemainder() { m_panRemainder = 0.0; }

private:
    QDateTime m_min;
    QDateTime m_max;
    QString m_labelFormat = "HH:mm";
    QRectF m_cachePlotArea;
    double m_panRemainder = 0.0;
    qint64 niceInterval(qint64 rangeSecs) const;
};

// ============================================================================
// DateAxis — day precision
// ============================================================================

class DateAxis : public BaseAxis
{
    Q_OBJECT

public:
    explicit DateAxis(AxisOrientation orientation = Horizontal,
                      QObject *parent = nullptr);

    void setRange(const QDate &min, const QDate &max);
    QDate rangeMin() const { return m_min; }
    QDate rangeMax() const { return m_max; }

    void setLabelFormat(const QString &format);
    QString labelFormat() const { return m_labelFormat; }

    double valueToPixel(const QVariant &value) const override;
    QVariant pixelToValue(double pixel) const override;
    void calculateTicks() override;
    QString formatTickLabel(const QVariant &value) const override;
    void draw(QPainter *painter, const QRectF &axisRect,
              const QRectF &plotArea) override;
    void zoomRange(double factor, const QVariant &center) override;
    void panRange(double pixelDelta, double plotAreaSize) override;
    void resetPanRemainder() { m_panRemainder = 0.0; }

private:
    QDate m_min;
    QDate m_max;
    QString m_labelFormat = "yyyy-MM-dd";
    QRectF m_cachePlotArea;
    double m_panRemainder = 0.0;
    qint64 niceInterval(qint64 rangeDays) const;
};

// ============================================================================
// AbstractGraph — template base for all graphs
// ============================================================================

template<typename TKey, typename TValue>
class AbstractGraph : public GraphBase
{
public:
    AbstractGraph() = default;
    ~AbstractGraph() override = default;

    using DataPoint = ChartDataPoint<TKey, TValue>;
    using DataVector = QVector<DataPoint>;

    void setData(const DataVector &data);
    DataVector data() const { return m_data; }

    void addPoint(const TKey &key, const TValue &value);
    void removePoint(const TKey &key);
    void clearData() { m_data.clear(); }
    int dataCount() const { return m_data.size(); }

    TKey keyMin() const { return m_data.isEmpty() ? TKey() : m_data.first().key; }
    TKey keyMax() const { return m_data.isEmpty() ? TKey() : m_data.last().key; }

    TValue valueMin() const
    {
        if (m_data.isEmpty()) return TValue();
        TValue minVal = m_data.first().value;
        for (const auto &pt : m_data) {
            if (pt.value < minVal) minVal = pt.value;
        }
        return minVal;
    }

    TValue valueMax() const
    {
        if (m_data.isEmpty()) return TValue();
        TValue maxVal = m_data.first().value;
        for (const auto &pt : m_data) {
            if (maxVal < pt.value) maxVal = pt.value;
        }
        return maxVal;
    }

    void setColor(const QColor &color) { m_color = color; }
    QColor color() const { return m_color; }

    bool nearestPoint(BaseAxis *xAxis, BaseAxis *yAxis,
                      const QRectF &plotArea, const QPointF &pixel,
                      QVariant &key, QVariant &value,
                      double &distance) const override;

protected:
    DataVector m_data;
    QColor m_color = Qt::blue;

    struct VisibleRange { int start = 0; int end = 0; };
    VisibleRange computeVisibleRange(BaseAxis *xAxis, const QRectF &plotArea,
                                     int extend = 1) const;
};

template<typename TKey, typename TValue>
void AbstractGraph<TKey, TValue>::setData(const DataVector &data)
{
    m_data = data;
    std::sort(m_data.begin(), m_data.end());
}

template<typename TKey, typename TValue>
void AbstractGraph<TKey, TValue>::addPoint(const TKey &key, const TValue &value)
{
    DataPoint pt{key, value};
    auto it = std::lower_bound(m_data.begin(), m_data.end(), pt);
    if (it != m_data.end() && it->key == key)
        it->value = value;
    else
        m_data.insert(it, pt);
}

template<typename TKey, typename TValue>
void AbstractGraph<TKey, TValue>::removePoint(const TKey &key)
{
    auto it = std::find_if(m_data.begin(), m_data.end(),
                           [&key](const DataPoint &pt) { return pt.key == key; });
    if (it != m_data.end())
        m_data.erase(it);
}

template<typename TKey, typename TValue>
bool AbstractGraph<TKey, TValue>::nearestPoint(
        BaseAxis *xAxis, BaseAxis *yAxis,
        const QRectF & /*plotArea*/, const QPointF &pixel,
        QVariant &key, QVariant &value, double &distance) const
{
    if (m_data.isEmpty() || !xAxis || !yAxis)
        return false;

    TKey searchKey = qvariant_cast<TKey>(xAxis->pixelToValue(pixel.x()));

    auto cmp = [](const DataPoint &pt, const TKey &k) { return pt.key < k; };
    auto it = std::lower_bound(m_data.cbegin(), m_data.cend(), searchKey, cmp);
    int idx = static_cast<int>(it - m_data.cbegin());

    double bestDist = std::numeric_limits<double>::max();
    int bestIdx = -1;

    int lo = qMax(0, idx - 1);
    int hi = qMin(static_cast<int>(m_data.size()) - 1, idx);

    for (int i = lo; i <= hi; ++i) {
        double px = xAxis->valueToPixel(QVariant::fromValue(m_data[i].key));
        double py = yAxis->valueToPixel(QVariant::fromValue(m_data[i].value));
        double dx = px - pixel.x();
        double dy = py - pixel.y();
        double dist = qSqrt(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        key = QVariant::fromValue(m_data[bestIdx].key);
        value = QVariant::fromValue(m_data[bestIdx].value);
        distance = bestDist;
        return true;
    }

    return false;
}

// ============================================================================
// LineGraph
// ============================================================================

template<typename TKey, typename TValue>
class LineGraph : public AbstractGraph<TKey, TValue>
{
public:
    LineGraph();

    void setLineColor(const QColor &color);
    QColor lineColor() const { return m_lineColor; }

    void setLineWidth(double width) { m_lineWidth = width; }
    double lineWidth() const { return m_lineWidth; }

    void setLineStyle(Qt::PenStyle style) { m_lineStyle = style; }
    Qt::PenStyle lineStyle() const { return m_lineStyle; }

    void setScatterFormat(const ScatterFormat &format) { m_scatterFormat = format; }
    ScatterFormat scatterFormat() const { return m_scatterFormat; }

    void setSmooth(bool smooth) { m_smooth = smooth; }
    bool smooth() const { return m_smooth; }

    void setFillBelow(bool fill) { m_fillBelow = fill; }
    bool fillBelow() const { return m_fillBelow; }

    void setFillColor(const QColor &color) { m_fillColor = color; }
    QColor fillColor() const { return m_fillColor; }

    void setAdaptiveSampling(bool enabled) { m_adaptiveSampling = enabled; }
    bool adaptiveSampling() const { return m_adaptiveSampling; }

    void draw(QPainter *painter, BaseAxis *xAxis, BaseAxis *yAxis,
              const QRectF &plotArea) override;

private:
    void drawScatter(QPainter *painter, const QPointF &center);
    QPolygonF buildDiamond(const QPointF &center, int halfSize);
    QPolygonF buildTriangle(const QPointF &center, int halfSize);
    QVector<QPointF> sampleAdaptive(const QVector<QPointF> &points,
                                     double plotWidth) const;

    QColor m_lineColor = Qt::blue;
    double m_lineWidth = 2.0;
    Qt::PenStyle m_lineStyle = Qt::SolidLine;
    ScatterFormat m_scatterFormat;
    bool m_smooth = false;
    bool m_fillBelow = false;
    QColor m_fillColor = QColor(0, 0, 255, 30);
    bool m_adaptiveSampling = true;
};

// ============================================================================
// BarGraph
// ============================================================================

template<typename TKey, typename TValue>
class BarGraph : public AbstractGraph<TKey, TValue>
{
public:
    BarGraph();

    void setBarColor(const QColor &color);
    QColor barColor() const { return m_barColor; }

    void setBarWidthFactor(double factor);
    double barWidthFactor() const { return m_barWidthFactor; }

    void setBorderColor(const QColor &color) { m_borderColor = color; }
    QColor borderColor() const { return m_borderColor; }

    void setBorderWidth(double width) { m_borderWidth = width; }
    double borderWidth() const { return m_borderWidth; }

    void draw(QPainter *painter, BaseAxis *xAxis, BaseAxis *yAxis,
              const QRectF &plotArea) override;

private:
    QColor m_barColor = QColor(65, 105, 225);
    QColor m_borderColor = Qt::black;
    double m_borderWidth = 1.0;
    double m_barWidthFactor = 0.8;
};

// ============================================================================
// StackedBarGraph
// ============================================================================

template<typename TKey, typename TValue>
class StackedBarGraph : public AbstractGraph<TKey, TValue>
{
public:
    using DataPoint = ChartDataPoint<TKey, TValue>;
    using DataVector = QVector<DataPoint>;

    StackedBarGraph();

    void setSeriesCount(int count);
    int seriesCount() const { return m_seriesData.size(); }

    void setSeriesData(int seriesIndex, const DataVector &data);
    DataVector seriesData(int seriesIndex) const;

    void setSeriesColor(int seriesIndex, const QColor &color);
    QColor seriesColor(int seriesIndex) const;

    void setSeriesName(int seriesIndex, const QString &name);
    QString seriesName(int seriesIndex) const;

    void setBarWidthFactor(double factor);
    double barWidthFactor() const { return m_barWidthFactor; }

    void setBorderColor(const QColor &color) { m_borderColor = color; }
    QColor borderColor() const { return m_borderColor; }

    void setBorderWidth(double width) { m_borderWidth = width; }
    double borderWidth() const { return m_borderWidth; }

    void draw(QPainter *painter, BaseAxis *xAxis, BaseAxis *yAxis,
              const QRectF &plotArea) override;

private:
    QVector<DataVector> m_seriesData;
    QVector<QColor> m_seriesColors;
    QVector<QString> m_seriesNames;
    QColor m_borderColor = Qt::black;
    double m_borderWidth = 1.0;
    double m_barWidthFactor = 0.8;

    QVector<TKey> collectAllKeys() const;
    QColor defaultColor(int index) const;

    static TValue seriesValueAt(const DataVector &series, const TKey &key);
};

// ============================================================================
// AxisRect — a chart area containing axes and graphs
// ============================================================================

class AxisRect : public GridLayoutItem
{
public:
    AxisRect();
    ~AxisRect() override;

    void setXAxis(BaseAxis *axis);
    void setYAxis(BaseAxis *axis);
    BaseAxis *xAxis() const { return m_xAxis; }
    BaseAxis *yAxis() const { return m_yAxis; }

    void addGraph(GraphBase *graph);
    void removeGraph(GraphBase *graph);
    QVector<GraphBase *> graphs() const { return m_graphs; }
    void clearGraphs();

    void setBackgroundColor(const QColor &color) { m_backgroundColor = color; }
    QColor backgroundColor() const { return m_backgroundColor; }

    void setGridLineColor(const QColor &color) { m_gridLineColor = color; }
    QColor gridLineColor() const { return m_gridLineColor; }

    void setShowGrid(bool show) { m_showGrid = show; }
    bool showGrid() const { return m_showGrid; }

    void setShowGridX(bool show) { m_showGridX = show; }
    bool showGridX() const { return m_showGridX; }

    void setShowGridY(bool show) { m_showGridY = show; }
    bool showGridY() const { return m_showGridY; }

    void setMargins(const QMarginsF &margins) { m_margins = margins; }
    QMarginsF margins() const { return m_margins; }

    QRectF plotArea() const;

    QSize sizeHint() const override;
    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
    void render(QPainter *painter) override;

private:
    void drawBackground(QPainter *painter);
    void drawGridLines(QPainter *painter);

    BaseAxis *m_xAxis = nullptr;
    BaseAxis *m_yAxis = nullptr;
    QVector<GraphBase *> m_graphs;

    QRect m_geometry;
    QMarginsF m_margins{60.0, 20.0, 20.0, 40.0};
    QColor m_backgroundColor = Qt::white;
    QColor m_gridLineColor = QColor(220, 220, 220);
    bool m_showGrid = true;
    bool m_showGridX = true;
    bool m_showGridY = true;
};

// ============================================================================
// ChartTable — grid layout container
// ============================================================================

class ChartTable : public GridLayoutItem
{
public:
    explicit ChartTable(int rows = 1, int cols = 1);
    ~ChartTable() override;

    void setGridSize(int rows, int cols);
    int rowCount() const { return m_rows; }
    int colCount() const { return m_cols; }

    void addItem(GridLayoutItem *item, int row, int col,
                 int rowSpan = 1, int colSpan = 1);
    void removeItem(GridLayoutItem *item);
    GridLayoutItem *itemAt(int row, int col) const;
    GridLayoutItem *itemAtPos(const QPoint &pos) const;
    QVector<GridLayoutItem *> items() const { return m_items; }
    void clearItems();

    QSize sizeHint() const override;
    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
    void render(QPainter *painter) override;

private:
    void recalculateLayout();

    int m_rows = 1;
    int m_cols = 1;
    QVector<GridLayoutItem *> m_items;
    QRect m_geometry;
};

// ============================================================================
// ChartToolTipInfo — emitted on mouse move near data points
// ============================================================================

struct ChartToolTipInfo {
    AxisRect *axisRect = nullptr;
    GraphBase *graph = nullptr;
    QPointF pixelPos;
    QVariant dataKey;
    QVariant dataValue;
    double distancePx = -1.0;
    bool isValid() const { return graph != nullptr; }
};

// ============================================================================
// ChartWidget — main widget with double buffering
// ============================================================================

enum ChartInteraction {
    NoInteraction = 0x0,
    ZoomWheel     = 0x1,
    DragPan       = 0x2
};
Q_DECLARE_FLAGS(ChartInteractions, ChartInteraction)
Q_DECLARE_OPERATORS_FOR_FLAGS(ChartInteractions)

enum ZoomAxes {
    ZoomNone = 0x0,
    ZoomX    = 0x1,
    ZoomY    = 0x2,
    ZoomBoth = ZoomX | ZoomY
};

enum PanAxes {
    PanNone = 0x0,
    PanX    = 0x1,
    PanY    = 0x2,
    PanBoth = PanX | PanY
};

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget() override;

    ChartTable *chartTable() const { return m_chartTable; }
    void setChartTable(ChartTable *table);

    void invalidateBuffer();

    void setInteractions(ChartInteractions interactions) { m_interactions = interactions; }
    ChartInteractions interactions() const { return m_interactions; }

    void setZoomFactor(double factor) { m_zoomFactor = qBound(1.05, factor, 3.0); }
    double zoomFactor() const { return m_zoomFactor; }

    void setZoomAxes(ZoomAxes axes) { m_zoomAxes = axes; }
    ZoomAxes zoomAxes() const { return m_zoomAxes; }

    void setPanAxes(PanAxes axes) { m_panAxes = axes; }
    PanAxes panAxes() const { return m_panAxes; }

signals:
    void toolTipRequested(const ChartToolTipInfo &info);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateBuffer();

    ChartTable *m_chartTable = nullptr;
    bool m_ownsTable = false;
    QPixmap m_buffer;
    bool m_bufferDirty = true;
    ChartInteractions m_interactions = ZoomWheel | DragPan;
    double m_zoomFactor = 1.15;
    ZoomAxes m_zoomAxes = ZoomBoth;
    PanAxes m_panAxes = PanBoth;

    bool m_panning = false;
    QPoint m_panLastPos;
    AxisRect *m_panAxisRect = nullptr;
};

// ============================================================================
// Template implementations
// ============================================================================

template<typename TKey, typename TValue>
typename AbstractGraph<TKey, TValue>::VisibleRange
AbstractGraph<TKey, TValue>::computeVisibleRange(BaseAxis *xAxis,
                                                  const QRectF &plotArea,
                                                  int extend) const
{
    if (m_data.isEmpty() || !xAxis)
        return {0, 0};

    TKey minKey = qvariant_cast<TKey>(xAxis->pixelToValue(plotArea.left()));
    TKey maxKey = qvariant_cast<TKey>(xAxis->pixelToValue(plotArea.right()));

    auto cmpLo = [](const DataPoint &pt, const TKey &k) { return pt.key < k; };
    auto cmpHi = [](const TKey &k, const DataPoint &pt) { return k < pt.key; };

    int first = std::lower_bound(m_data.cbegin(), m_data.cend(), minKey, cmpLo)
                - m_data.cbegin();
    int last  = std::upper_bound(m_data.cbegin(), m_data.cend(), maxKey, cmpHi)
                - m_data.cbegin();

    first = qMax(0, first - extend);
    last  = qMin(static_cast<int>(m_data.size()), last + extend);

    return {first, qMax(first, last)};
}

// ---- LineGraph ----

template<typename TKey, typename TValue>
LineGraph<TKey, TValue>::LineGraph()
{
    m_scatterFormat.shape = ScatterShape::None;
}

template<typename TKey, typename TValue>
void LineGraph<TKey, TValue>::setLineColor(const QColor &color)
{
    m_lineColor = color;
    this->m_color = color;
}

template<typename TKey, typename TValue>
QVector<QPointF> LineGraph<TKey, TValue>::sampleAdaptive(
        const QVector<QPointF> &points, double plotWidth) const
{
    if (points.size() < 3)
        return points;

    int pixelW = qMax(1, static_cast<int>(plotWidth));
    if (points.size() <= pixelW * 2)
        return points;

    QVector<QPointF> sampled;
    sampled.reserve(pixelW * 4);

    int i = 0;
    while (i < points.size()) {
        int col = static_cast<int>(points[i].x());
        int colStart = i;
        int colMin   = i;
        int colMax   = i;
        ++i;

        while (i < points.size() && static_cast<int>(points[i].x()) == col) {
            if (points[i].y() < points[colMin].y()) colMin = i;
            if (points[i].y() > points[colMax].y()) colMax = i;
            ++i;
        }
        int colEnd = i - 1;

        QVector<int> indices;
        indices.reserve(4);
        indices.append(colStart);
        if (colMin != colStart && colMin != colEnd) indices.append(colMin);
        if (colMax != colStart && colMax != colEnd && colMax != colMin)
            indices.append(colMax);
        if (colEnd != colStart) indices.append(colEnd);

        std::sort(indices.begin(), indices.end());
        for (int idx : indices)
            sampled.append(points[idx]);
    }

    return sampled;
}

template<typename TKey, typename TValue>
void LineGraph<TKey, TValue>::draw(QPainter *painter, BaseAxis *xAxis,
                                    BaseAxis *yAxis, const QRectF &plotArea)
{
    if (!this->isVisible() || this->m_data.isEmpty())
        return;

    auto vr = this->computeVisibleRange(xAxis, plotArea, 1);
    if (vr.start >= vr.end)
        return;

    int count = vr.end - vr.start;
    QVector<QPointF> raw;
    raw.reserve(count);

    for (int i = vr.start; i < vr.end; ++i) {
        const auto &pt = this->m_data[i];
        double px = xAxis->valueToPixel(QVariant::fromValue(pt.key));
        double py = yAxis->valueToPixel(QVariant::fromValue(pt.value));
        raw.append(QPointF(px, py));
    }

    const QVector<QPointF> &points = m_adaptiveSampling
        ? sampleAdaptive(raw, plotArea.width()) : raw;

    painter->save();
    painter->setClipRect(plotArea);

    if (m_fillBelow && points.size() >= 2) {
        QPainterPath fillPath;
        fillPath.moveTo(points.first());
        for (int i = 1; i < points.size(); ++i)
            fillPath.lineTo(points[i]);
        fillPath.lineTo(QPointF(points.last().x(), plotArea.bottom()));
        fillPath.lineTo(QPointF(points.first().x(), plotArea.bottom()));
        fillPath.closeSubpath();
        painter->fillPath(fillPath, QBrush(m_fillColor));
    }

    QPen linePen(m_lineColor, m_lineWidth, m_lineStyle);
    linePen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(linePen);

    if (m_smooth && points.size() >= 3) {
        QPainterPath path;
        path.moveTo(points.first());
        for (int i = 1; i < points.size(); ++i) {
            QPointF c1, c2;
            if (i == 1) {
                c1 = points[0];
            } else {
                c1 = QPointF((points[i - 1].x() + points[i].x()) / 2.0,
                             points[i - 1].y());
            }
            if (i == points.size() - 1) {
                c2 = points.last();
            } else {
                c2 = QPointF((points[i - 1].x() + points[i].x()) / 2.0,
                             points[i].y());
            }
            path.cubicTo(c1, c2, points[i]);
        }
        painter->drawPath(path);
    } else {
        for (int i = 0; i < points.size() - 1; ++i)
            painter->drawLine(points[i], points[i + 1]);
    }

    if (m_scatterFormat.isVisible()) {
        for (const QPointF &pt : points)
            drawScatter(painter, pt);
    }

    painter->restore();
}

template<typename TKey, typename TValue>
void LineGraph<TKey, TValue>::drawScatter(QPainter *painter, const QPointF &center)
{
    int hs = m_scatterFormat.size / 2;
    QPen borderPen(m_scatterFormat.color, m_scatterFormat.borderWidth);
    QBrush fillBrush(m_scatterFormat.fillColor);
    painter->save();
    painter->setPen(borderPen);
    painter->setBrush(fillBrush);

    switch (m_scatterFormat.shape) {
    case ScatterShape::Circle:
        painter->drawEllipse(center, hs, hs);
        break;
    case ScatterShape::Square: {
        QRectF rect(center.x() - hs, center.y() - hs,
                    m_scatterFormat.size, m_scatterFormat.size);
        painter->drawRect(rect);
        break;
    }
    case ScatterShape::Diamond:
        painter->drawPolygon(buildDiamond(center, hs));
        break;
    case ScatterShape::Triangle:
        painter->drawPolygon(buildTriangle(center, hs));
        break;
    case ScatterShape::Cross:
        painter->drawLine(QPointF(center.x() - hs, center.y() - hs),
                          QPointF(center.x() + hs, center.y() + hs));
        painter->drawLine(QPointF(center.x() - hs, center.y() + hs),
                          QPointF(center.x() + hs, center.y() - hs));
        break;
    case ScatterShape::Plus:
        painter->drawLine(QPointF(center.x() - hs, center.y()),
                          QPointF(center.x() + hs, center.y()));
        painter->drawLine(QPointF(center.x(), center.y() - hs),
                          QPointF(center.x(), center.y() + hs));
        break;
    default:
        break;
    }
    painter->restore();
}

template<typename TKey, typename TValue>
QPolygonF LineGraph<TKey, TValue>::buildDiamond(const QPointF &center, int halfSize)
{
    QPolygonF diamond;
    diamond << QPointF(center.x(), center.y() - halfSize)
            << QPointF(center.x() + halfSize, center.y())
            << QPointF(center.x(), center.y() + halfSize)
            << QPointF(center.x() - halfSize, center.y());
    return diamond;
}

template<typename TKey, typename TValue>
QPolygonF LineGraph<TKey, TValue>::buildTriangle(const QPointF &center, int halfSize)
{
    double h = halfSize * 1.732;
    QPolygonF tri;
    tri << QPointF(center.x(), center.y() - h * 0.667)
        << QPointF(center.x() - halfSize, center.y() + h * 0.333)
        << QPointF(center.x() + halfSize, center.y() + h * 0.333);
    return tri;
}

// ---- BarGraph ----

template<typename TKey, typename TValue>
BarGraph<TKey, TValue>::BarGraph()
{
    this->m_color = m_barColor;
}

template<typename TKey, typename TValue>
void BarGraph<TKey, TValue>::setBarColor(const QColor &color)
{
    m_barColor = color;
    this->m_color = color;
}

template<typename TKey, typename TValue>
void BarGraph<TKey, TValue>::setBarWidthFactor(double factor)
{
    m_barWidthFactor = qBound(0.1, factor, 1.0);
}

template<typename TKey, typename TValue>
void BarGraph<TKey, TValue>::draw(QPainter *painter, BaseAxis *xAxis,
                                   BaseAxis *yAxis, const QRectF &plotArea)
{
    if (!this->isVisible() || this->m_data.isEmpty())
        return;

    auto vr = this->computeVisibleRange(xAxis, plotArea, 1);
    if (vr.start >= vr.end)
        return;

    painter->save();
    painter->setClipRect(plotArea);

    int totalCount = this->m_data.size();
    double totalWidth = plotArea.width();
    double gapWidth = (totalCount <= 1) ? totalWidth * 0.3 : totalWidth / totalCount;
    double barWidth = gapWidth * m_barWidthFactor;

    QPen borderPen(m_borderColor, m_borderWidth);
    QBrush barBrush(m_barColor);

    for (int i = vr.start; i < vr.end; ++i) {
        const auto &pt = this->m_data[i];
        double px = xAxis->valueToPixel(QVariant::fromValue(pt.key));
        double py = yAxis->valueToPixel(QVariant::fromValue(pt.value));
        double baseY = yAxis->valueToPixel(QVariant::fromValue(TValue()));
        py    = qBound(plotArea.top(), py,    plotArea.bottom());
        baseY = qBound(plotArea.top(), baseY, plotArea.bottom());

        double barTop = qMin(py, baseY);
        double barH   = qAbs(baseY - py);
        QRectF barRect(px - barWidth / 2.0, barTop, barWidth, barH);

        painter->setPen(borderPen);
        painter->setBrush(barBrush);
        painter->drawRect(barRect);
    }

    painter->restore();
}

// ---- StackedBarGraph ----

template<typename TKey, typename TValue>
StackedBarGraph<TKey, TValue>::StackedBarGraph()
{
    setSeriesCount(1);
}

template<typename TKey, typename TValue>
void StackedBarGraph<TKey, TValue>::setSeriesCount(int count)
{
    count = qMax(1, count);
    m_seriesData.resize(count);
    m_seriesColors.resize(count);
    m_seriesNames.resize(count);
    for (int i = 0; i < count; ++i) {
        if (!m_seriesColors[i].isValid())
            m_seriesColors[i] = defaultColor(i);
        if (m_seriesNames[i].isEmpty())
            m_seriesNames[i] = QString("Series %1").arg(i + 1);
    }
}

template<typename TKey, typename TValue>
void StackedBarGraph<TKey, TValue>::setSeriesData(int seriesIndex,
                                                    const DataVector &data)
{
    if (seriesIndex >= 0 && seriesIndex < m_seriesData.size()) {
        m_seriesData[seriesIndex] = data;
        std::sort(m_seriesData[seriesIndex].begin(), m_seriesData[seriesIndex].end());
    }
}

template<typename TKey, typename TValue>
typename StackedBarGraph<TKey, TValue>::DataVector
StackedBarGraph<TKey, TValue>::seriesData(int seriesIndex) const
{
    if (seriesIndex >= 0 && seriesIndex < m_seriesData.size())
        return m_seriesData[seriesIndex];
    return {};
}

template<typename TKey, typename TValue>
void StackedBarGraph<TKey, TValue>::setSeriesColor(int seriesIndex,
                                                     const QColor &color)
{
    if (seriesIndex >= 0 && seriesIndex < m_seriesColors.size())
        m_seriesColors[seriesIndex] = color;
}

template<typename TKey, typename TValue>
QColor StackedBarGraph<TKey, TValue>::seriesColor(int seriesIndex) const
{
    if (seriesIndex >= 0 && seriesIndex < m_seriesColors.size())
        return m_seriesColors[seriesIndex];
    return Qt::gray;
}

template<typename TKey, typename TValue>
void StackedBarGraph<TKey, TValue>::setSeriesName(int seriesIndex,
                                                    const QString &name)
{
    if (seriesIndex >= 0 && seriesIndex < m_seriesNames.size())
        m_seriesNames[seriesIndex] = name;
}

template<typename TKey, typename TValue>
QString StackedBarGraph<TKey, TValue>::seriesName(int seriesIndex) const
{
    if (seriesIndex >= 0 && seriesIndex < m_seriesNames.size())
        return m_seriesNames[seriesIndex];
    return {};
}

template<typename TKey, typename TValue>
void StackedBarGraph<TKey, TValue>::setBarWidthFactor(double factor)
{
    m_barWidthFactor = qBound(0.1, factor, 1.0);
}

template<typename TKey, typename TValue>
QVector<TKey> StackedBarGraph<TKey, TValue>::collectAllKeys() const
{
    QSet<TKey> keySet;
    for (const auto &series : m_seriesData) {
        for (const auto &pt : series)
            keySet.insert(pt.key);
    }
    QVector<TKey> keys(keySet.cbegin(), keySet.cend());
    std::sort(keys.begin(), keys.end());
    return keys;
}

template<typename TKey, typename TValue>
TValue StackedBarGraph<TKey, TValue>::seriesValueAt(const DataVector &series,
                                                      const TKey &key)
{
    auto it = std::lower_bound(series.cbegin(), series.cend(), key,
                               [](const DataPoint &pt, const TKey &k) {
                                   return pt.key < k;
                               });
    if (it != series.cend() && it->key == key)
        return it->value;
    return TValue();
}

template<typename TKey, typename TValue>
QColor StackedBarGraph<TKey, TValue>::defaultColor(int index) const
{
    static const QVector<QColor> palette = {
        QColor(65, 105, 225),
        QColor(220, 20, 60),
        QColor(50, 205, 50),
        QColor(255, 165, 0),
        QColor(138, 43, 226),
        QColor(0, 206, 209),
        QColor(255, 215, 0),
        QColor(165, 42, 42),
    };
    return palette[index % palette.size()];
}

template<typename TKey, typename TValue>
void StackedBarGraph<TKey, TValue>::draw(QPainter *painter, BaseAxis *xAxis,
                                           BaseAxis *yAxis,
                                           const QRectF &plotArea)
{
    if (!this->isVisible() || m_seriesData.isEmpty())
        return;

    QVector<TKey> allKeys = collectAllKeys();
    if (allKeys.isEmpty() || !xAxis)
        return;

    TKey minKey = qvariant_cast<TKey>(xAxis->pixelToValue(plotArea.left()));
    TKey maxKey = qvariant_cast<TKey>(xAxis->pixelToValue(plotArea.right()));

    int first = std::lower_bound(allKeys.cbegin(), allKeys.cend(), minKey)
                - allKeys.cbegin();
    int last  = std::upper_bound(allKeys.cbegin(), allKeys.cend(), maxKey)
                - allKeys.cbegin();
    first = qMax(0, first - 1);
    last  = qMin(static_cast<int>(allKeys.size()), last + 1);
    if (first >= last)
        return;

    painter->save();
    painter->setClipRect(plotArea);

    int totalCount = allKeys.size();
    double gapWidth = (totalCount <= 1) ? plotArea.width() * 0.3
                                        : plotArea.width() / totalCount;
    double barWidth = gapWidth * m_barWidthFactor;

    QPen borderPen(m_borderColor, m_borderWidth);

    for (int ki = first; ki < last; ++ki) {
        const TKey &key = allKeys[ki];
        double px = xAxis->valueToPixel(QVariant::fromValue(key));
        double barLeft = px - barWidth / 2.0;

        double baseY = yAxis->valueToPixel(QVariant::fromValue(TValue()));
        baseY = qBound(plotArea.top(), baseY, plotArea.bottom());

        TValue cumulative = TValue();
        for (int si = 0; si < m_seriesData.size(); ++si) {
            cumulative = cumulative + seriesValueAt(m_seriesData[si], key);
            double topY = yAxis->valueToPixel(QVariant::fromValue(cumulative));
            topY = qBound(plotArea.top(), topY, plotArea.bottom());

            double segTop = qMin(topY, baseY);
            double segH = qAbs(baseY - topY);

            if (segH >= 0.5) {
                QRectF segmentRect(barLeft, segTop, barWidth, segH);
                painter->setPen(borderPen);
                painter->setBrush(QBrush(m_seriesColors[si]));
                painter->drawRect(segmentRect);
            }

            baseY = topY;
        }
    }

    painter->restore();
}

// ============================================================================
// RangeBarDataPoint
// ============================================================================

template<typename TKey, typename TValue>
struct RangeBarDataPoint {
    TKey key;
    TValue max;
    TValue min;
    bool operator<(const RangeBarDataPoint &other) const { return key < other.key; }
};

// ============================================================================
// RangeBarGraph
// ============================================================================

template<typename TKey, typename TValue>
class RangeBarGraph : public GraphBase
{
public:
    using DataPoint = RangeBarDataPoint<TKey, TValue>;
    using DataVector = QVector<DataPoint>;

    RangeBarGraph();

    void setData(const DataVector &data);
    DataVector data() const { return m_data; }
    void addPoint(const TKey &key, const TValue &min, const TValue &max);
    void removePoint(const TKey &key);
    void clearData() { m_data.clear(); }
    int dataCount() const { return m_data.size(); }

    TKey keyMin() const { return m_data.isEmpty() ? TKey() : m_data.first().key; }
    TKey keyMax() const { return m_data.isEmpty() ? TKey() : m_data.last().key; }

    void setBarColor(const QColor &color) { m_barColor = color; }
    QColor barColor() const { return m_barColor; }
    void setBarWidthFactor(double factor) { m_barWidthFactor = qBound(0.1, factor, 1.0); }
    double barWidthFactor() const { return m_barWidthFactor; }
    void setBorderColor(const QColor &color) { m_borderColor = color; }
    QColor borderColor() const { return m_borderColor; }
    void setBorderWidth(double width) { m_borderWidth = width; }
    double borderWidth() const { return m_borderWidth; }

    void draw(QPainter *painter, BaseAxis *xAxis, BaseAxis *yAxis,
              const QRectF &plotArea) override;

    bool nearestPoint(BaseAxis *xAxis, BaseAxis *yAxis,
                      const QRectF &plotArea, const QPointF &pixel,
                      QVariant &key, QVariant &value, double &distance) const override;

private:
    DataVector m_data;
    QColor m_barColor = QColor(65, 105, 225);
    QColor m_borderColor = Qt::black;
    double m_borderWidth = 1.0;
    double m_barWidthFactor = 0.8;
};

// ---- RangeBarGraph implementations ----

template<typename TKey, typename TValue>
RangeBarGraph<TKey, TValue>::RangeBarGraph()
{
}

template<typename TKey, typename TValue>
void RangeBarGraph<TKey, TValue>::setData(const DataVector &data)
{
    m_data = data;
    std::sort(m_data.begin(), m_data.end());
}

template<typename TKey, typename TValue>
void RangeBarGraph<TKey, TValue>::addPoint(const TKey &key,
                                            const TValue &min,
                                            const TValue &max)
{
    DataPoint pt{key, max, min};
    auto it = std::lower_bound(m_data.begin(), m_data.end(), pt);
    if (it != m_data.end() && it->key == key) {
        it->min = min;
        it->max = max;
    } else {
        m_data.insert(it, pt);
    }
}

template<typename TKey, typename TValue>
void RangeBarGraph<TKey, TValue>::removePoint(const TKey &key)
{
    auto it = std::find_if(m_data.begin(), m_data.end(),
                           [&key](const DataPoint &pt) { return pt.key == key; });
    if (it != m_data.end())
        m_data.erase(it);
}

template<typename TKey, typename TValue>
void RangeBarGraph<TKey, TValue>::draw(QPainter *painter, BaseAxis *xAxis,
                                        BaseAxis *yAxis, const QRectF &plotArea)
{
    if (!this->isVisible() || m_data.isEmpty() || !xAxis)
        return;

    // Viewport culling by key
    TKey minKey = qvariant_cast<TKey>(xAxis->pixelToValue(plotArea.left()));
    TKey maxKey = qvariant_cast<TKey>(xAxis->pixelToValue(plotArea.right()));

    auto cmpLo = [](const DataPoint &pt, const TKey &k) { return pt.key < k; };
    auto cmpHi = [](const TKey &k, const DataPoint &pt) { return k < pt.key; };

    int first = std::lower_bound(m_data.cbegin(), m_data.cend(), minKey, cmpLo)
                - m_data.cbegin();
    int last  = std::upper_bound(m_data.cbegin(), m_data.cend(), maxKey, cmpHi)
                - m_data.cbegin();
    first = qMax(0, first - 1);
    last  = qMin(static_cast<int>(m_data.size()), last + 1);
    if (first >= last)
        return;

    painter->save();
    painter->setClipRect(plotArea);

    int totalCount = m_data.size();
    double gapWidth = (totalCount <= 1) ? plotArea.width() * 0.3
                                        : plotArea.width() / totalCount;
    double barWidth = gapWidth * m_barWidthFactor;
    QPen borderPen(m_borderColor, m_borderWidth);
    QBrush barBrush(m_barColor);

    for (int i = first; i < last; ++i) {
        const auto &pt = m_data[i];
        double px = xAxis->valueToPixel(QVariant::fromValue(pt.key));
        double pyMin = yAxis->valueToPixel(QVariant::fromValue(pt.min));
        double pyMax = yAxis->valueToPixel(QVariant::fromValue(pt.max));

        // Clamp to plot area
        pyMin = qBound(plotArea.top(), pyMin, plotArea.bottom());
        pyMax = qBound(plotArea.top(), pyMax, plotArea.bottom());

        double barTop = qMin(pyMin, pyMax);
        double barH   = qAbs(pyMax - pyMin);

        QRectF barRect(px - barWidth / 2.0, barTop, barWidth, barH);
        painter->setPen(borderPen);
        painter->setBrush(barBrush);
        painter->drawRect(barRect);
    }

    painter->restore();
}

template<typename TKey, typename TValue>
bool RangeBarGraph<TKey, TValue>::nearestPoint(
        BaseAxis *xAxis, BaseAxis *yAxis,
        const QRectF & /*plotArea*/, const QPointF &pixel,
        QVariant &key, QVariant &value, double &distance) const
{
    if (m_data.isEmpty() || !xAxis || !yAxis)
        return false;

    TKey searchKey = qvariant_cast<TKey>(xAxis->pixelToValue(pixel.x()));

    auto cmp = [](const DataPoint &pt, const TKey &k) { return pt.key < k; };
    auto it = std::lower_bound(m_data.cbegin(), m_data.cend(), searchKey, cmp);
    int idx = static_cast<int>(it - m_data.cbegin());

    double bestDist = std::numeric_limits<double>::max();
    int bestIdx = -1;

    int lo = qMax(0, idx - 1);
    int hi = qMin(static_cast<int>(m_data.size()) - 1, idx);

    for (int i = lo; i <= hi; ++i) {
        double px = xAxis->valueToPixel(QVariant::fromValue(m_data[i].key));
        double pyMid = yAxis->valueToPixel(QVariant::fromValue(
            (m_data[i].min + m_data[i].max) / TValue(2)));
        double dx = px - pixel.x();
        double dy = pyMid - pixel.y();
        double dist = qSqrt(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        key = QVariant::fromValue(m_data[bestIdx].key);
        value = QVariant::fromValue((m_data[bestIdx].min + m_data[bestIdx].max)
                                     / TValue(2));
        distance = bestDist;
        return true;
    }

    return false;
}

#endif // CHARTWIDGET_H
