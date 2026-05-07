#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QDateTime>
#include <QPointF>
#include <QRectF>
#include <QMargins>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>
#include <QMap>
#include <QHash>
#include <QPair>
#include <QObject>
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <cmath>

// ------------------------------------------------------------------------
// Enums
// ------------------------------------------------------------------------
enum class AxisType {
    Numeric,
    Date,
    Category
};

enum class DateFormat {
    MMdd,       // "05-06"
    HHmm,       // "14:30"
    MMddHHmm    // "05-06 14:30"
};

enum class ScatterStyle {
    None,
    Circle,
    Square,
    Diamond,
    Triangle
};

// ------------------------------------------------------------------------
// Legend (value type)
// ------------------------------------------------------------------------
struct Legend {
    enum Position {
        InsideChart,
        AboveChart,
        BelowChart
    };
    enum Orientation {
        Horizontal,
        Vertical
    };

    Position     position      = InsideChart;
    Orientation  orientation   = Horizontal;
    QColor       borderColor   = QColor(180, 180, 180);
    QColor       backgroundColor = QColor(255, 255, 255, 235);
    QColor       textColor     = QColor(50, 50, 50);
    QFont        font          = QFont("Arial", 9);
};

// ------------------------------------------------------------------------
// ChartTheme
// ------------------------------------------------------------------------
class ChartTheme {
public:
    QColor background  = QColor(255, 255, 255);
    QColor titleColor  = QColor(50, 50, 50);
    QColor gridColor   = QColor(210, 210, 210);
    QColor subGridColor = QColor(235, 235, 235);
    QColor axisLineColor = QColor(180, 180, 180);
    QColor axisLabelColor = QColor(100, 100, 100);

    static ChartTheme light() {
        ChartTheme t;
        t.background  = QColor(255, 255, 255);
        t.titleColor  = QColor(50, 50, 50);
        t.gridColor   = QColor(210, 210, 210);
        t.subGridColor = QColor(235, 235, 235);
        t.axisLineColor = QColor(180, 180, 180);
        t.axisLabelColor = QColor(100, 100, 100);
        return t;
    }

    static ChartTheme dark() {
        ChartTheme t;
        t.background  = QColor(40, 40, 48);
        t.titleColor  = QColor(210, 210, 210);
        t.gridColor   = QColor(70, 70, 80);
        t.subGridColor = QColor(55, 55, 65);
        t.axisLineColor = QColor(100, 100, 110);
        t.axisLabelColor = QColor(160, 160, 160);
        return t;
    }
};

// ------------------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------------------
class LineSeries;
class BarSeries;
class StackedBarSeries;
class Axis;
class ChartModel;
class ChartLayout;
class ChartWidget;

// ------------------------------------------------------------------------
// Series (abstract base)
// ------------------------------------------------------------------------
class Series : public QObject {
    Q_OBJECT
public:
    explicit Series(const QString &name, QObject *parent = nullptr);
    virtual ~Series();

    QString name() const { return m_name; }
    QColor  color() const { return m_color; }
    bool    isVisible() const { return m_visible; }
    int     index() const { return m_index; }

    void setName(const QString &name);
    void setColor(const QColor &color);
    void setVisible(bool visible);
    void setIndex(int idx) { m_index = idx; }

    bool showInLegend() const { return m_showInLegend; }
    void setShowInLegend(bool show) { m_showInLegend = show; }

    virtual int  dataCount() const = 0;
    virtual void clear() = 0;

signals:
    void dataChanged();

private:
    QString m_name;
    QColor  m_color;
    bool    m_visible = true;
    int     m_index   = -1;
    bool    m_showInLegend = true;
};

// ------------------------------------------------------------------------
// LineSeries — 折线/散点图数据系列
// ------------------------------------------------------------------------
class LineSeries : public Series {
    Q_OBJECT
public:
    struct DataPoint {
        double key;
        double value;
    };

    explicit LineSeries(const QString &name, QObject *parent = nullptr);

    // data management
    void append(double key, double value);
    void append(const QDateTime &time, double value);
    void removeAt(int index);
    void setData(const QVector<DataPoint> &data) { m_data = data; emit dataChanged(); }
    int  dataCount() const override { return m_data.size(); }
    void clear() override { m_data.clear(); emit dataChanged(); }

    const DataPoint &dataAt(int index) const { return m_data.at(index); }
    const QVector<DataPoint> &allData() const { return m_data; }

    // styling
    void setScatterStyle(ScatterStyle style) { m_scatterStyle = style; }
    ScatterStyle scatterStyle() const { return m_scatterStyle; }

    void setMarkerSize(double size) { m_markerSize = size; }
    double markerSize() const { return m_markerSize; }

    void setFillEnabled(bool enabled) { m_fillEnabled = enabled; }
    bool fillEnabled() const { return m_fillEnabled; }

    void setFillBrush(const QBrush &brush) { m_fillBrush = brush; }
    QBrush fillBrush() const { return m_fillBrush; }

    void setLineWidth(double w) { m_lineWidth = w; }
    double lineWidth() const { return m_lineWidth; }

private:
    QVector<DataPoint> m_data;
    ScatterStyle m_scatterStyle = ScatterStyle::None;
    double m_markerSize = 5.0;
    bool   m_fillEnabled = false;
    QBrush m_fillBrush = Qt::NoBrush;
    double m_lineWidth = 1.5;
};

// ------------------------------------------------------------------------
// BarSeries — 柱状图数据系列（分组显示，带 key-value 数据点）
// ------------------------------------------------------------------------
class BarSeries : public Series {
    Q_OBJECT
public:
    struct DataPoint {
        double key;
        double value;
    };

    explicit BarSeries(const QString &name, QObject *parent = nullptr);

    void append(double key, double value) { m_data.append({key, value}); emit dataChanged(); }
    void setData(const QVector<DataPoint> &data) { m_data = data; emit dataChanged(); }
    int  dataCount() const override { return m_data.size(); }
    void clear() override { m_data.clear(); emit dataChanged(); }

    const DataPoint &dataAt(int index) const { return m_data.at(index); }
    const QVector<DataPoint> &allData() const { return m_data; }

private:
    QVector<DataPoint> m_data;
};

// ------------------------------------------------------------------------
// StackedBarSeries — 堆叠柱状图数据系列（带 key-value 数据点）
// ------------------------------------------------------------------------
class StackedBarSeries : public Series {
    Q_OBJECT
public:
    struct DataPoint {
        double key;
        double value;
    };

    explicit StackedBarSeries(const QString &name, QObject *parent = nullptr);

    void append(double key, double value) { m_data.append({key, value}); emit dataChanged(); }
    void setData(const QVector<DataPoint> &data) { m_data = data; emit dataChanged(); }
    int  dataCount() const override { return m_data.size(); }
    void clear() override { m_data.clear(); emit dataChanged(); }

    const DataPoint &dataAt(int index) const { return m_data.at(index); }
    const QVector<DataPoint> &allData() const { return m_data; }

private:
    QVector<DataPoint> m_data;
};

// ------------------------------------------------------------------------
// RangeBarSeries — 柱状图，每个数据点有最小值/最大值（如温度范围）
// ------------------------------------------------------------------------
class RangeBarSeries : public Series {
    Q_OBJECT
public:
    struct DataPoint {
        double key;
        double minValue;
        double maxValue;
    };

    explicit RangeBarSeries(const QString &name, QObject *parent = nullptr);

    void append(double key, double minValue, double maxValue) { m_data.append({key, minValue, maxValue}); emit dataChanged(); }
    void setData(const QVector<DataPoint> &data) { m_data = data; emit dataChanged(); }
    int  dataCount() const override { return m_data.size(); }
    void clear() override { m_data.clear(); emit dataChanged(); }

    const DataPoint &dataAt(int index) const { return m_data.at(index); }
    const QVector<DataPoint> &allData() const { return m_data; }

private:
    QVector<DataPoint> m_data;
};

// ------------------------------------------------------------------------
// Axis — 坐标轴（含刻度、网格、坐标变换）
// ------------------------------------------------------------------------
class Axis : public QObject {
    Q_OBJECT
public:
    explicit Axis(bool vertical, QObject *parent = nullptr);

    // type & format
    void setType(AxisType type) { m_type = type; }
    AxisType type() const { return m_type; }

    void setDateFormat(DateFormat fmt) { m_dateFormat = fmt; }
    DateFormat dateFormat() const { return m_dateFormat; }

    // range
    void setRange(double min, double max);
    void setMin(double min) { setRange(min, m_max); }
    void setMax(double max) { setRange(m_min, max); }
    double min() const { return m_min; }
    double max() const { return m_max; }
    QPair<double, double> range() const { return {m_min, m_max}; }

    // title
    void setTitle(const QString &title) { m_title = title; }
    QString title() const { return m_title; }

    void setTitleColor(const QColor &c) { m_titleColor = c; }
    QColor titleColor() const { return m_titleColor; }

    // ticks
    void setTickCount(int count) { m_tickCount = qMax(2, count); }
    int  tickCount() const { return m_tickCount; }

    void setSubTickCount(int count) { m_subTickCount = count; }
    int  subTickCount() const { return m_subTickCount; }

    void setTickColor(const QColor &c) { m_tickColor = c; }
    QColor tickColor() const { return m_tickColor; }

    void setSubTickColor(const QColor &c) { m_subTickColor = c; }
    QColor subTickColor() const { return m_subTickColor; }

    void setGridColor(const QColor &c) { m_gridColor = c; }
    QColor gridColor() const { return m_gridColor; }

    // geometry (set by layout)
    void setRect(const QRectF &rect) { m_rect = rect; }
    void setGridRect(const QRectF &rect) { m_gridRect = rect; }
    QRectF rect() const { return m_rect; }
    QRectF gridRect() const { return m_gridRect; }
    bool isVertical() const { return m_vertical; }

    // coordinate transforms
    double coordToPixel(double value) const;
    double pixelToCoord(double pixel) const;

    // tick calculation
    void recalculateTicks();
    const QVector<double> &ticks() const { return m_ticks; }
    const QVector<double> &subTicks() const { return m_subTicks; }
    const QVector<QString> &tickLabels() const { return m_tickLabels; }

    // drawing (called by paint engine)
    void drawGrid(QPainter *p) const;
    void drawSubGrid(QPainter *p) const;
    void drawAxis(QPainter *p) const;
    void drawLabels(QPainter *p) const;
    void drawTitle(QPainter *p) const;

signals:
    void rangeChanged();

private:
    QString tickLabelText(double value) const;

    bool     m_vertical;
    AxisType m_type    = AxisType::Numeric;
    DateFormat m_dateFormat = DateFormat::MMdd;

    double   m_min = 0, m_max = 1;
    QString  m_title;
    int      m_tickCount = 5;
    int      m_subTickCount = 2;

    QColor   m_tickColor    = QColor(120, 120, 120);
    QColor   m_subTickColor = QColor(200, 200, 200);
    QColor   m_gridColor    = QColor(210, 210, 210);
    QColor   m_titleColor   = QColor(50, 50, 50);

    QRectF   m_rect;     // pixel rect for axis labels/ticks
    QRectF   m_gridRect; // pixel rect for grid lines (usually the chart plot area)

    // calculated
    QVector<double>   m_ticks;
    QVector<double>   m_subTicks;
    QVector<QString>  m_tickLabels;
};

// ------------------------------------------------------------------------
// ChartModel — 数据模型（管理 series & axes，发射信号）
// ------------------------------------------------------------------------
class ChartModel : public QObject {
    Q_OBJECT
public:
    explicit ChartModel(QObject *parent = nullptr);
    ~ChartModel() override;

    void addSeries(Series *series);
    void removeSeries(Series *series);
    void clearSeries();
    const QList<Series*> &seriesList() const { return m_series; }

    template<typename T>
    QList<T*> seriesByType() const {
        QList<T*> result;
        for (auto *s : m_series)
            if (auto *t = qobject_cast<T*>(s))
                result.append(t);
        return result;
    }

    void addAxis(Axis *axis);
    void removeAxis(Axis *axis);
    const QList<Axis*> &axes() const { return m_axes; }

    void setTitle(const QString &title) { m_title = title; emit titleChanged(); }
    QString title() const { return m_title; }

    void setTheme(const ChartTheme &theme);
    ChartTheme theme() const { return m_theme; }

signals:
    void axisAdded(Axis *axis);
    void axisRemoved(Axis *axis);
    void seriesAdded(Series *s);
    void seriesRemoved(Series *s);
    void dataChanged();
    void titleChanged();
    void themeChanged();

private:
    void connectSeries(Series *s);

    QList<Series*> m_series;
    QList<Axis*>   m_axes;
    QString        m_title;
    ChartTheme     m_theme;
};

// ------------------------------------------------------------------------
// ChartLayout — 布局计算（标题、轴标签、图例、绘图区域）
// ------------------------------------------------------------------------
class ChartLayout : public QObject {
    Q_OBJECT
public:
    struct LayoutInfo {
        QRectF chartArea;   // area where data is drawn
        QRectF titleRect;
        QRectF legendRect;
        QRectF xAxisRect;
        QRectF yAxisRect;
        QMarginsF margins;
    };

    explicit ChartLayout(QObject *parent = nullptr);

    LayoutInfo calculate(const QSize &widgetSize,
                         const QString &title,
                         const QFont &titleFont,
                         Axis *xAxis,
                         Axis *yAxis,
                         const Legend &legend,
                         const QList<Series*> &series) const;

    void setTitleHeight(int h) { m_titleHeight = h; }
    int  titleHeight() const { return m_titleHeight; }

    void setLegendMargin(int m) { m_legendMargin = m; }
    int  legendMargin() const { return m_legendMargin; }

    void setPlotMargin(int m) { m_plotMargin = m; }
    int  plotMargin() const { return m_plotMargin; }

private:
    QSizeF measureLegend(const Legend &legend, const QList<Series*> &series) const;
    int m_titleHeight  = 40;
    int m_legendMargin = 5;
    int m_plotMargin   = 10;
};

// ------------------------------------------------------------------------
// PaintBuffer — 离屏渲染缓冲
// ------------------------------------------------------------------------
class PaintBuffer {
public:
    explicit PaintBuffer();
    ~PaintBuffer();

    void resize(const QSize &size, double devicePixelRatio = 1.0);
    QPainter *beginPainting(const QColor &clearColor);
    void endPainting();
    void paint(QPainter *target) const;
    QPixmap toPixmap() const { return m_pixmap; }
    QSize size() const { return m_pixmap.size(); }
    bool isValid() const { return !m_pixmap.isNull(); }

private:
    QPixmap  m_pixmap;
    QPainter *m_activePainter = nullptr;
};

// ------------------------------------------------------------------------
// ChartWidget — 主 Widget
// ------------------------------------------------------------------------
class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget() override;

    // title
    void setTitle(const QString &title) { m_model->setTitle(title); }
    QString title() const { return m_model->title(); }

    // axes
    Axis *axisX() const { return m_xAxis; }
    Axis *axisY() const { return m_yAxis; }

    // series
    void addSeries(Series *series);
    void removeSeries(Series *series);
    void clearSeries();

    // legend
    void setLegendOrientation(Legend::Orientation o) { m_legend.orientation = o; }
    void setLegendPosition(Legend::Position p) { m_legend.position = p; }
    void setLegend(const Legend &legend) { m_legend = legend; }
    Legend legend() const { return m_legend; }

    // theme
    void setTheme(const ChartTheme &theme);
    ChartTheme theme() const { return m_model->theme(); }

    // rendering
    void refresh();
    QPixmap exportToPixmap(const QSize &size);

    // convenience
    void setTitleFont(const QFont &f) { m_titleFont = f; }
    QFont titleFont() const { return m_titleFont; }

signals:
    void dataPointHovered(Series *series, int index, const QPointF &dataPos);
    void dataPointClicked(Series *series, int index, const QPointF &dataPos);
    void rangeChanged(double xMin, double xMax, double yMin, double yMax);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QSize minimumSizeHint() const override { return QSize(300, 200); }

private:
    // rendering pipeline
    void renderChart(QPainter &painter, const QSize &size);
    void renderBackground(QPainter &p, const QRectF &area);
    void renderTitle(QPainter &p, const QRectF &rect);
    void renderLegend(QPainter &p, const QRectF &rect);
    void renderGrid(QPainter &p, const QRectF &plotArea);
    void renderAxes(QPainter &p, const QRectF &plotArea);
    void renderLineSeries(QPainter &p, const QRectF &plotArea, LineSeries *series);
    void renderBarSeries(QPainter &p, const QRectF &plotArea);
    void renderStackedBarSeries(QPainter &p, const QRectF &plotArea);
    void renderRangeBarSeries(QPainter &p, const QRectF &plotArea);

    // pixel generation helper
    QVector<QPointF> generateLineSeriesPixels(const QRectF &plotArea,
                                               const QVector<LineSeries::DataPoint> &data,
                                               int dataSize) const;

    // mouse helpers
    int hitTestDataPoint(const QPointF &widgetPos, Series **outSeries) const;

    // members
    ChartModel  *m_model      = nullptr;
    ChartLayout *m_layout     = nullptr;
    Axis        *m_xAxis      = nullptr;
    Axis        *m_yAxis      = nullptr;
    PaintBuffer *m_buffer     = nullptr;
    Legend       m_legend;
    QFont        m_titleFont  = QFont("Arial", 13, QFont::Bold);
    bool         m_dirty      = true;

    // interaction
    bool   m_dragging = false;
    QPointF m_lastMousePos;
    QPointF m_mousePressPos;
    double  m_dragStartXMin = 0, m_dragStartXMax = 0;
    double  m_dragStartYMin = 0, m_dragStartYMax = 0;
    int    m_hoveredSeriesIdx = -1;
    int    m_hoveredPointIdx = -1;

    friend class ChartModel;
};

#endif // CHARTWIDGET_H
