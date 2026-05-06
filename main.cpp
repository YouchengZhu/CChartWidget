#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QStandardPaths>
#include <QtMath>
#include <cstdlib>

#include "chartwidget.h"

// ------------------------------------------------------------------------
// 折线图 Demo（展示实时数据 + 填充 + 主题切换）
// ------------------------------------------------------------------------
static QWidget* createLineChartDemo(QWidget *parent)
{
    QWidget *page = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout(page);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *btnExport = new QPushButton("Export PNG");
    QPushButton *btnTheme  = new QPushButton("Theme: Light");
    btnLayout->addWidget(btnExport);
    btnLayout->addWidget(btnTheme);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    ChartWidget *chart = new ChartWidget;
    chart->setTitle("Temperature Monitor");
    chart->setLegendOrientation(Legend::Vertical); // 使用 Legend::Orientation

    // 获取默认轴并进行配置
    Axis *axisX = chart->axisX();
    axisX->setTitle("Time");
    axisX->setSubTickCount(2);
    // 默认轴类型已是 Numeric，后续会自动根据数据调整（此处可保持默认）

    Axis *axisY = chart->axisY();
    axisY->setTitle("Temperature (C)");
    axisY->setSubTickCount(4);

    QDateTime base = QDateTime::currentDateTime().addSecs(-600);

    LineSeries *line1 = new LineSeries("Sensor A");
    line1->setScatterStyle(ScatterStyle::Circle);
    QLinearGradient grad(QPointF(0, 0), QPointF(0, 100));
    grad.setColorAt(0.0, QColor(255, 0, 0, 180));
    grad.setColorAt(0.5, QColor(255, 255, 0, 120));
    grad.setColorAt(1.0, QColor(0, 0, 255, 50));
    line1->setFillBrush(grad);
    line1->setFillEnabled(true);

    LineSeries *line2 = new LineSeries("Sensor B");
    line2->setScatterStyle(ScatterStyle::Diamond);

    LineSeries *line3 = new LineSeries("Sensor C");
    line3->setScatterStyle(ScatterStyle::Triangle);

    for (int i = 0; i < 60; ++i) {
        QDateTime t = base.addSecs(i * 10);
        line1->append(t, 20.0 + 5.0 * std::sin(i * 0.2) + (qrand() % 60) / 100.0);
        line2->append(t, 22.0 + 3.0 * std::cos(i * 0.15) + (qrand() % 60) / 100.0);
        line3->append(t, 18.0 + 4.0 * std::sin(i * 0.25) + (qrand() % 60) / 100.0);
    }
    chart->addSeries(line1);
    chart->addSeries(line2);
    chart->addSeries(line3);

    // 实时更新
    QTimer *timer = new QTimer(chart);
    static int tick = 60;
    QObject::connect(timer, &QTimer::timeout, [=]() mutable {
        QDateTime now = QDateTime::currentDateTime();
        line1->append(now, 20.0 + 5.0 * std::sin(tick * 0.2) + (qrand() % 60) / 100.0);
        line2->append(now, 22.0 + 3.0 * std::cos(tick * 0.15) + (qrand() % 60) / 100.0);
        line3->append(now, 18.0 + 4.0 * std::sin(tick * 0.25) + (qrand() % 60) / 100.0);
        ++tick;
        while (line1->dataCount() > 120) {
            line1->removeAt(0);
            line2->removeAt(0);
            line3->removeAt(0);
        }
        // 时间轴自动滚动
        axisX->setRange(now.addSecs(-1200).toSecsSinceEpoch(),
                        now.toSecsSinceEpoch());
        chart->refresh();
    });
    timer->start(1000);

    // 导出
    QObject::connect(btnExport, &QPushButton::clicked, [chart]() {
        QPixmap pm = chart->exportToPixmap(QSize(1920, 1080));
        QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                       + "/chart_export.png";
        pm.save(path);
    });

    // 主题切换（同时设置 Axis 和 Legend 的样式以适配暗色主题）
    QObject::connect(btnTheme, &QPushButton::clicked, [chart, btnTheme, axisX, axisY]() mutable {
        static bool dark = false;
        dark = !dark;
        if (dark) {
            chart->setTheme(ChartTheme::dark());
            // 轴样式
            axisX->setTickColor(QColor(140,140,140));
            axisX->setSubTickColor(QColor(80,80,80));
            axisX->setGridColor(QColor(70,70,70));
            axisX->setTitleColor(QColor(210,210,210));
            axisY->setTickColor(QColor(140,140,140));
            axisY->setSubTickColor(QColor(80,80,80));
            axisY->setGridColor(QColor(70,70,70));
            axisY->setTitleColor(QColor(210,210,210));
            // 图例
            Legend leg = chart->legend();
            leg.borderColor = QColor(80,80,80);
            leg.backgroundColor = QColor(50,50,50,220);
            chart->setLegend(leg);
        } else {
            chart->setTheme(ChartTheme::light());
            // 恢复默认轴样式
            axisX->setTickColor(QColor(120,120,120));
            axisX->setSubTickColor(QColor(200,200,200));
            axisX->setGridColor(QColor(210,210,210));
            axisX->setTitleColor(QColor(50,50,50));
            axisY->setTickColor(QColor(120,120,120));
            axisY->setSubTickColor(QColor(200,200,200));
            axisY->setGridColor(QColor(210,210,210));
            axisY->setTitleColor(QColor(50,50,50));
            // 图例恢复
            Legend leg = chart->legend();
            leg.borderColor = QColor(180,180,180);
            leg.backgroundColor = QColor(255,255,255,235);
            chart->setLegend(leg);
        }
        btnTheme->setText(dark ? "Theme: Dark" : "Theme: Light");
    });

    layout->addWidget(chart);
    return page;
}

// ------------------------------------------------------------------------
// 柱状图 Demo（X 轴使用日期）
// ------------------------------------------------------------------------
static ChartWidget* createBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Daily Sales (Date Axis)");

    Axis *axisX = chart->axisX();
    axisX->setType(AxisType::Date);
    axisX->setDateFormat(DateFormat::MMdd);
    axisX->setTitle("Date");

    chart->axisY()->setTitle("Sales");

    QDate startDate(2026, 5, 1);
    int days = 14;

    axisX->setRange(
        QDateTime(startDate, QTime(0, 0)).toSecsSinceEpoch(),
        QDateTime(startDate.addDays(days), QTime(0, 0)).toSecsSinceEpoch()
    );

    BarSeries *b1 = new BarSeries("Product A");
    BarSeries *b2 = new BarSeries("Product B");
    for (int d = 0; d < days; ++d) {
        double key = QDateTime(startDate.addDays(d), QTime(0, 0)).toSecsSinceEpoch();
        b1->append(key, 50 + (qrand() % 80));
        b2->append(key, 30 + (qrand() % 60));
    }
    chart->addSeries(b1);
    chart->addSeries(b2);

    return chart;
}

// ------------------------------------------------------------------------
// 堆叠柱状图 Demo（X 轴使用显式 key 值）
// ------------------------------------------------------------------------
static ChartWidget* createStackedBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Product Composition");
    chart->axisX()->setTitle("Region");
    chart->axisX()->setRange(0.5, 5.5);
    chart->axisY()->setTitle("Units");

    StackedBarSeries *s1 = new StackedBarSeries("Product A");
    s1->append(1, 30); s1->append(2, 45); s1->append(3, 28); s1->append(4, 22); s1->append(5, 35);
    chart->addSeries(s1);

    StackedBarSeries *s2 = new StackedBarSeries("Product B");
    s2->append(1, 20); s2->append(2, 32); s2->append(3, 18); s2->append(4, 15); s2->append(5, 25);
    chart->addSeries(s2);

    StackedBarSeries *s3 = new StackedBarSeries("Product C");
    s3->append(1, 15); s3->append(2, 28); s3->append(3, 12); s3->append(4, 10); s3->append(5, 20);
    chart->addSeries(s3);

    return chart;
}

// ------------------------------------------------------------------------
// 范围柱状图 Demo（每日温度 min/max）
// ------------------------------------------------------------------------
static ChartWidget* createRangeBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Daily Temperature Range");

    Axis *axisX = chart->axisX();
    axisX->setType(AxisType::Date);
    axisX->setDateFormat(DateFormat::MMdd);
    axisX->setTitle("Date");

    chart->axisY()->setTitle("Temperature (°C)");

    QDate startDate(2026, 5, 1);
    int days = 7;
    axisX->setRange(
        QDateTime(startDate, QTime(0, 0)).toSecsSinceEpoch(),
        QDateTime(startDate.addDays(days), QTime(0, 0)).toSecsSinceEpoch()
    );

    RangeBarSeries *range = new RangeBarSeries("Beijing");
    RangeBarSeries *range2 = new RangeBarSeries("Shanghai");
    double minTemps[]  = {12, 14, 10, 13, 15, 11, 16};
    double maxTemps[]  = {26, 28, 22, 25, 29, 24, 30};
    double minTemps2[] = {15, 16, 13, 14, 17, 14, 18};
    double maxTemps2[] = {24, 25, 21, 22, 26, 23, 27};
    for (int d = 0; d < days; ++d) {
        double key = QDateTime(startDate.addDays(d), QTime(0, 0)).toSecsSinceEpoch();
        range->append(key, minTemps[d], maxTemps[d]);
        range2->append(key, minTemps2[d], maxTemps2[d]);
    }
    chart->addSeries(range);
    chart->addSeries(range2);

    return chart;
}

// ------------------------------------------------------------------------
// 混合图 Demo
// ------------------------------------------------------------------------
static ChartWidget* createMixedChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Monthly Revenue & Profit Rate");
    chart->setLegendPosition(Legend::AboveChart);
    chart->axisX()->setTitle("Month");
    chart->axisX()->setRange(0, 13);
    chart->axisY()->setTitle("Revenue / Profit Rate");

    BarSeries *bars = new BarSeries("Revenue");
    double vals[] = {85,92,78,110,125,118,130,142,135,148,155,160};
    for (int i = 0; i < 12; ++i) bars->append(double(i + 1), vals[i]);
    chart->addSeries(bars);

    LineSeries *line = new LineSeries("Profit Rate");
    line->setScatterStyle(ScatterStyle::Diamond);
    line->setColor(QColor(46, 204, 113));
    double rates[] = {12,14,10,16,18,15,20,22,19,23,25,24};
    for (int i = 0; i < 12; ++i) line->append(double(i + 1), rates[i] * 6.0);
    chart->addSeries(line);

    return chart;
}

// ------------------------------------------------------------------------
// 本周日期折线图 Demo（30个随机数据点，X轴为本周7天）
// ------------------------------------------------------------------------
static ChartWidget* createWeekLineChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Weekly Activity (30 Samples)");
    chart->setLegendPosition(Legend::AboveChart);

    // 获取默认 X 轴，配置为日期类型
    Axis *axisX = chart->axisX();
    axisX->setType(AxisType::Date);             // 日期轴
    axisX->setDateFormat(DateFormat::MMdd);     // 显示 MM-dd 格式
    axisX->setTitle("Date");
    axisX->setSubTickCount(2);

    // 获取默认 Y 轴，固定范围 0-20
    Axis *axisY = chart->axisY();
    axisY->setTitle("Score");
    axisY->setRange(0, 20);
    axisY->setSubTickCount(4);

    // 本周一 00:00
    QDate today = QDate::currentDate();
    int dow = today.dayOfWeek(); // 1=Monday ... 7=Sunday
    QDate monday = today.addDays(-(dow - 1));
    QDateTime weekStart(monday, QTime(0, 0, 0));

    // 生成 30 个随机数据点，均匀分布在 7 天内
    LineSeries *line = new LineSeries("Samples");
    line->setScatterStyle(ScatterStyle::Circle);
    line->setMarkerSize(6.0);
    line->setColor(QColor(54, 162, 235));

    for (int i = 0; i < 30; ++i) {
        int dayOffset = i * 7 / 30;                                          // 0~6
        int randSecs = qrand() % 86400;                                      // 0~86399
        QDateTime t = weekStart.addDays(dayOffset).addSecs(randSecs);
        double y = 1.0 + (qrand() % 190) / 10.0;                            // 1.0 ~ 19.9
        line->append(t, y);
    }

    // 设定 X 轴范围为整周
    axisX->setRange(
        double(weekStart.toSecsSinceEpoch()),
        double(weekStart.addDays(7).toSecsSinceEpoch())
        );

    chart->addSeries(line);
    return chart;
}

// ------------------------------------------------------------------------
// 10K 点性能测试 Demo
// ------------------------------------------------------------------------
static ChartWidget* createPerfTestChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("10,000 Points Performance Test");
    chart->setLegendPosition(Legend::AboveChart);

    Axis *axisX = chart->axisX();
    axisX->setTitle("X");
    axisX->setRange(0, 10000);

    Axis *axisY = chart->axisY();
    axisY->setTitle("Y");
    axisY->setRange(-10, 10);

    LineSeries *line = new LineSeries("Sine Wave");
    line->setScatterStyle(ScatterStyle::Circle);
    line->setMarkerSize(4.0);
    line->setLineWidth(1.0);
    line->setColor(QColor(54, 162, 235));

    for (int i = 0; i < 10000; ++i)
        line->append(double(i), 5.0 * std::sin(i * 0.05) + 2.0 * std::cos(i * 0.013));

    chart->addSeries(line);
    return chart;
}

// ------------------------------------------------------------------------
// 主函数
// ------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    srand(static_cast<uint>(time(nullptr)));

    QMainWindow win;
    win.setWindowTitle("Qt5 Chart Library (Refactored)");
    win.resize(1100, 750);

    QTabWidget *tabs = new QTabWidget;
    tabs->addTab(createLineChartDemo(tabs),    "Line Chart");
    tabs->addTab(createBarChart(tabs),         "Bar Chart");
    tabs->addTab(createStackedBarChart(tabs),  "Stacked Bar");
    tabs->addTab(createRangeBarChart(tabs),    "Range Bar");
    tabs->addTab(createMixedChart(tabs),       "Mixed Chart");
    tabs->addTab(createWeekLineChart(tabs),    "Week Line");
    tabs->addTab(createPerfTestChart(tabs),    "10K Points");

    win.setCentralWidget(tabs);
    win.show();

    return app.exec();
}
