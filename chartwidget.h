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
#include <QPixmap>
#include <QMenu>

class Axis : public QObject {
    Q_OBJECT
public:
    enum Position { Bottom, Left };
    enum SubTickDirection { SubTickInside, SubTickOutside };

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

    void setSubTickCount(int n);
    int  subTickCount() const;
    void setSubTicksVisible(bool on);
    bool isSubTicksVisible() const;
    void setSubTickColor(const QColor &c);
    QColor subTickColor() const;
    void setSubTickDirection(SubTickDirection dir);
    SubTickDirection subTickDirection() const;

    void setGridVisible(bool on);
    bool isGridVisible() const;
    void setHorizontalGridVisible(bool on);
    bool isHorizontalGridVisible() const;
    void setVerticalGridVisible(bool on);
    bool isVerticalGridVisible() const;
    void setGridColor(const QColor &c);
    QColor gridColor() const;

    virtual QString formatValue(double v) const = 0;
    virtual QVector<double> computeTicks() const = 0;

protected:
    Position m_position;
    bool     m_autoRange  = true;
    double   m_min = 0.0, m_max = 100.0;
    QString  m_title;
    QFont    m_labelFont, m_titleFont;
    int      m_tickCount    = 6;
    bool     m_ticksVisible = true;
    QColor   m_tickColor;
    int      m_subTickCount    = 4;
    bool     m_subTicksVisible = true;
    QColor   m_subTickColor;
    SubTickDirection m_subTickDirection = SubTickInside;
    bool     m_gridVisible          = true;
    bool     m_horizontalGridVisible = true;
    bool     m_verticalGridVisible   = true;
    QColor   m_gridColor;

    static double niceStep(double range, int tickCount);
    static int decimalsForStep(double step);
};

class NumericAxis : public Axis {
    Q_OBJECT
public:
    enum Notation { Decimal, Scientific };
    explicit NumericAxis(Position pos, QObject *parent = nullptr);
    void setNotation(Notation n);
    Notation notation() const;
    void setFormatPrecision(int p);
    int  formatPrecision() const;
    QString formatValue(double v) const override;
    QVector<double> computeTicks() const override;
private:
    Notation m_notation = Decimal;
    int      m_precision = -1;
};

class DateTimeAxis : public Axis {
    Q_OBJECT
public:
    enum Format { HHmmss, HHmm, HHmmsszzz };
    explicit DateTimeAxis(Position pos, QObject *parent = nullptr);
    void setFormat(Format f);
    Format format() const;
    void setRange(const QDateTime &lo, const QDateTime &hi);
    QDateTime minDateTime() const;
    QDateTime maxDateTime() const;
    QString formatValue(double v) const override;
    QVector<double> computeTicks() const override;
private:
    Format m_fmt = HHmmss;
    double dt2d(const QDateTime &dt) const;
    QDateTime d2dt(double v) const;
};

class DateAxis : public Axis {
    Q_OBJECT
public:
    enum Format { yyyyMMdd, yyyy_MM_dd, MMdd, yyyyMM };
    explicit DateAxis(Position pos, QObject *parent = nullptr);
    void setFormat(Format f);
    Format format() const;
    void setRange(const QDateTime &lo, const QDateTime &hi);
    QDateTime minDateTime() const;
    QDateTime maxDateTime() const;
    QString formatValue(double v) const override;
    QVector<double> computeTicks() const override;
private:
    Format m_fmt = yyyyMMdd;
    double dt2d(const QDateTime &dt) const;
    QDateTime d2dt(double v) const;
};

class Series : public QObject {
    Q_OBJECT
public:
    enum Type { Line, Bar, StackedBar };
    explicit Series(const QString &name, Type type, QObject *parent = nullptr);
    virtual ~Series();
    QString name() const;
    Type    type() const;
    void setVisible(bool on);
    bool isVisible() const;
    void setColor(const QColor &c);
    QColor color() const;
    virtual int dataCount() const = 0;
protected:
    QString m_name;
    Type    m_type;
    bool    m_visible = true;
    QColor  m_color;
};

struct DataPoint {
    double x, y;
    DataPoint() : x(0), y(0) {}
    DataPoint(double xv, double yv) : x(xv), y(yv) {}
    DataPoint(const QDateTime &t, double yv)
        : x(static_cast<double>(t.toSecsSinceEpoch())), y(yv) {}
};

enum ScatterStyle {
    ScatterNone, ScatterCircle, ScatterSquare, ScatterDiamond,
    ScatterTriangle, ScatterCross, ScatterPlus, ScatterStar, ScatterDot
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
    void setLineWidth(double w);
    double lineWidth() const;
    void setMarkerSize(double s);
    double markerSize() const;
    void setScatterStyle(ScatterStyle style);
    ScatterStyle scatterStyle() const;
private:
    QVector<DataPoint> m_data;
    double m_lineWidth = 2.0, m_markerSize = 5.0;
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
    void setBarWidthRatio(double r);
    double barWidthRatio() const;
private:
    QVector<double> m_data;
    QVector<XY>     m_xyData;
    bool            m_useXY = false;
    double          m_barWidthRatio = 0.7;
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
    void setBarWidthRatio(double r);
    double barWidthRatio() const;
private:
    QVector<double> m_data;
    double m_barWidthRatio = 0.7;
};

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    enum RescaleMode { AutoFit, FitVisible, Manual };
    enum LegendPosition { LegendTopRight, LegendTop, LegendBottom, LegendHidden };
    enum LegendOrientation { LegendHorizontal, LegendVertical };

    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    void addAxis(Axis *axis);
    void removeAxis(Axis *axis);
    QList<Axis*> axes() const;
    Axis* axisX() const { return m_axisX; }
    Axis* axisY() const { return m_axisY; }

    void addSeries(Series *s);
    void removeSeries(Series *s);
    QList<Series*> seriesList() const;

    void setCategories(const QStringList &cats);
    QStringList categories() const;

    void setTitle(const QString &t);
    QString title() const;
    void setTitleFont(const QFont &f);
    void setBackgroundColor(const QColor &c);
    void setPlotBackgroundColor(const QColor &c);
    void setMargin(int m);

    void setRescaleMode(RescaleMode mode);
    RescaleMode rescaleMode() const;
    void fitToData();
    void zoom(double factor);
    void zoomTo(double xMin, double xMax, double yMin, double yMax);

    void setLegendPosition(LegendPosition pos);
    LegendPosition legendPosition() const;
    void setLegendOrientation(LegendOrientation ori);
    LegendOrientation legendOrientation() const;
    void setLegendFont(const QFont &f);

    void setTooltipEnabled(bool on);
    bool isTooltipEnabled() const;
    void setTooltipFont(const QFont &f);

    QPixmap exportToPixmap(const QSize &size = QSize()) const;
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
    void renderChart(QPainter &p, const QRect &targetRect);
    void drawPlotBackground(QPainter &p);
    void drawGrid(QPainter &p);
    void drawSeries(QPainter &p);
    void drawAxes(QPainter &p);
    void drawBackground(QPainter &p);
    void drawTitle(QPainter &p);
    void drawLegend(QPainter &p);
    void drawTooltip(QPainter &p);
    void drawLine(QPainter &p, LineSeries *s);
    void drawBar(QPainter &p, BarSeries *s);
    void drawStackedBar(QPainter &p);
    void drawScatter(QPainter &p, ScatterStyle style,
                     const QPointF &center, double size, const QColor &color);

    QRectF  plotRect() const;
    QPointF mapToPixel(double dataX, double dataY) const;
    double  xToDouble(double raw) const;
    void computeXRange();
    void computeYRange();
    void syncRangeFromAxes();
    void updateLayout();

    void findNearest(QPoint mousePos);
    void updateTooltipForBar(int catIdx, int barIdx);
    void updateTooltipForStackedBar(int catIdx);
    void updateTooltipForLine(LineSeries *ls, int ptIdx);
    double pixelDist(const QPointF &a, const QPointF &b) const;

    Axis          *m_axisX = nullptr;
    Axis          *m_axisY = nullptr;
    QList<Series*> m_series;
    QStringList    m_categories;

    QString m_title;
    QFont   m_titleFont;
    QColor  m_bgColor;
    QColor  m_plotBgColor;
    int     m_margin = 12;

    QRectF  m_plotArea;
    double  m_xMin = 0, m_xMax = 100;
    double  m_yMin = 0, m_yMax = 100;
    bool    m_layoutDirty = true;

    RescaleMode m_rescaleMode = AutoFit;

    bool    m_panning = false;
    QPoint  m_panStart;
    double  m_panXMin = 0, m_panXMax = 0;
    double  m_panYMin = 0, m_panYMax = 0;

    bool    m_tooltipEnabled = true;
    QFont   m_tooltipFont;
    bool    m_showTooltip = false;
    QPointF m_tooltipPos;
    QString m_tooltipText;
    QColor  m_tooltipColor;
    QPoint  m_mouseScreen;

    LegendPosition    m_legendPosition    = LegendTopRight;
    LegendOrientation m_legendOrientation = LegendHorizontal;
    QFont             m_legendFont;
};

#endif
