#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QStandardPaths>
#include <QtMath>

#include "ChartWidget.h"

// ========================================================================
// 折线图 Demo（展示实时数据 + 填充 + 主题切换）
// ========================================================================
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
    chart->setLegendOrientation(ChartWidget::LegendOrientation::Vertical);

    // X 轴：日期时间轴
    Axis *axisX = new Axis(AxisPosition::Bottom);
    axisX->setTitle("Time");
    axisX->setSubTickCount(2);
    chart->addAxis(axisX);

    // Y 轴：数值轴
    Axis *axisY = new Axis(AxisPosition::Left);
    axisY->setTitle("Temperature (C)");
    axisY->setSubTickCount(4);
    chart->addAxis(axisY);

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
    static bool darkMode = false;
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

    // 主题切换
    QObject::connect(btnTheme, &QPushButton::clicked, [chart, btnTheme]() mutable {
        static bool dark = false;
        dark = !dark;
        chart->setTheme(dark ? ChartTheme::dark() : ChartTheme::light());
        btnTheme->setText(dark ? "Theme: Dark" : "Theme: Light");
    });

    layout->addWidget(chart);
    return page;
}

// ========================================================================
// 柱状图 Demo
// ========================================================================
static ChartWidget* createBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Quarterly Sales");
    chart->setCategories({"Q1", "Q2", "Q3", "Q4"});

    chart->addAxis(new Axis(AxisPosition::Left));
    chart->addAxis(new Axis(AxisPosition::Bottom));

    BarSeries *b1 = new BarSeries("2024");
    b1->append(120); b1->append(150); b1->append(98); b1->append(180);
    chart->addSeries(b1);

    BarSeries *b2 = new BarSeries("2025");
    b2->append(135); b2->append(168); b2->append(112); b2->append(200);
    chart->addSeries(b2);

    return chart;
}

// ========================================================================
// 堆叠柱状图 Demo
// ========================================================================
static ChartWidget* createStackedBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Product Composition");
    chart->setCategories({"East", "South", "North", "West", "Central"});

    chart->addAxis(new Axis(AxisPosition::Left));
    chart->addAxis(new Axis(AxisPosition::Bottom));

    StackedBarSeries *s1 = new StackedBarSeries("Product A");
    s1->append(30); s1->append(45); s1->append(28); s1->append(22); s1->append(35);
    chart->addSeries(s1);

    StackedBarSeries *s2 = new StackedBarSeries("Product B");
    s2->append(20); s2->append(32); s2->append(18); s2->append(15); s2->append(25);
    chart->addSeries(s2);

    StackedBarSeries *s3 = new StackedBarSeries("Product C");
    s3->append(15); s3->append(28); s3->append(12); s3->append(10); s3->append(20);
    chart->addSeries(s3);

    return chart;
}

// ========================================================================
// 混合图 Demo
// ========================================================================
static ChartWidget* createMixedChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Monthly Revenue & Profit Rate");
    chart->setLegendPosition(ChartWidget::LegendPosition::OutsideTop);
    chart->setCategories({"Jan","Feb","Mar","Apr","May","Jun",
                          "Jul","Aug","Sep","Oct","Nov","Dec"});

    chart->addAxis(new Axis(AxisPosition::Left));
    chart->addAxis(new Axis(AxisPosition::Bottom));

    BarSeries *bars = new BarSeries("Revenue");
    double vals[] = {85,92,78,110,125,118,130,142,135,148,155,160};
    for (double v : vals) bars->append(v);
    chart->addSeries(bars);

    LineSeries *line = new LineSeries("Profit Rate");
    line->setScatterStyle(ScatterStyle::Diamond);
    line->setColor(QColor(46, 204, 113));
    double rates[] = {12,14,10,16,18,15,20,22,19,23,25,24};
    for (int i = 0; i < 12; ++i) line->append(double(i), rates[i] * 6.0);
    chart->addSeries(line);

    return chart;
}

// ========================================================================
// 本周日期折线图 Demo（30个随机数据点，X轴为本周7天）
// ========================================================================
static ChartWidget* createWeekLineChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Weekly Activity (30 Samples)");
    chart->setLegendPosition(ChartWidget::LegendPosition::OutsideTop);

    // X 轴：日期时间轴
    Axis *axisX = new Axis(AxisPosition::Bottom);
    axisX->setType(AxisType::Date);             // ✅ 日期轴
    axisX->setDateFormat(DateFormat::MMdd);     // ✅ 显示 MM-dd 格式
    axisX->setTitle("Date");
    axisX->setSubTickCount(2);
    chart->addAxis(axisX);

    // Y 轴：数值轴，范围 0-20
    Axis *axisY = new Axis(AxisPosition::Left);
    axisY->setTitle("Score");
    axisY->setRange(0, 20);
    axisY->setSubTickCount(4);
    chart->addAxis(axisY);

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


// ========================================================================
// 主函数
// ========================================================================
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
    tabs->addTab(createMixedChart(tabs),       "Mixed Chart");
    tabs->addTab(createWeekLineChart(tabs),    "Week Line");   // ← 新增

    win.setCentralWidget(tabs);
    win.show();

    return app.exec();
}
