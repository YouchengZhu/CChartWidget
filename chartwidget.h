#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QMenu>

/* ═══════════════════════════════════════════════════════════════
 *  Axis 类族
 * ═══════════════════════════════════════════════════════════════ */

class Axis : public QObject {
    Q_OBJECT
public:
    enum Position { Bottom, Left };
    explicit Axis(Position pos, QObject *parent = nullptr);
    virtual ~Axis();
    Position position() const;
    void setRange(double min, double max);
    void setAutoRange(bool on);
    bool isAutoRange() const;
    double min() const;
    double max() const;
    void setTitle(const QString &t);
    QString title() const;
    void setTickCount(int n);
    int  tickCount() const;
    void setLabelFont(const QFont &f);
    QFont labelFont() const;
    void setTitleFont(const QFont &f);
    QFont titleFont() const;
    void setGridVisible(bool on);
    bool isGridVisible() const;
    void setGridColor(const QColor &c);
    QColor gridColor() const;
    virtual QString formatValue(double v) const = 0;
    virtual QVector<double> computeTicks() const = 0;
protected:
    Position m_position;
    bool     m_autoRange  = true;
    double   m_min = 0.0, m_max = 100.0;
    QString  m_title;
    int      m_tickCount  = 6;
    QFont    m_labelFont, m_titleFont;
    bool     m_gridVisible = true;
    QColor   m_gridColor;
    static double niceStep(double range, int tickCount);
    static int decimalsForStep(double step);
};

class NumericAxis : public Axis {
    Q_OBJECT
public:
    enum Notation { Decimal, Scientific };
    explicit NumericAxis(Position pos, QObject *parent = nullptr);
    void setNotation(Notation n); Notation notation() const;
    void setFormatPrecision(int p); int formatPrecision() const;
    QString formatValue(double v) const override;
    QVector<double> computeTicks() const override;
private:
    Notation m_notation = Decimal; int m_precision = -1;
};

class DateTimeAxis : public Axis {
    Q_OBJECT
public:
    enum Format { HHmmss, HHmm, HHmmsszzz };
    explicit DateTimeAxis(Position pos, QObject *parent = nullptr);
    void setFormat(Format f); Format format() const;
    void setRange(const QDateTime &lo, const QDateTime &hi);
    QDateTime minDateTime() const; QDateTime maxDateTime() const;
    QString formatValue(double v) const override;
    QVector<double> computeTicks() const override;
private:
    Format m_fmt = HHmmss;
    double dt2d(const QDateTime &dt) const; QDateTime d2dt(double v) const;
};

class DateAxis : public Axis {
    Q_OBJECT
public:
    enum Format { yyyyMMdd, yyyy_MM_dd, MMdd, yyyyMM };
    explicit DateAxis(Position pos, QObject *parent = nullptr);
    void setFormat(Format f); Format format() const;
    void setRange(const QDateTime &lo, const QDateTime &hi);
    QDateTime minDateTime() const; QDateTime maxDateTime() const;
    QString formatValue(double v) const override;
    QVector<double> computeTicks() const override;
private:
    Format m_fmt = yyyyMMdd;
    double dt2d(const QDateTime &dt) const; QDateTime d2dt(double v) const;
};

/* ═══════════════════════════════════════════════════════════════
 *  Series 类族
 * ═══════════════════════════════════════════════════════════════ */

class Series : public QObject {
    Q_OBJECT
public:
    enum Type { Line, Bar, StackedBar };
    explicit Series(const QString &name, Type type, QObject *parent = nullptr);
    virtual ~Series();
    QString name() const; Type type() const;
    void setVisible(bool on); bool isVisible() const;
    void setColor(const QColor &c); QColor color() const;
    virtual int dataCount() const = 0;
protected:
    QString m_name; Type m_type; bool m_visible = true; QColor m_color;
};

struct DataPoint {
    double x, y;
    DataPoint() : x(0), y(0) {}
    DataPoint(double xv, double yv) : x(xv), y(yv) {}
    DataPoint(const QDateTime &t, double yv)
        : x(static_cast<double>(t.toSecsSinceEpoch())), y(yv) {}
};

/* ── 散点样式 ────────────────────────────────────────────────── */
enum ScatterStyle {
    ScatterNone,        // 无标记
    ScatterCircle,      // 圆形（默认）
    ScatterSquare,      // 正方形
    ScatterDiamond,     // 菱形
    ScatterTriangle,    // 三角形（朝上）
    ScatterCross,       // 十字
    ScatterPlus,        // 加号
    ScatterStar,        // 五角星
    ScatterDot          // 小圆点（无内圈）
};

class LineSeries : public Series {
    Q_OBJECT
public:
    explicit LineSeries(const QString &name, QObject *parent = nullptr);
    void append(double x, double y);
    void append(const QDateTime &timeX, double y);
    void append(const DataPoint &p);
    void append(const QVector<DataPoint> &pts);
    void removeAt(int index);
    void clear();
    const QVector<DataPoint>& data() const;
    int dataCount() const override;
    void setLineWidth(double w); double lineWidth() const;
    void setMarkerSize(double s); double markerSize() const;

    /* 散点样式 */
    void setScatterStyle(ScatterStyle style);
    ScatterStyle scatterStyle() const;

private:
    QVector<DataPoint> m_data;
    double m_lineWidth    = 2.0;
    double m_markerSize   = 5.0;
    ScatterStyle m_scatterStyle = ScatterCircle;
};

class BarSeries : public Series {
    Q_OBJECT
public:
    explicit BarSeries(const QString &name, QObject *parent = nullptr);
    void append(double value);
    void removeAt(int index);
    void clear();
    const QVector<double>& data() const;
    int dataCount() const override;
    void appendXY(double x, double y);
    struct XY { double x, y; };
    const QVector<XY>& xyData() const;
    bool useXY() const;
    void setBarWidthRatio(double r); double barWidthRatio() const;
private:
    QVector<double> m_data; QVector<XY> m_xyData;
    bool m_useXY = false; double m_barWidthRatio = 0.7;
};

class StackedBarSeries : public Series {
    Q_OBJECT
public:
    explicit StackedBarSeries(const QString &name, QObject *parent = nullptr);
    void append(double value);
    void removeAt(int index);
    void clear();
    const QVector<double>& data() const;
    int dataCount() const override;
    void setBarWidthRatio(double r); double barWidthRatio() const;
private:
    QVector<double> m_data; double m_barWidthRatio = 0.7;
};

/* ═══════════════════════════════════════════════════════════════
 *  ChartWidget
 * ═══════════════════════════════════════════════════════════════ */

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    enum RescaleMode { AutoFit, FitVisible, Manual };

    /* ── 图例位置 & 布局 ──────────────────────────────────── */
    enum LegendPosition {
        LegendTopRight,
        LegendTop,
        LegendBottom,
        LegendHidden
    };
    enum LegendOrientation {
        LegendHorizontal,   // 水平排列（默认）
        LegendVertical      // 垂直排列
    };

    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    /* 坐标轴 */
    void addAxis(Axis *axis);
    void removeAxis(Axis *axis);
    QList<Axis*> axes() const;
    Axis* axisX() const { return m_axisX; }
    Axis* axisY() const { return m_axisY; }

    /* 数据序列 */
    void addSeries(Series *s);
    void removeSeries(Series *s);
    QList<Series*> seriesList() const;

    /* 分类标签 */
    void setCategories(const QStringList &cats);
    QStringList categories() const;

    /* 外观 */
    void setTitle(const QString &t); QString title() const;
    void setTitleFont(const QFont &f);
    void setBackgroundColor(const QColor &c);
    void setPlotBackgroundColor(const QColor &c);
    void setMargin(int m);

    /* 缩放 */
    void setRescaleMode(RescaleMode mode);
    RescaleMode rescaleMode() const;
    void fitToData();
    void zoom(double factor);
    void zoomTo(double xMin, double xMax, double yMin, double yMax);

    /* 图例 */
    void setLegendPosition(LegendPosition pos);
    LegendPosition legendPosition() const;
    void setLegendOrientation(LegendOrientation ori);
    LegendOrientation legendOrientation() const;
    void setLegendFont(const QFont &f);

    /* Tooltip */
    void setTooltipEnabled(bool on);
    bool isTooltipEnabled() const;
    void setTooltipFont(const QFont &f);

    void refresh();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    /* 绘制 */
    void drawBackground(QPainter &p);
    void drawPlotBackground(QPainter &p);
    void drawGrid(QPainter &p);
    void drawSeries(QPainter &p);
    void drawAxes(QPainter &p);
    void drawTitle(QPainter &p);
    void drawLegend(QPainter &p);
    void drawTooltip(QPainter &p);

    void drawLine(QPainter &p, LineSeries *s);
    void drawBar(QPainter &p, BarSeries *s);
    void drawStackedBar(QPainter &p);

    /* 散点绘制辅助 */
    void drawScatter(QPainter &p, ScatterStyle style,
                     const QPointF &center, double size, const QColor &color);

    /* 坐标映射 */
    QRectF  plotRect() const;
    QPointF mapToPixel(double dataX, double dataY) const;
    double  xToDouble(double raw) const;

    void updateLayout();

    /* 范围 */
    void computeXRange();
    void computeYRange();
    void syncRangeFromAxes();

    /* 鼠标 */
    void findNearest(QPoint mousePos);
    void updateTooltipForBar(int catIdx, int barIdx);
    void updateTooltipForStackedBar(int catIdx);
    void updateTooltipForLine(LineSeries *ls, int ptIdx);
    double pixelDist(const QPointF &a, const QPointF &b) const;

    /* ── 数据 ────────────────────────────────────────────── */
    Axis          *m_axisX = nullptr;
    Axis          *m_axisY = nullptr;
    QList<Series*> m_series;
    QStringList    m_categories;

    /* ── 外观 ────────────────────────────────────────────── */
    QString m_title;
    QFont   m_titleFont;
    QColor  m_bgColor;
    QColor  m_plotBgColor;
    int     m_margin = 12;

    /* ── 布局 ────────────────────────────────────────────── */
    QRectF  m_plotArea;
    double  m_xMin = 0, m_xMax = 100;
    double  m_yMin = 0, m_yMax = 100;
    bool    m_layoutDirty = true;

    /* ── 缩放 ────────────────────────────────────────────── */
    RescaleMode m_rescaleMode = AutoFit;

    /* ── 鼠标交互 ────────────────────────────────────────── */
    bool    m_panning = false;
    QPoint  m_panStart;
    double  m_panXMin = 0, m_panXMax = 0;
    double  m_panYMin = 0, m_panYMax = 0;

    /* ── Tooltip ─────────────────────────────────────────── */
    bool    m_tooltipEnabled = true;
    QFont   m_tooltipFont;
    bool    m_showTooltip    = false;
    QPointF m_tooltipPos;
    QString m_tooltipText;
    QColor  m_tooltipColor;     // ← tooltip 主色调（数据点所在序列的颜色）
    QPoint m_mouseScreen;

    /* ── 图例 ────────────────────────────────────────────── */
    LegendPosition    m_legendPosition    = LegendTopRight;
    LegendOrientation m_legendOrientation = LegendHorizontal;
    QFont             m_legendFont;
};

#endif // CHARTWIDGET_H
