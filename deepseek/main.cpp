#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QDateTime>
#include <QDate>
#include <QVector>
#include <cstdlib>
#include <ctime>
#include <QLabel>

#include "chartwidget.h"

// ---- helper ----
static double rng(int lo, int hi) { return lo + qrand() % (hi - lo + 1); }

// ============================================================================
// 1. LineGraph — 折线图 + 散点 + 填充 + 平滑
// ============================================================================
static AxisRect *createLineDemo()
{
    auto *ar = new AxisRect;

    auto *xAxis = new NumericAxis(BaseAxis::Horizontal);
    xAxis->setRange(-4, 4);
    xAxis->setTickCount(9);

    auto *yAxis = new NumericAxis(BaseAxis::Vertical);
    yAxis->setRange(-1.2, 1.2);
    yAxis->setTickCount(7);

    ar->setXAxis(xAxis);
    ar->setYAxis(yAxis);

    auto *line = new LineGraph<double, double>;
    line->setName("sin(x) / x");
    line->setLayer(0);
    line->setLineColor(QColor(50, 100, 220));
    line->setLineWidth(2.5);
    line->setSmooth(true);
    line->setFillBelow(true);
    line->setFillColor(QColor(50, 100, 220, 40));

    ScatterFormat sf;
    sf.shape = ScatterShape::Circle;
    sf.size = 7;
    sf.color = QColor(50, 100, 220);
    sf.fillColor = Qt::white;
    sf.borderWidth = 2.0;
    line->setScatterFormat(sf);

    for (double x = -3.9; x <= 3.9; x += 0.15) {
        double y = (qFuzzyIsNull(x) ? 1.0 : qSin(x * 3.0) / (x * 3.0));
        line->addPoint(x, y + (qrand() % 10 - 5) * 0.02);
    }

    ar->addGraph(line);
    return ar;
}

// ============================================================================
// 2. BarGraph — 柱状图 (数字 X / 数字 Y)
// ============================================================================
static AxisRect *createBarDemo()
{
    auto *ar = new AxisRect;

    auto *xAxis = new NumericAxis(BaseAxis::Horizontal);
    xAxis->setRange(0, 5);
    xAxis->setTickCount(6);
    xAxis->setDecimalPlaces(0);

    auto *yAxis = new NumericAxis(BaseAxis::Vertical);
    yAxis->setRange(0, 120);
    yAxis->setTickCount(7);

    ar->setXAxis(xAxis);
    ar->setYAxis(yAxis);

    auto *bar = new BarGraph<double, double>;
    bar->setName("Revenue");
    bar->setLayer(0);
    bar->setBarColor(QColor(70, 130, 180));
    bar->setBorderColor(QColor(30, 80, 130));
    bar->setBorderWidth(1.5);
    bar->setBarWidthFactor(0.6);

    const QStringList labels = {"Q1", "Q2", "Q3", "Q4", "Q5"};
    for (int i = 0; i < labels.size(); ++i)
        bar->addPoint(static_cast<double>(i), rng(30, 100));

    ar->addGraph(bar);
    return ar;
}

// ============================================================================
// 3. StackedBarGraph — 堆叠柱状图 (日期 X / 数字 Y)
// ============================================================================
static AxisRect *createStackedBarDemo()
{
    auto *ar = new AxisRect;

    QDate today = QDate::currentDate();
    QDate start = today.addDays(-9);

    auto *xAxis = new DateAxis(BaseAxis::Horizontal);
    xAxis->setRange(start, today);
    xAxis->setTickCount(6);
    xAxis->setLabelFormat("MM-dd");

    auto *yAxis = new NumericAxis(BaseAxis::Vertical);
    yAxis->setRange(0, 160);
    yAxis->setTickCount(6);

    ar->setXAxis(xAxis);
    ar->setYAxis(yAxis);

    auto *stacked = new StackedBarGraph<QDate, double>;
    stacked->setName("Traffic");
    stacked->setLayer(0);
    stacked->setSeriesCount(3);
    stacked->setSeriesColor(0, QColor(65, 105, 225));
    stacked->setSeriesColor(1, QColor(50, 205, 50));
    stacked->setSeriesColor(2, QColor(255, 165, 0));
    stacked->setSeriesName(0, "Organic");
    stacked->setSeriesName(1, "Referral");
    stacked->setSeriesName(2, "Direct");
    stacked->setBarWidthFactor(0.7);

    using DP = ChartDataPoint<QDate, double>;
    QVector<DP> s0, s1, s2;
    for (int i = 0; i < 10; ++i) {
        QDate d = start.addDays(i);
        s0.append({d, rng(30, 60)});
        s1.append({d, rng(15, 30)});
        s2.append({d, rng(5, 20)});
    }
    stacked->setSeriesData(0, s0);
    stacked->setSeriesData(1, s1);
    stacked->setSeriesData(2, s2);

    ar->addGraph(stacked);
    return ar;
}

// ============================================================================
// 4. RangeBarGraph — 范围柱状图 (数字 X / 数字 Y，自定义下限)
// ============================================================================
static AxisRect *createRangeBarDemo()
{
    auto *ar = new AxisRect;

    auto *xAxis = new NumericAxis(BaseAxis::Horizontal);
    xAxis->setRange(-0.5, 6.5);
    xAxis->setTickCount(8);
    xAxis->setDecimalPlaces(0);

    auto *yAxis = new NumericAxis(BaseAxis::Vertical);
    yAxis->setRange(0, 100);
    yAxis->setTickCount(6);

    ar->setXAxis(xAxis);
    ar->setYAxis(yAxis);

    auto *rangeBar = new RangeBarGraph<double, double>;
    rangeBar->setName("Temperature Range");
    rangeBar->setLayer(0);
    rangeBar->setBarColor(QColor(220, 120, 80));
    rangeBar->setBorderColor(QColor(160, 70, 40));
    rangeBar->setBorderWidth(1.5);
    rangeBar->setBarWidthFactor(0.55);

    for (int i = 0; i < 7; ++i) {
        double lo = rng(15, 25);
        double hi = lo + rng(20, 40);
        rangeBar->addPoint(static_cast<double>(i), lo, hi);
    }

    ar->addGraph(rangeBar);
    return ar;
}

// ============================================================================
// 5. 大数量折线图 — 展示自适应采样
// ============================================================================
static AxisRect *createLargeDataDemo()
{
    auto *ar = new AxisRect;

    auto *xAxis = new NumericAxis(BaseAxis::Horizontal);
    xAxis->setRange(0, 100);
    xAxis->setTickCount(11);

    auto *yAxis = new NumericAxis(BaseAxis::Vertical);
    yAxis->setRange(-3, 3);
    yAxis->setTickCount(7);

    ar->setXAxis(xAxis);
    ar->setYAxis(yAxis);

    // Composite waveform with 10,000 points
    auto *line = new LineGraph<double, double>;
    line->setName("Waveform (10k pts)");
    line->setLayer(0);
    line->setLineColor(QColor(60, 140, 100));
    line->setLineWidth(1.5);

    for (int i = 0; i <= 10000; ++i) {
        double x = i * 0.01;
        double y = qSin(x * 0.5) + 0.5 * qSin(x * 3.7)
                   + 0.25 * qSin(x * 13.3) + 0.125 * qSin(x * 37.0);
        line->addPoint(x, y);
    }

    ar->addGraph(line);
    return ar;
}

// ============================================================================
// 6. 混合图表 — LineGraph + RangeBarGraph 同框
// ============================================================================
static AxisRect *createComboDemo()
{
    auto *ar = new AxisRect;

    QDate today = QDate::currentDate();

    auto *xAxis = new DateAxis(BaseAxis::Horizontal);
    xAxis->setRange(today.addDays(-7), today);
    xAxis->setTickCount(8);
    xAxis->setLabelFormat("MM-dd");

    auto *yAxis = new NumericAxis(BaseAxis::Vertical);
    yAxis->setRange(-10, 50);
    yAxis->setTickCount(7);

    ar->setXAxis(xAxis);
    ar->setYAxis(yAxis);

    // Range bar (layer 0 — background)
    auto *rangeBar = new RangeBarGraph<QDate, double>;
    rangeBar->setName("Low-High");
    rangeBar->setLayer(0);
    rangeBar->setBarColor(QColor(200, 200, 220));
    rangeBar->setBorderColor(QColor(140, 140, 160));
    rangeBar->setBorderWidth(1.0);
    rangeBar->setBarWidthFactor(0.45);

    for (int i = 0; i < 8; ++i) {
        QDate d = today.addDays(i - 7);
        double lo = rng(-5, 5);
        double hi = lo + rng(20, 30);
        rangeBar->addPoint(d, lo, hi);
    }

    // Line (layer 1 — foreground)
    auto *line = new LineGraph<QDate, double>;
    line->setName("Avg Temp");
    line->setLayer(1);
    line->setLineColor(QColor(200, 40, 40));
    line->setLineWidth(2.5);

    ScatterFormat sf;
    sf.shape = ScatterShape::Circle;
    sf.size = 9;
    sf.color = QColor(200, 40, 40);
    sf.fillColor = Qt::white;
    sf.borderWidth = 2.5;
    line->setScatterFormat(sf);

    for (int i = 0; i < 8; ++i) {
        QDate d = today.addDays(i - 7);
        line->addPoint(d, rng(10, 25));
    }

    ar->addGraph(rangeBar);
    ar->addGraph(line);
    return ar;
}

// ============================================================================
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));

    QMainWindow window;
    window.setWindowTitle("Custom Chart Library — All Graph Types Demo");

    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(6);

    // Label row
    auto *labelRow = new QHBoxLayout;
    const QStringList titles = {
        "LineGraph\nsin(x)/x + scatter + fill + smooth",
        "BarGraph\ncolumn chart",
        "StackedBarGraph\ndate X · 3-series stacked",
        "RangeBarGraph\ncustom min–max range",
        "Large Data (10k pts)\nadaptive sampling ON",
        "Combo: RangeBar + Line\ndate X · layered"
    };
    for (const auto &t : titles) {
        auto *lbl = new QLabel(t);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("font-size:11px; color:#444; padding:2px;");
        labelRow->addWidget(lbl);
    }

    auto *chartWidget = new ChartWidget;
    auto *table = new ChartTable(2, 3);   // 2 rows × 3 cols

    table->addItem(createLineDemo(),        0, 0);
    table->addItem(createBarDemo(),         0, 1);
    table->addItem(createStackedBarDemo(),  0, 2);
    table->addItem(createRangeBarDemo(),    1, 0);
    table->addItem(createLargeDataDemo(),   1, 1);
    table->addItem(createComboDemo(),       1, 2);

    chartWidget->setChartTable(table);
    chartWidget->invalidateBuffer();

    mainLayout->addLayout(labelRow);
    mainLayout->addWidget(chartWidget, 1);

    window.setCentralWidget(central);
    window.resize(1500, 850);
    window.show();

    return app.exec();
}
