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
// AxisRange — data range (QCPRange-style)
// ============================================================================

class AxisRange
{
public:
    AxisRange() = default;
    AxisRange(double lower, double upper) : mLower(lower), mUpper(upper)
    { if (mLower > mUpper) qSwap(mLower, mUpper); }

    double lower() const { return mLower; }
    double upper() const { return mUpper; }
    void setLower(double l) { mLower = l; if (mLower > mUpper) qSwap(mLower, mUpper); }
    void setUpper(double u) { mUpper = u; if (mLower > mUpper) qSwap(mLower, mUpper); }

    double size() const { return mUpper - mLower; }
    double center() const { return (mUpper + mLower) * 0.5; }

    void set(double l, double u) { mLower = l; mUpper = u; if (mLower > mUpper) qSwap(mLower, mUpper); }
    bool contains(double v) const { return v >= mLower && v <= mUpper; }
    bool isEmpty() const { return qFuzzyCompare(mLower, mUpper); }

    void expand(double v);
    AxisRange expanded(double v) const;
    AxisRange sanitized(double minRange = 1e-10) const;
    AxisRange bounded(double lo, double hi) const;

    static bool validRange(double lo, double hi) { return lo < hi && (hi - lo) < 1e15; }
    static constexpr double minRange() { return 4.0 * 1e-16; }

private:
    double mLower = 0.0;
    double mUpper = 100.0;
};

// ============================================================================
// ChartDataContainer — optimized data storage (QCPDataContainer-style)
// ============================================================================

template<typename DataType>
class ChartDataContainer
{
public:
    ChartDataContainer() = default;

    int size() const { return m_data.size() - m_preallocation; }
    bool isEmpty() const { return size() <= 0; }

    const DataType &at(int i) const { return m_data[m_preallocation + i]; }
    DataType &at(int i) { return m_data[m_preallocation + i]; }
    const DataType &first() const { return at(0); }
    const DataType &last() const { return at(size() - 1); }

    const DataType *constData() const { return m_data.constData() + m_preallocation; }
    const DataType *constBegin() const { return constData(); }
    const DataType *constEnd() const { return constData() + size(); }

    int preallocation() const { return m_preallocation; }

    void add(const DataType &item);
    void set(const QVector<DataType> &data, bool alreadySorted);
    void clear();

private:
    QVector<DataType> m_data;
    int m_preallocation = 0;
};

template<typename DataType>
void ChartDataContainer<DataType>::add(const DataType &item)
{
    if (isEmpty()) {
        m_data.append(item);
        return;
    }
    // Fast O(1) append
    if (!(item < last())) {
        m_data.append(item);
        return;
    }
    // Fast O(1) prepend
    if (item < first()) {
        if (m_preallocation > 0) {
            m_data[--m_preallocation] = item;
        } else {
            m_data.prepend(item);
        }
        return;
    }
    // Binary search insert
    auto it = std::lower_bound(constBegin(), constEnd(), item);
    m_data.insert(it - m_data.constData(), item);
}

template<typename DataType>
void ChartDataContainer<DataType>::set(const QVector<DataType> &data, bool alreadySorted)
{
    m_data = data;
    m_preallocation = 0;
    if (!alreadySorted)
        std::sort(m_data.begin(), m_data.end());
}

template<typename DataType>
void ChartDataContainer<DataType>::clear()
{
    m_data.clear();
    m_preallocation = 0;
}

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

    virtual void setLayer(int layer) { m_layer = layer; }
    int layer() const { return m_layer; }

    virtual void setName(const QString &name) { m_name = name; }
    QString name() const { return m_name; }

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    virtual void setColor(const QColor &c) { m_color = c; }
    QColor color() const { return m_color; }

private:
    int m_layer = 0;
    QString m_name;
    bool m_visible = true;
    QColor m_color;
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
// DateTimeFormat — axis label format presets
// ============================================================================

enum class DateTimeFormat {
    HHmm,           // 14:30
    HHmmss,         // 14:30:05
    MMdd,           // 05-18
    MMddHHmm,       // 05-18 14:30
    yyyyMMdd,       // 2026-05-18
    yyyyMMddHHmm,   // 2026-05-18 14:30
    yyyyMM,         // 2026-05
    MMMyy           // May 26
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
// AxisTicker — independent tick strategy (QCustomPlot-style)
// ============================================================================

class AxisTicker
{
public:
    virtual ~AxisTicker() = default;

    virtual double getTickStep(double rangeSize) const = 0;
    virtual QVector<double> createTickVector(double tickStep, double rangeMin,
                                              double rangeMax) const;
    virtual int getSubTickCount(double tickStep) const;
    virtual QVector<double> createSubTickVector(int subTickCount,
                                                 const QVector<double> &ticks) const;
    virtual QString getTickLabel(double value) const = 0;
    QVector<QString> createLabelVector(const QVector<double> &ticks) const;

    void setTickCount(int count) { m_tickCount = qMax(2, count); }
    int tickCount() const { return m_tickCount; }

    void setTickOrigin(double origin) { m_tickOrigin = origin; }
    double tickOrigin() const { return m_tickOrigin; }

    static void trimTicks(double rangeMin, double rangeMax,
                          QVector<double> &ticks, bool keepOneOutlier = true);

protected:
    int m_tickCount = 5;
    double m_tickOrigin = 0.0;

    static double cleanMantissa(double input);
    static double getMantissa(double input, double *magnitude = nullptr);
    static double pickClosest(double target, const QVector<double> &candidates);
};

// ============================================================================
// NumericTicker
// ============================================================================

class NumericTicker : public AxisTicker
{
public:
    double getTickStep(double rangeSize) const override;
    int getSubTickCount(double tickStep) const override;
    QString getTickLabel(double value) const override;

    void setDecimalPlaces(int dp) { m_decimalPlaces = dp; }
    int decimalPlaces() const { return m_decimalPlaces; }

    void setLabelFormat(const QString &fmt) { m_labelFormat = fmt; }
    QString labelFormat() const { return m_labelFormat; }

private:
    int m_decimalPlaces = 0;
    QString m_labelFormat;
};

// ============================================================================
// DateTimeTicker — seconds to years
// ============================================================================

class DateTimeTicker : public AxisTicker
{
public:
    double getTickStep(double rangeSize) const override;
    int getSubTickCount(double tickStep) const override;
    QString getTickLabel(double value) const override;

    void setDateTimeFormat(DateTimeFormat f) { m_format = f; }
    DateTimeFormat dateTimeFormat() const { return m_format; }

    enum DateStrategy { dsNone, dsUniformTimeInDay, dsUniformDayInMonth };
    DateStrategy dateStrategy() const { return m_dateStrategy; }

    static QString formatString(DateTimeFormat f);

private:
    DateTimeFormat m_format = DateTimeFormat::HHmm;
    mutable DateStrategy m_dateStrategy = dsNone;
};

// ============================================================================
// DateTicker — days to years
// ============================================================================

class DateTicker : public AxisTicker
{
public:
    double getTickStep(double rangeSize) const override;
    int getSubTickCount(double tickStep) const override;
    QString getTickLabel(double value) const override;

    void setDateTimeFormat(DateTimeFormat f) { m_format = f; }
    DateTimeFormat dateTimeFormat() const { return m_format; }

    static QString formatString(DateTimeFormat f);

private:
    DateTimeFormat m_format = DateTimeFormat::yyyyMMdd;
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
    virtual void draw(QPainter *painter, const QRectF &axisRect,
                      const QRectF &plotArea) = 0;
    virtual void zoomRange(double factor, const QVariant &center) = 0;
    virtual void panRange(double pixelDelta, double plotAreaSize) = 0;

    // Ticker (runtime-replaceable strategy)
    void setTicker(AxisTicker *ticker);
    AxisTicker *ticker() const { return m_ticker; }

    // Sub-ticks
    void setSubTickCount(int count) { m_subTickCount = count; }
    int subTickCount() const { return m_subTickCount; }
    QVector<double> subTickPixelPositions() const { return m_subTickPixelPositions; }

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    void setZoomEnabled(bool enabled) { m_zoomEnabled = enabled; }
    bool isZoomEnabled() const { return m_zoomEnabled; }

    void setPanEnabled(bool enabled) { m_panEnabled = enabled; }
    bool isPanEnabled() const { return m_panEnabled; }

    void setTickCount(int count);
    int tickCount() const;

    void setShowTickLabels(bool show) { m_showTickLabels = show; }
    bool showTickLabels() const { return m_showTickLabels; }

    void setAxisColor(const QColor &color) { m_axisColor = color; }
    QColor axisColor() const { return m_axisColor; }

    void setTickColor(const QColor &color) { m_tickColor = color; }
    QColor tickColor() const { return m_tickColor; }

    void setSubTickColor(const QColor &color) { m_subTickColor = color; }
    QColor subTickColor() const { return m_subTickColor; }

    void setLabelColor(const QColor &color) { m_labelColor = color; }
    QColor labelColor() const { return m_labelColor; }

    void setLabelFont(const QFont &font) { m_labelFont = font; }
    QFont labelFont() const { return m_labelFont; }

    void setAxisLineWidth(double width) { m_axisLineWidth = width; }
    double axisLineWidth() const { return m_axisLineWidth; }

    void setTickLength(double length) { m_tickLength = length; }
    double tickLength() const { return m_tickLength; }

    void setSubTickLength(double length) { m_subTickLength = length; }
    double subTickLength() const { return m_subTickLength; }

    QVector<QVariant> tickValues() const { return m_tickValues; }
    QVector<double> tickPixelPositions() const { return m_tickPixelPositions; }

    void shiftTickPixelPositions(double delta);
    void setSkipTickRecalc(bool skip) { m_skipTickRecalc = skip; }
    bool skipTickRecalc() const { return m_skipTickRecalc; }

    // Tick label rotation (degrees, 0 = horizontal)
    void setTickLabelRotation(double degrees) { m_tickLabelRotation = degrees; }
    double tickLabelRotation() const { return m_tickLabelRotation; }

    // Axis line ending style
    enum AxisEnding { EndingNone, EndingArrow, EndingDisc };
    void setLowerEnding(AxisEnding e) { m_lowerEnding = e; }
    AxisEnding lowerEnding() const { return m_lowerEnding; }
    void setUpperEnding(AxisEnding e) { m_upperEnding = e; }
    AxisEnding upperEnding() const { return m_upperEnding; }

protected:
    void calculateTicks();
    virtual double tickRangeMin() const = 0;
    virtual double tickRangeMax() const = 0;
    virtual QVariant tickValueToVariant(double v) const = 0;
    void drawSubTicks(QPainter *painter, const QRectF &plotArea) const;

    AxisOrientation m_orientation;
    bool m_visible = true;
    bool m_zoomEnabled = true;
    bool m_panEnabled = true;
    bool m_showTickLabels = true;
    QColor m_axisColor = Qt::black;
    QColor m_tickColor = Qt::black;
    QColor m_subTickColor{120, 120, 120};
    QColor m_labelColor = Qt::black;
    QFont m_labelFont;
    double m_axisLineWidth = 1.5;
    double m_tickLength = 5.0;
    double m_subTickLength = 2.5;

    QVector<QVariant> m_tickValues;
    QVector<double> m_tickPixelPositions;
    QVector<double> m_subTickPixelPositions;
    QVector<double> m_tickValuesDouble;

    double m_tickLabelRotation = 0.0;
    AxisEnding m_lowerEnding = EndingNone;
    AxisEnding m_upperEnding = EndingNone;

    QRectF m_cachePlotArea;

    AxisTicker *m_ticker = nullptr;
    bool m_ownsTicker = false;
    int m_subTickCount = 4;
    bool m_skipTickRecalc = false;

    double niceNumber(double x, bool roundUp) const;
    void drawAxisEnding(QPainter *painter, AxisEnding ending,
                        const QPointF &pos, double directionAngle) const;
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
    double rangeMin() const { return m_range.lower(); }
    double rangeMax() const { return m_range.upper(); }
    AxisRange &range() { return m_range; }
    const AxisRange &range() const { return m_range; }

    void setDecimalPlaces(int places);
    int decimalPlaces() const;

    void setLabelFormat(const QString &format);
    QString labelFormat() const;

    double valueToPixel(const QVariant &value) const override;
    QVariant pixelToValue(double pixel) const override;
    void draw(QPainter *painter, const QRectF &axisRect,
              const QRectF &plotArea) override;
    void zoomRange(double factor, const QVariant &center) override;
    void panRange(double pixelDelta, double plotAreaSize) override;

protected:
    double tickRangeMin() const override { return m_range.lower(); }
    double tickRangeMax() const override { return m_range.upper(); }
    QVariant tickValueToVariant(double v) const override { return QVariant(v); }

private:
    AxisRange m_range{0.0, 100.0};
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
    QDateTime rangeMin() const;
    QDateTime rangeMax() const;
    AxisRange &range() { return m_range; }
    const AxisRange &range() const { return m_range; }

    void setDateTimeFormat(DateTimeFormat f);
    DateTimeFormat dateTimeFormat() const;

    double valueToPixel(const QVariant &value) const override;
    QVariant pixelToValue(double pixel) const override;
    void draw(QPainter *painter, const QRectF &axisRect,
              const QRectF &plotArea) override;
    void zoomRange(double factor, const QVariant &center) override;
    void panRange(double pixelDelta, double plotAreaSize) override;
    void resetPanRemainder() { m_panRemainder = 0.0; }

protected:
    double tickRangeMin() const override { return m_range.lower(); }
    double tickRangeMax() const override { return m_range.upper(); }
    QVariant tickValueToVariant(double v) const override;

private:
    AxisRange m_range;
    double m_panRemainder = 0.0;
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
    QDate rangeMin() const;
    QDate rangeMax() const;
    AxisRange &range() { return m_range; }
    const AxisRange &range() const { return m_range; }

    void setDateTimeFormat(DateTimeFormat f);
    DateTimeFormat dateTimeFormat() const;

    double valueToPixel(const QVariant &value) const override;
    QVariant pixelToValue(double pixel) const override;
    void draw(QPainter *painter, const QRectF &axisRect,
              const QRectF &plotArea) override;
    void zoomRange(double factor, const QVariant &center) override;
    void panRange(double pixelDelta, double plotAreaSize) override;
    void resetPanRemainder() { m_panRemainder = 0.0; }

protected:
    double tickRangeMin() const override { return m_range.lower(); }
    double tickRangeMax() const override { return m_range.upper(); }
    QVariant tickValueToVariant(double v) const override;

private:
    AxisRange m_range;
    double m_panRemainder = 0.0;
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
    using DataContainer = ChartDataContainer<DataPoint>;
    using DataVector = QVector<DataPoint>;

    void setData(const DataVector &data);
    DataVector data() const;
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
        for (int i = 0; i < m_data.size(); ++i) {
            if (m_data.at(i).value < minVal) minVal = m_data.at(i).value;
        }
        return minVal;
    }

    TValue valueMax() const
    {
        if (m_data.isEmpty()) return TValue();
        TValue maxVal = m_data.first().value;
        for (int i = 0; i < m_data.size(); ++i) {
            if (maxVal < m_data.at(i).value) maxVal = m_data.at(i).value;
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
    DataContainer m_data;
    QColor m_color = Qt::blue;

    struct VisibleRange { int start = 0; int end = 0; };
    VisibleRange computeVisibleRange(BaseAxis *xAxis, const QRectF &plotArea,
                                     int extend = 1) const;
};

template<typename TKey, typename TValue>
void AbstractGraph<TKey, TValue>::setData(const DataVector &data)
{
    m_data.set(data, false);
}

template<typename TKey, typename TValue>
typename AbstractGraph<TKey, TValue>::DataVector
AbstractGraph<TKey, TValue>::data() const
{
    DataVector v;
    v.reserve(m_data.size());
    for (int i = 0; i < m_data.size(); ++i) v.append(m_data.at(i));
    return v;
}

template<typename TKey, typename TValue>
void AbstractGraph<TKey, TValue>::addPoint(const TKey &key, const TValue &value)
{
    m_data.add(DataPoint{key, value});
}

template<typename TKey, typename TValue>
void AbstractGraph<TKey, TValue>::removePoint(const TKey &key)
{
    DataVector copy;
    copy.reserve(m_data.size());
    for (int i = 0; i < m_data.size(); ++i) {
        if (!(m_data.at(i).key == key)) copy.append(m_data.at(i));
    }
    m_data.set(copy, true);
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
    auto it = std::lower_bound(m_data.constBegin(), m_data.constEnd(), searchKey, cmp);
    int idx = static_cast<int>(it - m_data.constBegin());

    double bestDist = std::numeric_limits<double>::max();
    int bestIdx = -1;

    int lo = qMax(0, idx - 1);
    int hi = qMin(m_data.size() - 1, idx);

    for (int i = lo; i <= hi; ++i) {
        double px = xAxis->valueToPixel(QVariant::fromValue(m_data.at(i).key));
        double py = yAxis->valueToPixel(QVariant::fromValue(m_data.at(i).value));
        double dx = px - pixel.x();
        double dy = py - pixel.y();
        double dist = qSqrt(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        key = QVariant::fromValue(m_data.at(bestIdx).key);
        value = QVariant::fromValue(m_data.at(bestIdx).value);
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
    void chartClicked(QPoint pos);

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

    int first = std::lower_bound(m_data.constBegin(), m_data.constEnd(), minKey, cmpLo)
                - m_data.constBegin();
    int last  = std::upper_bound(m_data.constBegin(), m_data.constEnd(), maxKey, cmpHi)
                - m_data.constBegin();

    first = qMax(0, first - extend);
    last  = qMin(m_data.size(), last + extend);

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
        const auto &pt = this->m_data.at(i);
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
        const auto &pt = this->m_data.at(i);
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

// ============================================================================
// ComboGraph — RangeBarGraph + LineGraph combo (shared color, mid-aligned)
// ============================================================================

template<typename TKey, typename TValue>
class ComboGraph : public GraphBase
{
public:
    ComboGraph();
    ~ComboGraph() override;

    void addPoint(const TKey &key, const TValue &lo, const TValue &hi);
    void clearData();

    void setColor(const QColor &c);
    QColor color() const { return m_color; }

    void setSelected(bool sel);
    bool isSelected() const { return m_selected; }

    void setLayer(int layer) override;
    void setName(const QString &name) override;

    RangeBarGraph<TKey, TValue> *rangeBar() const { return m_rangeBar; }
    LineGraph<TKey, TValue> *line() const { return m_line; }

    void draw(QPainter *painter, BaseAxis *xAxis, BaseAxis *yAxis,
              const QRectF &plotArea) override;
    bool nearestPoint(BaseAxis *xAxis, BaseAxis *yAxis,
                      const QRectF &plotArea, const QPointF &pixel,
                      QVariant &key, QVariant &value, double &distance) const override;

private:
    RangeBarGraph<TKey, TValue> *m_rangeBar = nullptr;
    LineGraph<TKey, TValue> *m_line = nullptr;
    QColor m_color{70, 130, 180};
    bool m_selected = false;
    int m_baseAlpha = 40;
    int m_savedLayer = 0;

    void applyOpacityToGraph(GraphBase *g, const QColor &baseColor, int alpha);
};

template<typename TKey, typename TValue>
ComboGraph<TKey, TValue>::ComboGraph()
{
    m_rangeBar = new RangeBarGraph<TKey, TValue>;
    m_line = new LineGraph<TKey, TValue>;
    m_line->setAdaptiveSampling(false);

    ScatterFormat sf;
    sf.shape = ScatterShape::Circle;
    sf.size = 6;
    line()->setScatterFormat(sf);
}

template<typename TKey, typename TValue>
ComboGraph<TKey, TValue>::~ComboGraph()
{
    delete m_rangeBar;
    delete m_line;
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::addPoint(const TKey &key,
                                         const TValue &lo, const TValue &hi)
{
    m_rangeBar->addPoint(key, lo, hi);
    m_line->addPoint(key, (lo + hi) / TValue(2));
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::clearData()
{
    m_rangeBar->clearData();
    m_line->clearData();
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::setColor(const QColor &c)
{
    m_color = c;
    int alpha = m_selected ? 255 : m_baseAlpha;
    applyOpacityToGraph(m_rangeBar, m_color, alpha);
    applyOpacityToGraph(m_line, m_color, alpha);
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::setSelected(bool sel)
{
    if (sel == m_selected) return;
    m_selected = sel;
    if (sel) {
        m_savedLayer = layer();
        int topLayer = 1000;
        setLayer(topLayer);
        m_rangeBar->setLayer(topLayer);
        m_line->setLayer(topLayer);
    } else {
        setLayer(m_savedLayer);
        m_rangeBar->setLayer(m_savedLayer);
        m_line->setLayer(m_savedLayer);
    }
    setColor(m_color); // re-apply with correct alpha
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::setLayer(int layer)
{
    GraphBase::setLayer(layer);
    m_rangeBar->setLayer(layer);
    m_line->setLayer(layer);
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::setName(const QString &name)
{
    GraphBase::setName(name);
    m_rangeBar->setName(name + " range");
    m_line->setName(name + " line");
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::applyOpacityToGraph(GraphBase *g,
    const QColor &baseColor, int alpha)
{
    QColor c = baseColor;
    c.setAlpha(alpha);
    g->setColor(c);

    if (auto *rb = dynamic_cast<RangeBarGraph<TKey, TValue> *>(g)) {
        QColor border = baseColor;
        border.setAlpha(alpha);
        rb->setBarColor(c);
        rb->setBorderColor(border);
    }
    if (auto *lg = dynamic_cast<LineGraph<TKey, TValue> *>(g)) {
        lg->setLineColor(c);
        ScatterFormat sf = lg->scatterFormat();
        QColor sc = baseColor;
        sc.setAlpha(alpha);
        sf.color = sc;
        sf.fillColor = sc;
        lg->setScatterFormat(sf);
    }
}

template<typename TKey, typename TValue>
void ComboGraph<TKey, TValue>::draw(QPainter *painter, BaseAxis *xAxis,
                                     BaseAxis *yAxis, const QRectF &plotArea)
{
    if (!isVisible()) return;
    m_rangeBar->draw(painter, xAxis, yAxis, plotArea);
    m_line->draw(painter, xAxis, yAxis, plotArea);
}

template<typename TKey, typename TValue>
bool ComboGraph<TKey, TValue>::nearestPoint(BaseAxis *xAxis, BaseAxis *yAxis,
    const QRectF &plotArea, const QPointF &pixel,
    QVariant &key, QVariant &value, double &distance) const
{
    double distR = -1, distL = -1;
    QVariant kR, vR, kL, vL;
    bool okR = m_rangeBar->nearestPoint(xAxis, yAxis, plotArea, pixel, kR, vR, distR);
    bool okL = m_line->nearestPoint(xAxis, yAxis, plotArea, pixel, kL, vL, distL);

    if (okR && okL) {
        if (distR <= distL) { key = kR; value = vR; distance = distR; }
        else                { key = kL; value = vL; distance = distL; }
        return true;
    }
    if (okR) { key = kR; value = vR; distance = distR; return true; }
    if (okL) { key = kL; value = vL; distance = distL; return true; }
    return false;
}

#endif // CHARTWIDGET_H
