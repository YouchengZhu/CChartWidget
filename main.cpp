#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QStandardPaths>
#include "chartWidget.h"

static QWidget* createLineChartDemo(QWidget *parent)
{
    QWidget *page = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout(page);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *btnExport = new QPushButton("Export PNG");
    QPushButton *btnYGrid  = new QPushButton("H-Grid: ON");
    QPushButton *btnXGrid  = new QPushButton("V-Grid: ON");
    QPushButton *btnSubTick = new QPushButton("Sub-Tick: ON");
    QPushButton *btnTick    = new QPushButton("Major Tick: ON");
    QPushButton *btnSubDir  = new QPushButton("Sub-Tick: Inside");
    btnLayout->addWidget(btnExport);
    btnLayout->addWidget(btnYGrid);
    btnLayout->addWidget(btnXGrid);
    btnLayout->addWidget(btnSubTick);
    btnLayout->addWidget(btnTick);
    btnLayout->addWidget(btnSubDir);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    ChartWidget *chart = new ChartWidget;
    chart->setTitle("Temperature Monitor - Sub-Ticks & Grid Control");
    chart->setLegendOrientation(ChartWidget::LegendVertical);

    DateTimeAxis *axisX = new DateTimeAxis(Axis::Bottom);
    axisX->setTitle("Time");
    axisX->setSubTickCount(2);
    axisX->setTickColor(QColor(120, 120, 120));
    axisX->setSubTickColor(QColor(200, 200, 200));
    chart->addAxis(axisX);

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("Temperature (C)");
    axisY->setSubTickCount(4);
    axisY->setTickColor(QColor(120, 120, 120));
    axisY->setSubTickColor(QColor(200, 200, 200));
    chart->addAxis(axisY);

    QDateTime base = QDateTime::currentDateTime().addSecs(-600);
    LineSeries *line1 = new LineSeries("Sensor A");
    line1->setScatterStyle(ScatterCircle);
    LineSeries *line2 = new LineSeries("Sensor B");
    line2->setScatterStyle(ScatterDiamond);
    LineSeries *line3 = new LineSeries("Sensor C");
    line3->setScatterStyle(ScatterTriangle);

    for (int i = 0; i < 60; ++i) {
        QDateTime t = base.addSecs(i * 10);
        line1->append(t, 20.0 + 5.0 * std::sin(i * 0.2) + (qrand() % 60) / 100.0);
        line2->append(t, 22.0 + 3.0 * std::cos(i * 0.15) + (qrand() % 60) / 100.0);
        line3->append(t, 18.0 + 4.0 * std::sin(i * 0.25) + (qrand() % 60) / 100.0);
    }
    chart->addSeries(line1);
    chart->addSeries(line2);
    chart->addSeries(line3);

    QTimer *timer = new QTimer(chart);
    static int tick = 60;
    QObject::connect(timer, &QTimer::timeout, [=]() {
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
        axisX->setRange(now.addSecs(-1200), now);
        chart->refresh();
    });
    timer->start(1000);

    QObject::connect(btnExport, &QPushButton::clicked, [chart]() {
        QPixmap pm = chart->exportToPixmap(QSize(1920, 1080));
        QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/chart_export.png";
        pm.save(path);
    });
    QObject::connect(btnYGrid, &QPushButton::clicked, [chart, axisY, btnYGrid]() {
        bool on = !axisY->isHorizontalGridVisible();
        axisY->setHorizontalGridVisible(on);
        btnYGrid->setText(QString("H-Grid: %1").arg(on ? "ON" : "OFF"));
        chart->update();
    });
    QObject::connect(btnXGrid, &QPushButton::clicked, [chart, axisX, btnXGrid]() {
        bool on = !axisX->isVerticalGridVisible();
        axisX->setVerticalGridVisible(on);
        btnXGrid->setText(QString("V-Grid: %1").arg(on ? "ON" : "OFF"));
        chart->update();
    });
    QObject::connect(btnSubTick, &QPushButton::clicked, [chart, axisX, axisY, btnSubTick]() {
        bool on = !axisX->isSubTicksVisible();
        axisX->setSubTicksVisible(on);
        axisY->setSubTicksVisible(on);
        btnSubTick->setText(QString("Sub-Tick: %1").arg(on ? "ON" : "OFF"));
        chart->update();
    });
    QObject::connect(btnTick, &QPushButton::clicked, [chart, axisX, axisY, btnTick]() {
        bool on = !axisX->isTicksVisible();
        axisX->setTicksVisible(on);
        axisY->setTicksVisible(on);
        btnTick->setText(QString("Major Tick: %1").arg(on ? "ON" : "OFF"));
        chart->update();
    });
    QObject::connect(btnSubDir, &QPushButton::clicked, [chart, axisX, axisY, btnSubDir]() {
        bool inside = (axisX->subTickDirection() == Axis::SubTickInside);
        Axis::SubTickDirection dir = inside ? Axis::SubTickOutside : Axis::SubTickInside;
        axisX->setSubTickDirection(dir);
        axisY->setSubTickDirection(dir);
        btnSubDir->setText(QString("Sub-Tick: %1").arg(inside ? "Outside" : "Inside"));
        chart->update();
    });

    layout->addWidget(chart);
    return page;
}

static ChartWidget* createScatterGallery(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Scatter Style Gallery");
    chart->setLegendOrientation(ChartWidget::LegendVertical);

    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    axisX->setTitle("X");
    chart->addAxis(axisX);

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("Y");
    chart->addAxis(axisY);

    struct SI { const char *name; ScatterStyle style; };
    SI styles[] = {
                   {"Circle", ScatterCircle}, {"Square", ScatterSquare},
                   {"Diamond", ScatterDiamond}, {"Triangle", ScatterTriangle},
                   {"Cross", ScatterCross}, {"Plus", ScatterPlus},
                   {"Star", ScatterStar}, {"Dot", ScatterDot},
                   };
    for (int i = 0; i < 8; ++i) {
        LineSeries *s = new LineSeries(styles[i].name);
        s->setScatterStyle(styles[i].style);
        s->setMarkerSize(7);
        s->setLineWidth(1.5);
        for (int j = 0; j < 8; ++j) {
            s->append(i * 10.0 + j, 50.0 + 30.0 * std::sin(j * 0.6 + i * 0.8) + (qrand() % 20 - 10));
        }
        chart->addSeries(s);
    }
    return chart;
}

static ChartWidget* createBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Quarterly Sales");
    chart->setCategories({"Q1", "Q2", "Q3", "Q4"});

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("10K CNY");
    chart->addAxis(axisY);

    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    chart->addAxis(axisX);

    BarSeries *b1 = new BarSeries("2024");
    b1->append(120); b1->append(150); b1->append(98); b1->append(180);
    chart->addSeries(b1);

    BarSeries *b2 = new BarSeries("2025");
    b2->append(135); b2->append(168); b2->append(112); b2->append(200);
    chart->addSeries(b2);

    return chart;
}

static ChartWidget* createStackedBarChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Product Composition");
    chart->setCategories({"East", "South", "North", "West", "Central"});

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("10K Units");
    chart->addAxis(axisY);

    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    chart->addAxis(axisX);

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

static ChartWidget* createMixedChart(QWidget *parent)
{
    ChartWidget *chart = new ChartWidget(parent);
    chart->setTitle("Monthly Revenue & Profit Rate");
    chart->setCategories({"Jan","Feb","Mar","Apr","May","Jun",
                          "Jul","Aug","Sep","Oct","Nov","Dec"});

    NumericAxis *axisY = new NumericAxis(Axis::Left);
    axisY->setTitle("10K CNY");
    chart->addAxis(axisY);

    NumericAxis *axisX = new NumericAxis(Axis::Bottom);
    chart->addAxis(axisX);

    BarSeries *bars = new BarSeries("Revenue");
    bars->append(85); bars->append(92); bars->append(78); bars->append(110);
    bars->append(125); bars->append(118); bars->append(130); bars->append(142);
    bars->append(135); bars->append(148); bars->append(155); bars->append(160);
    chart->addSeries(bars);

    LineSeries *line = new LineSeries("Profit Rate");
    line->setScatterStyle(ScatterDiamond);
    double rates[] = {12, 14, 10, 16, 18, 15, 20, 22, 19, 23, 25, 24};
    for (int i = 0; i < 12; ++i) {
        line->append(double(i), rates[i] * 6.0);
    }
    line->setColor(QColor(46, 204, 113));
    chart->addSeries(line);

    return chart;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    srand(static_cast<uint>(time(nullptr)));

    QMainWindow win;
    win.setWindowTitle("Qt5 Custom Chart Library");
    win.resize(1100, 750);

    QTabWidget *tabs = new QTabWidget;
    tabs->addTab(createLineChartDemo(tabs),    "Line (Sub-Tick)");
    tabs->addTab(createScatterGallery(tabs),   "Scatter Gallery");
    tabs->addTab(createBarChart(tabs),         "Bar Chart");
    tabs->addTab(createStackedBarChart(tabs),  "Stacked Bar");
    tabs->addTab(createMixedChart(tabs),       "Mixed Chart");

    win.setCentralWidget(tabs);
    win.show();

    return app.exec();
}
