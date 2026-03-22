#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTimer>
#include "ChartWidget.h"

/* ═══════════════════════════════════════════════════════════════
 *  1. 折线图 — 每条线不同散点样式
 * ═══════════════════════════════════════════════════════════════ */
static ChartWidget* createLineChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("多散点样式折线图");

    DateTimeAxis *axisX = new DateTimeAxis(Axis::Bottom);
    axisX->setTitle("时间");
    chart->addAxis(axisX);

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("温度 (°C)");
    chart->addAxis(axisY);

    QDateTime base = QDateTime::currentDateTime().addSecs(-600);

    /* 传感器A — 圆形 */
    LineSeries *line1 = new LineSeries("传感器A (圆形)");
    line1->setScatterStyle(ScatterCircle);
    line1->setMarkerSize(5);

    /* 传感器B — 菱形 */
    LineSeries *line2 = new LineSeries("传感器B (菱形)");
    line2->setScatterStyle(ScatterDiamond);
    line2->setMarkerSize(5);

    /* 传感器C — 三角形 */
    LineSeries *line3 = new LineSeries("传感器C (三角形)");
    line3->setScatterStyle(ScatterTriangle);
    line3->setMarkerSize(5);

    /* 传感器D — 十字 */
    LineSeries *line4 = new LineSeries("传感器D (十字)");
    line4->setScatterStyle(ScatterCross);
    line4->setMarkerSize(5);

    /* 传感器E — 五角星 */
    LineSeries *line5 = new LineSeries("传感器E (五角星)");
    line5->setScatterStyle(ScatterStar);
    line5->setMarkerSize(5);

    /* 传感器F — 加号 */
    LineSeries *line6 = new LineSeries("传感器F (加号)");
    line6->setScatterStyle(ScatterPlus);
    line6->setMarkerSize(5);

    for (int i = 0; i < 60; ++i) {
        QDateTime t = base.addSecs(i * 10);
        line1->append(t, 20.0 + 5.0 * std::sin(i * 0.2)   + (qrand() % 60) / 100.0);
        line2->append(t, 22.0 + 3.0 * std::cos(i * 0.15)  + (qrand() % 60) / 100.0);
        line3->append(t, 18.0 + 4.0 * std::sin(i * 0.25)  + (qrand() % 60) / 100.0);
        line4->append(t, 25.0 + 2.0 * std::cos(i * 0.18)  + (qrand() % 60) / 100.0);
        line5->append(t, 16.0 + 6.0 * std::sin(i * 0.12)  + (qrand() % 60) / 100.0);
        line6->append(t, 21.0 + 3.5 * std::cos(i * 0.22)  + (qrand() % 60) / 100.0);
    }

    chart->addSeries(line1);
    chart->addSeries(line2);
    chart->addSeries(line3);
    chart->addSeries(line4);
    chart->addSeries(line5);
    chart->addSeries(line6);

    /* 动态追加 */
    QTimer *timer = new QTimer(chart);
    static int tick = 60;
    QObject::connect(timer, &QTimer::timeout, [chart, line1, line2, line3, line4, line5, line6, axisX]() {
        QDateTime now = QDateTime::currentDateTime();
        line1->append(now, 20.0 + 5.0 * std::sin(tick * 0.2)   + (qrand() % 60) / 100.0);
        line2->append(now, 22.0 + 3.0 * std::cos(tick * 0.15)  + (qrand() % 60) / 100.0);
        line3->append(now, 18.0 + 4.0 * std::sin(tick * 0.25)  + (qrand() % 60) / 100.0);
        line4->append(now, 25.0 + 2.0 * std::cos(tick * 0.18)  + (qrand() % 60) / 100.0);
        line5->append(now, 16.0 + 6.0 * std::sin(tick * 0.12)  + (qrand() % 60) / 100.0);
        line6->append(now, 21.0 + 3.5 * std::cos(tick * 0.22)  + (qrand() % 60) / 100.0);
        ++tick;
        while (line1->dataCount() > 120) { line1->removeAt(0); line2->removeAt(0);
            line3->removeAt(0); line4->removeAt(0); line5->removeAt(0); line6->removeAt(0); }
        axisX->setRange(now.addSecs(-1200), now);
        chart->refresh();
    });
    timer->start(1000);

    /* 默认垂直图例 */
    chart->setLegendOrientation(ChartWidget::LegendVertical);

    return chart;
}

/* ═══════════════════════════════════════════════════════════════
 *  2. 散点样式一览（无折线，纯散点）
 * ═══════════════════════════════════════════════════════════════ */
static ChartWidget* createScatterGallery(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("散点样式一览");

    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    axisX->setTitle("X");
    chart->addAxis(axisX);

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("Y");
    chart->addAxis(axisY);

    struct StyleInfo { const char *name; ScatterStyle style; };
    StyleInfo styles[] = {
                          {"Circle (圆形)",   ScatterCircle},
                          {"Square (正方形)", ScatterSquare},
                          {"Diamond (菱形)",  ScatterDiamond},
                          {"Triangle (三角形)", ScatterTriangle},
                          {"Cross (十字)",    ScatterCross},
                          {"Plus (加号)",     ScatterPlus},
                          {"Star (五角星)",   ScatterStar},
                          {"Dot (小圆点)",    ScatterDot},
                          };

    for (int i = 0; i < 8; ++i) {
        LineSeries *s = new LineSeries(styles[i].name);
        s->setScatterStyle(styles[i].style);
        s->setMarkerSize(7);
        s->setLineWidth(1.5);
        /* 每种样式 8 个点，沿 X 分布 */
        for (int j = 0; j < 8; ++j) {
            double x = i * 10.0 + j;
            double y = 50.0 + 30.0 * std::sin(j * 0.6 + i * 0.8) + (qrand() % 20 - 10);
            s->append(x, y);
        }
        chart->addSeries(s);
    }

    /* 垂直图例，右上角 */
    chart->setLegendOrientation(ChartWidget::LegendVertical);

    return chart;
}

/* ═══════════════════════════════════════════════════════════════
 *  3. 柱状图
 * ═══════════════════════════════════════════════════════════════ */
static ChartWidget* createBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("季度销售额（柱状图）");
    chart->setCategories({"Q1", "Q2", "Q3", "Q4"});
    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("销售额 (万元)");
    chart->addAxis(axisY);
    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    axisX->setTitle("季度");
    chart->addAxis(axisX);

    BarSeries *bar1 = new BarSeries("2024年");
    bar1->append(120); bar1->append(150); bar1->append(98); bar1->append(180);
    chart->addSeries(bar1);

    BarSeries *bar2 = new BarSeries("2025年");
    bar2->append(135); bar2->append(168); bar2->append(112); bar2->append(200);
    chart->addSeries(bar2);

    return chart;
}

/* ═══════════════════════════════════════════════════════════════
 *  4. 堆叠柱状图
 * ═══════════════════════════════════════════════════════════════ */
static ChartWidget* createStackedBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("产品构成（堆叠柱状图）");
    chart->setCategories({"华东", "华南", "华北", "西南", "华中"});
    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("销量 (万件)");
    chart->addAxis(axisY);
    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    chart->addAxis(axisX);

    StackedBarSeries *s1 = new StackedBarSeries("产品A");
    s1->append(30); s1->append(45); s1->append(28); s1->append(22); s1->append(35);
    chart->addSeries(s1);

    StackedBarSeries *s2 = new StackedBarSeries("产品B");
    s2->append(20); s2->append(32); s2->append(18); s2->append(15); s2->append(25);
    chart->addSeries(s2);

    StackedBarSeries *s3 = new StackedBarSeries("产品C");
    s3->append(15); s3->append(28); s3->append(12); s3->append(10); s3->append(20);
    chart->addSeries(s3);

    return chart;
}

/* ═══════════════════════════════════════════════════════════════
 *  5. 混合图
 * ═══════════════════════════════════════════════════════════════ */
static ChartWidget* createMixedChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("月度营收与利润率（混合图）");
    chart->setCategories({"1月","2月","3月","4月","5月","6月",
                          "7月","8月","9月","10月","11月","12月"});
    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("营收 (万元)");
    chart->addAxis(axisY);
    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    axisX->setTitle("月份");
    chart->addAxis(axisX);

    BarSeries *bars = new BarSeries("营收");
    bars->append(85); bars->append(92); bars->append(78); bars->append(110);
    bars->append(125); bars->append(118); bars->append(130); bars->append(142);
    bars->append(135); bars->append(148); bars->append(155); bars->append(160);
    chart->addSeries(bars);

    LineSeries *line = new LineSeries("利润率 (‰)");
    line->setScatterStyle(ScatterDiamond);
    line->setMarkerSize(5);
    double rates[] = {12, 14, 10, 16, 18, 15, 20, 22, 19, 23, 25, 24};
    for (int i = 0; i < 12; ++i) line->append(double(i), rates[i] * 6.0);
    line->setColor(QColor(46, 204, 113));
    chart->addSeries(line);

    return chart;
}

/* ═══════════════════════════════════════════════════════════════
 *  main
 * ═══════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    srand(static_cast<uint>(time(nullptr)));

    QMainWindow win;
    win.setWindowTitle("Qt5 Custom Chart — 散点样式 / 图例布局 / Tooltip 颜色");
    win.resize(1100, 750);

    QTabWidget *tabs = new QTabWidget;
    tabs->addTab(createLineChart(tabs),        "折线图 (多散点样式)");
    tabs->addTab(createScatterGallery(tabs),   "散点样式一览");
    tabs->addTab(createBarChart(tabs),         "柱状图");
    tabs->addTab(createStackedBarChart(tabs),  "堆叠柱状图");
    tabs->addTab(createMixedChart(tabs),       "混合图");

    win.setCentralWidget(tabs);
    win.show();
    return app.exec();
}
