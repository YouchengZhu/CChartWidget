#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QObject>
#include <QVector>
#include <QList>
#include <QStringList>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QBrush>
#include <QPixmap>
#include <QRectF>
#include <QPointF>

// ========================================================================
// 枚举定义
// ========================================================================

enum class AxisPosition  { Bottom, Left };
enum class AxisType      { Numeric, DateTime, Date };
enum class NumericNotation { Decimal, Scientific };
enum class DateTimeFormat  { HHmmss, HHmm, HHmmsszzz };
enum class DateFormat      { yyyyMMdd, yyyy_MM_dd, MMdd, yyyyMM };
enum class TickDirection    { Inside, Outside };
enum class SeriesType      { Line, Bar, StackedBar };
enum class ScatterStyle {
    None, Circle, Square, Diamond,
    Triangle, Cross, Plus, Star, Dot
};

// ========================================================================
// DataPoint
// ========================================================================

struct DataPoint {
    double x = 0, y = 0;
    DataPoint() = default;
    DataPoint(double xv, double yv) : x(xv), y(yv) {}
    DataPoint(const QDateTime &t, double yv)
        : x(static_cast<double>(t.toSecsSinceEpoch())), y(yv) {}
};

// ========================================================================
// ChartTheme
// ========================================================================

struct ChartTheme {
    QColor background       = QColor(252, 252, 252);
    QColor plotBackground   = QColor(255, 255, 255);
    QColor textColor        = QColor(60, 60, 60);
    QColor titleColor       = QColor(40, 40, 40);
    QColor axisTitleColor   = QColor(50, 50, 50);
    QColor tickColor        = QColor(120, 120, 120);
    QColor subTickColor     = QColor(200, 200, 200);
    QColor gridColor        = QColor(210, 210, 210);
    Qt::PenStyle gridStyle  = Qt::DashLine;
    int     gridWidth       = 1;

    QFont titleFont         = QFont("Microsoft YaHei", 13, QFont::Bold);
    QFont labelFont         = QFont("Microsoft YaHei", 9);
    QFont axisTitleFont     = QFont("Microsoft YaHei", 10, QFont::Bold);
    QFont legendFont        = QFont("Microsoft YaHei", 9);
    QFont tooltipFont       = QFont("Microsoft YaHei", 9);

    QList<QColor> seriesColors = {
        QColor(255, 77, 79),  QColor(54, 162, 235), QColor(255, 206, 86),
        QColor(75, 192, 192), QColor(153, 102, 255), QColor(255, 159, 64),
        QColor(46, 204, 113), QColor(231, 76, 60),  QColor(52, 152, 219),
        QColor(241, 196, 15), QColor(26, 188, 156),  QColor(155, 89, 182),
    };

    QColor legendBorder       = QColor(180, 180, 180);
    QColor legendBackground   = QColor(255, 255, 255, 235);
    int    legendRadius       = 4;

    QColor tooltipBorder      = QColor(160, 160, 160);
    QColor tooltipBackground  = QColor(255, 255, 255, 245);
    QColor tooltipShadow      = QColor(0, 0, 0, 40);
    int    tooltipRadius      = 6;

    static ChartTheme light() { return ChartTheme{}; }

    static ChartTheme dark() {
        ChartTheme t;
        t.background        = QColor(30, 30, 30);
        t.plotBackground    = QColor(40, 40, 40);
        t.textColor         = QColor(200, 200, 200);
        t.titleColor        = QColor(230, 230, 230);
        t.axisTitleColor    = QColor(210, 210, 210);
        t.tickColor         = QColor(140, 140, 140);
        t.subTickColor      = QColor(80, 80, 80);
        t.gridColor         = QColor(70, 70, 70);
        t.legendBorder      = QColor(80, 80, 80);
        t.legendBackground  = QColor(50, 50, 50, 220);
        t.tooltipBorder     = QColor(90, 90, 90);
        t.tooltipBackground = QColor(55, 55, 55, 240);
        t.tooltipShadow     = QColor(0, 0, 0, 80);
        return t;
    }

    static ChartTheme scientific() {
        ChartTheme t;
        t.background      = QColor(245, 245, 240);
        t.gridColor       = QColor(200, 200, 200);
        t.gridStyle       = Qt::SolidLine;
        t.seriesColors = {
            QColor(0,0,0), QColor(200,50,50), QColor(50,100,200),
            QColor(0,150,100), QColor(180,100,30), QColor(120,50,150),
        };
        return t;
    }
};

// ========================================================================
// Axis：纯数据容器
// ========================================================================

class Axis {
public:
    explicit Axis(AxisPosition pos);
    ~Axis();

    AxisPosition position() const;
    AxisType     type() const;
    void setType(AxisType t);

    void setRange(double min, double max);
    void setAutoRange(bool on);
    bool isAutoRange() const;
    double min() const;
    double max() const;

    void setTitle(const QString &t);
    QString title() const;
    void setLabelFont(const QFont &f);
    QFont labelFont() const;
    void setTitleFont(const QFont &f);
    QFont titleFont() const;

    void setTickCount(int n);
    int  tickCount() const;
    void setTicksVisible(bool on);
    bool isTicksVisible() const;
    void setTickColor(const QColor &c);
    QColor tickColor() const;
    void setTickDirection(TickDirection d);
    TickDirection tickDirection() const;

    void setSubTickCount(int n);
    int  subTickCount() const;
    void setSubTicksVisible(bool on);
    bool isSubTicksVisible() const;
    void setSubTickColor(const QColor &c);
    QColor subTickColor() const;
    void setSubTickDirection(TickDirection d);
    TickDirection subTickDirection() const;

    void setGridVisible(bool on);
    bool isGridVisible() const;
    void setHorizontalGridVisible(bool on);
    bool isHorizontalGridVisible() const;
    void setVerticalGridVisible(bool on);
    bool isVerticalGridVisible() const;
    void setGridColor(const QColor &c);
    QColor gridColor() const;

    void setNotation(NumericNotation n);
    NumericNotation notation() const;
    void setFormatPrecision(int p);
    int  formatPrecision() const;

    void setDateTimeFormat(DateTimeFormat f);
    DateTimeFormat dateTimeFormat() const;
    void setDateFormat(DateFormat f);
    DateFormat dateFormat() const;

private:
    AxisPosition  m_position;
    AxisType      m_type = AxisType::Numeric;
    bool   m_autoRange = true;
    double m_min = 0.0, m_max = 100.0;
    QString m_title;
    QFont  m_labelFont, m_titleFont;

    int  m_tickCount = 6;
    bool m_ticksVisible = true;
    QColor        m_tickColor;
    TickDirection m_tickDirection = TickDirection::Outside;

    int  m_subTickCount = 4;
    bool m_subTicksVisible = true;
    QColor        m_subTickColor;
    TickDirection m_subTickDirection = TickDirection::Inside;

    bool   m_gridVisible = true;
    bool   m_horizontalGridVisible = true;
    bool   m_verticalGridVisible = true;
    QColor m_gridColor;

    NumericNotation m_notation = NumericNotation::Decimal;
    int m_precision = -1;

    DateTimeFormat m_dateTimeFmt = DateTimeFormat::HHmmss;
    DateFormat     m_dateFmt     = DateFormat::yyyyMMdd;
};

// ========================================================================
// Series
// ========================================================================

class Series {
public:
    explicit Series(const QString &name, SeriesType type);
    virtual ~Series();
    QString    name() const;
    SeriesType type() const;
    void setVisible(bool on);
    bool isVisible() const;
    void setColor(const QColor &c);
    QColor color() const;
    virtual int dataCount() const = 0;
protected:
    QString    m_name;
    SeriesType m_type;
    bool       m_visible = true;
    QColor     m_color;
};

class LineSeries : public Series {
public:
    explicit LineSeries(const QString &name);
    void append(double x, double y);
    void append(const QDateTime &tx, double y);
    void append(const DataPoint &p);
    void append(const QVector<DataPoint> &pts);
    void removeAt(int i);
    void clear();
    const QVector<DataPoint>& data() const;
    int dataCount() const override;
    void setLineWidth(double w);
    double lineWidth() const;
    void setMarkerSize(double s);
    double markerSize() const;
    void setScatterStyle(ScatterStyle s);
    ScatterStyle scatterStyle() const;
    void setFillBrush(const QBrush &b);
    QBrush fillBrush() const;
    void setFillEnabled(bool on);
    bool isFillEnabled() const;
private:
    QVector<DataPoint> m_data;
    double       m_lineWidth    = 2.0;
    double       m_markerSize   = 5.0;
    ScatterStyle m_scatterStyle = ScatterStyle::Circle;
    QBrush       m_fillBrush;
    bool         m_fillEnabled  = false;
};

class BarSeries : public Series {
public:
    struct XY { double x, y; };
    explicit BarSeries(const QString &name);
    void append(double value);
    void appendXY(double x, double y);
    void removeAt(int i);
    void clear();
    const QVector<double>& data() const;
    const QVector<XY>& xyData() const;
    bool useXY() const;
    int dataCount() const override;
    void setBarWidthRatio(double r);
    double barWidthRatio() const;
private:
    QVector<double> m_data;
    QVector<XY>     m_xyData;
    bool   m_useXY = false;
    double m_barWidthRatio = 0.7;
};

class StackedBarSeries : public Series {
public:
    explicit StackedBarSeries(const QString &name);
    void append(double value);
    void removeAt(int i);
    void clear();
    const QVector<double>& data() const;
    int dataCount() const override;
    void setBarWidthRatio(double r);
    double barWidthRatio() const;
private:
    QVector<double> m_data;
    double m_barWidthRatio = 0.7;
};

// ========================================================================
// ChartModel：数据管理 + 信号
// ========================================================================

class ChartModel : public QObject {
    Q_OBJECT
public:
    explicit ChartModel(QObject *parent = nullptr);
    ~ChartModel();

    void addAxis(Axis *axis);
    void removeAxis(Axis *axis);
    QList<Axis*> axes() const;
    Axis* axisX() const;
    Axis* axisY() const;

    void addSeries(Series *s);
    void removeSeries(Series *s);
    QList<Series*> seriesList() const;

    void setCategories(const QStringList &cats);
    QStringList categories() const;

    void setTitle(const QString &t);
    QString title() const;

    void setTheme(const ChartTheme &theme);
    ChartTheme theme() const;

    QColor nextColor();

signals:
    void axisAdded(Axis *axis);
    void axisRemoved(Axis *axis);
    void seriesAdded(Series *s);
    void seriesRemoved(Series *s);
    void dataChanged();
    void categoriesChanged();
    void titleChanged();
    void themeChanged();

private:
    Axis          *m_axisX = nullptr;
    Axis          *m_axisY = nullptr;
    QList<Series*> m_series;
    QStringList    m_categories;
    QString        m_title;
    ChartTheme     m_theme;
    int            m_colorIndex = 0;
};

// ========================================================================
// ChartLayout：布局计算引擎
// ========================================================================

class ChartLayout : public QObject {
    Q_OBJECT
public:
    explicit ChartLayout(ChartModel *model, QObject *parent = nullptr);

    void recalculate(int widgetWidth, int widgetHeight);
    void setOutsideLegendHeight(double h);
    void invalidate();

    QPointF mapToPixel(double dataX, double dataY) const;
    double  pixelXToData(double px) const;
    double  pixelYToData(double py) const;

    double xMin() const;
    double xMax() const;
    double yMin() const;
    double yMax() const;
    QRectF plotArea() const;

    QVector<double> xTicks() const;
    QVector<double> yTicks() const;

    QString formatAxisValue(Axis *axis, double value) const;

    static double niceStep(double range, int tickCount);
    static int    decimalsForStep(double step);

private:
    void computeXRange();
    void computeYRange();
    void computeLayout(int widgetWidth, int widgetHeight);
    void computeTicks();

    QVector<double> computeNumericTicks(Axis *axis) const;
    QVector<double> computeDateTimeTicks(Axis *axis) const;
    QVector<double> computeDateTicks(Axis *axis) const;

    QString formatNumericValue(Axis *axis, double v) const;
    QString formatDateTimeValue(double v, DateTimeFormat fmt) const;
    QString formatDateValue(double v, DateFormat fmt) const;

    ChartModel *m_model;
    QRectF m_plotArea;
    double m_xMin = 0, m_xMax = 100;
    double m_yMin = 0, m_yMax = 100;
    QVector<double> m_xTicks, m_yTicks;
    int  m_margin = 12;
    bool m_dirty  = true;
    double m_outsideLegendHeight = 0;
};

// ========================================================================
// ChartRenderer：纯绘制层
// ========================================================================

class ChartRenderer {
public:
    struct TooltipData {
        bool    visible  = false;
        QPointF position;
        QString text;
        QColor  color;
    };

    ChartRenderer(ChartModel *model, ChartLayout *layout);

    void render(QPainter &p, int width, int height);
    void drawBackground(QPainter &p, int width, int height);
    void drawPlotBackground(QPainter &p);
    void drawGrid(QPainter &p);
    void drawSeries(QPainter &p);
    void drawAxes(QPainter &p);
    void drawTitle(QPainter &p);
    void drawLegend(QPainter &p, const QRectF &plotArea,
                    const QList<Series*> &visible, int pos, int ori);
    void drawTooltip(QPainter &p, const TooltipData &tip);

private:
    void drawLine(QPainter &p, LineSeries *s);
    void drawBar(QPainter &p, BarSeries *s);
    void drawStackedBar(QPainter &p);
    void drawScatter(QPainter &p, ScatterStyle style,
                     const QPointF &center, double size, const QColor &color);

    ChartModel  *m_model;
    ChartLayout *m_layout;
};

// ========================================================================
// ChartWidget：交互层
// ========================================================================

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    enum class LegendPosition    { TopRight, Top, Bottom, Hidden, OutsideTop  };
    enum class LegendOrientation { Horizontal, Vertical };
    enum class RescaleMode       { AutoFit, FitVisible, Manual };

    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    ChartModel*  model()  const;
    ChartLayout* layout() const;

    void addAxis(Axis *axis);
    void removeAxis(Axis *axis);
    void addSeries(Series *s);
    void removeSeries(Series *s);
    void setCategories(const QStringList &cats);
    void setTitle(const QString &t);
    void setTheme(const ChartTheme &t);

    void setRescaleMode(RescaleMode mode);
    RescaleMode rescaleMode() const;
    void fitToData();
    void zoom(double factor);
    void zoomTo(double xMin, double xMax, double yMin, double yMax);

    void setLegendPosition(LegendPosition pos);
    LegendPosition legendPosition() const;
    void setLegendOrientation(LegendOrientation ori);
    LegendOrientation legendOrientation() const;

    void setTooltipEnabled(bool on);
    bool isTooltipEnabled() const;

    QPixmap exportToPixmap(const QSize &size = QSize()) const;
    void refresh();

signals:
    void dataPointHovered(Series *series, int index, const QPointF &dataPos);
    void dataPointClicked(Series *series, int index, const QPointF &dataPos);
    void rangeChanged(double xMin, double xMax, double yMin, double yMax);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    void ensureComponents();
    void markDirty();
    void rebuildBuffer();

    void findNearest(QPoint mousePos);
    void buildTooltipForLine(LineSeries *ls, int ptIdx);
    void buildTooltipForBar(int catIdx, int barIdx);
    void buildTooltipForStackedBar(int catIdx);
    double pointDist(const QPointF &a, const QPointF &b) const;

    ChartModel    *m_model    = nullptr;
    ChartLayout   *m_layout   = nullptr;
    ChartRenderer *m_renderer = nullptr;

    QPixmap m_buffer;
    bool    m_bufferDirty = true;

    RescaleMode m_rescaleMode = RescaleMode::AutoFit;

    bool   m_panning = false;
    QPoint m_panStart;
    double m_panXMin = 0, m_panXMax = 0;
    double m_panYMin = 0, m_panYMax = 0;

    bool    m_tooltipEnabled = true;
    bool    m_showTooltip    = false;
    QPointF m_tooltipPos;
    QString m_tooltipText;
    QColor  m_tooltipColor;
    QPoint  m_mouseScreen;

    LegendPosition    m_legendPosition    = LegendPosition::TopRight;
    LegendOrientation m_legendOrientation = LegendOrientation::Horizontal;
};

#endif // CHARTWIDGET_H
