#include "SignatureDesigner.h"
#include "ui_SignatureDesigner.h"
#include <QMouseEvent>
#include <QPainter>
#include <cmath>
#include <QDesktopWidget>
#include <QInputDialog>
#include "clipper2/clipper.h"

SignatureDesigner::SignatureDesigner(QWidget* parent) : 
    QMainWindow(parent),
    ui(new Ui::SignatureDesigner) 
{   // 信号与槽
    order = 3;  // 默认3阶B样条曲线
    width = 5;  // 默认线宽为 5 pixels
    color = { 0,0,0 };
    mode = 1;   // 默认为曲线设计模式
    isOpen = true;  // 默认为开曲线
    offset = 10;   // 默认offset为-10
    fillMode = 1;
    pixmap = QPixmap(QDesktopWidget().availableGeometry(this).size() * 0.7);
    pixmap.fill(Qt::transparent);
    toFilename = ".\\output\\test.png";
    fromFilename = ".//swan.png";
    ui->setupUi(this);

    connect(ui->actionSave, &QAction::triggered, this, &SignatureDesigner::saveFile);
    connect(ui->actionNew, &QAction::triggered, this, &SignatureDesigner::newFile);
    connect(ui->actionExit, &QAction::triggered, this, &SignatureDesigner::exitFile);
    connect(ui->actionRedo, &QAction::triggered, this, &SignatureDesigner::redoFile);
    connect(ui->actionOrder_1, &QAction::triggered, this, &SignatureDesigner::setOrder1);
    connect(ui->actionOrder_2, &QAction::triggered, this, &SignatureDesigner::setOrder2);
    connect(ui->actionOrder_3, &QAction::triggered, this, &SignatureDesigner::setOrder3);
    connect(ui->action1_pixel, &QAction::triggered, this, &SignatureDesigner::setWidth1);
    connect(ui->action3_pixel, &QAction::triggered, this, &SignatureDesigner::setWidth3);
    connect(ui->action5_pixel, &QAction::triggered, this, &SignatureDesigner::setWidth5);
    connect(ui->action7_pixel, &QAction::triggered, this, &SignatureDesigner::setWidth7);
    connect(ui->actionBlack, &QAction::triggered, this, &SignatureDesigner::setColorBlack);
    connect(ui->actionRed, &QAction::triggered, this, &SignatureDesigner::setColorRed);
    connect(ui->actionBlue, &QAction::triggered, this, &SignatureDesigner::setColorBlue);
    connect(ui->actionYellow, &QAction::triggered, this, &SignatureDesigner::setColorYellow);
    connect(ui->actionImport, &QAction::triggered, this, &SignatureDesigner::importPicture);
    connect(ui->actionClear_Picture, &QAction::triggered, this, &SignatureDesigner::clearPicture);
    connect(ui->actionClear_Curve, &QAction::triggered, this, &SignatureDesigner::clearCurve);
    connect(ui->actionClear_All, &QAction::triggered, this, &SignatureDesigner::clearAll);
    connect(ui->actionClose_Curve, &QAction::triggered, this, &SignatureDesigner::closedCurve);
    connect(ui->actionOpen_Curve, &QAction::triggered, this, &SignatureDesigner::openCurve);
    connect(ui->actionParallel_Contour, &QAction::triggered, this, &SignatureDesigner::setParallel);
    connect(ui->actionSet_Offset, &QAction::triggered, this, &SignatureDesigner::onSetOffsetClicked);
    connect(ui->actionZigzag, &QAction::triggered, this, &SignatureDesigner::setZigzag);
    connect(ui->actionSet_Path, &QAction::triggered, this, &SignatureDesigner::onSetFromFilePathClicked);

    this->resize(QDesktopWidget().availableGeometry(this).size() * 0.7);
}

SignatureDesigner::~SignatureDesigner() 
{
    // 析构函数
    delete  ui;
    m_ctrlPoints.clear();
}

void SignatureDesigner::mousePressEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton) {   // 单击鼠标左键创建控制点
        m_ctrlPoints.push_back(event->pos());
    }
    else if (event->buttons() == Qt::RightButton) { // 单击鼠标右键删除与之最近的控制点
        if (!m_ctrlPoints.empty()) {
            curIdx = find_closest(m_ctrlPoints, event->pos());
            m_ctrlPoints.erase(m_ctrlPoints.begin() + curIdx);
        }
    }
    else if (event->buttons() == Qt::MiddleButton) {    // 单机鼠标中键拖动与之最近的控制点
        if (!m_ctrlPoints.empty()) {
            curIdx = find_closest(m_ctrlPoints, event->pos());
            m_dragging = true;
            m_dragStart = m_ctrlPoints[curIdx];
        }
    }
    update();
}

qint16 SignatureDesigner::find_closest(QVector<QPointF> Points, QPointF p) {
    // 根据当前点p和之前选中的点Points，找p和Points中最近的点的下标
    qreal dis = 0;
    std::vector<double> distance;
    for (int i = 0; i < Points.size(); i++) {
        dis = pow(Points[i].x() - p.x(), 2) + pow(Points[i].y() - p.y(), 2);
        distance.push_back(dis);
    }
    return std::min_element(distance.begin(), distance.end()) - distance.begin();
}

void SignatureDesigner::mouseReleaseEvent(QMouseEvent* event) 
{
    if (event->button() == Qt::MiddleButton) {
        m_dragging = false;
    }
}

void SignatureDesigner::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        QPointF delta = event->pos() - m_dragStart;
        m_ctrlPoints[curIdx] += delta;
        m_dragStart = event->pos();
        update();
    }
}

void SignatureDesigner::paintEvent(QPaintEvent* event)
{
    if (mode == 1) {    // 签名设计
        drawSpline();
    }
    else if (mode == 2) {   // 手动抠图，提取边界
        QPainter painter;
        painter.begin(this);
        QImage image(fromFilename);
        QRect rect(this->geometry().x(), 
            this->geometry().y() - 70, image.width(), image.height());
        painter.drawImage(rect, image);
        painter.end();
        drawSpline();
    }
    else if (mode == 3) {
        switch (fillMode){
            case 1:
                drawParallelContour(-offset);
                break;
            case 2:
                drawZigzag(offset);
                break;
            default:
                break;
        }
    }
}

void SignatureDesigner::drawSpline()    // 绘图函数
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿
    m_curvePoints.clear();

    if (!isOpen) {  // 闭曲线
        for (qreal u = order; u < m_ctrlPoints.size() + order; u += 0.001) {
            QPointF point(0.0, 0.0);
            for (int i = 0; i < m_ctrlPoints.size() + order; i++) { // ++i
                QPointF points = m_ctrlPoints[i % m_ctrlPoints.size()];
                points *= getBSpline(order, i, u);
                point += points;
            }
            m_curvePoints.push_back(point);
        }
    }
    else {
        for (qreal u = order; u < m_ctrlPoints.size(); u += 0.01) {
            QPointF point(0.0, 0.0);
            for (int i = 0; i < m_ctrlPoints.size(); i++) { // ++i
                QPointF points = m_ctrlPoints[i];
                points *= getBSpline(order, i, u);
                point += points;
            }
            m_curvePoints.push_back(point);
        }
    }
    // 绘制控制点
    QPen ctrlPen1(QColor(0, 0, 255));
    ctrlPen1.setWidth(5);
    painter.setPen(ctrlPen1);
    for (int i = 0; i < m_ctrlPoints.size(); i++) { // ++i
        painter.drawPoint(m_ctrlPoints[i]);
    }
    // 用虚线绘制控制线段
    QPen ctrlPen2(QColor(255, 0, 0));
    ctrlPen2.setWidth(2);
    ctrlPen2.setStyle(Qt::DashDotDotLine);
    painter.setPen(ctrlPen2);
    for (int i = 0; i < m_ctrlPoints.size() - 1; i++) { // ++i
        painter.drawLine(m_ctrlPoints[i], m_ctrlPoints[i + 1]);
    }

    if ((!isOpen) && m_ctrlPoints.size() > 2) {
        painter.drawLine(m_ctrlPoints[m_ctrlPoints.size() - 1], m_ctrlPoints[0]);
    }

    // 绘制样条曲线
    QPen curvePen(color);
    curvePen.setWidth(width);
    painter.setPen(curvePen);
    for (int i = 0; i < m_curvePoints.size() - 1; i++) {
        painter.drawLine(m_curvePoints[i], m_curvePoints[i + 1]);
    }
}

qreal SignatureDesigner::getBSpline(int order, int i, qreal u)
{
    switch (order) {
        case 1:
            return BSplineOrder1(i, u);
        case 2:
            return BSplineOrder2(i, u);
        case 3:
            return BSplineOrder3(i, u);
        default:
            break;
    }
    return 0;
}

qreal SignatureDesigner::BSplineOrder1(int i, qreal u)
{
    qreal t = u - i;
    if (0 <= t && t < 1) {
        return t;
    }
    else if (1 <= t && t < 2) {
        return 2 - t;
    }
    return 0;
}

qreal SignatureDesigner::BSplineOrder2(int i, qreal u)
{
    qreal t = u - i;
    if (0 <= t && t < 1) {
        return 0.5 * t * t;
    }
    else if (1 <= t && t < 2) {
        return 3 * t - t * t - 1.5;
    }
    else if (2 <= t && t < 3) {
        return 0.5 * pow(3 - t, 2);
    }
    return 0;
}

qreal SignatureDesigner::BSplineOrder3(int i, qreal u)
{
    qreal t = u - i;
    qreal a = 1.0 / 6.0;
    if (0 <= t && t < 1) {
        return a * t * t * t;
    }
    else if (1 <= t && t < 2) {
        return a * (-3 * pow(t - 1, 3) + 3 * pow(t - 1, 2) + 3 * (t - 1) + 1);
    }
    else if (2 <= t && t < 3) {
        return a * (3 * pow(t - 2, 3) - 6 * pow(t - 2, 2) + 4);
    }
    else if (3 <= t && t < 4) {
        return a * pow(4 - t, 3);
    }
    return 0;
}

QPolygonF SignatureDesigner::PathsDToQPolygonF(Clipper2Lib::PathsD paths) 
{   // 将Paths64对象转成QPolygonF对象
    QPolygonF polygon; 
    for (const auto& path : paths) { 
        for (const auto& pt : path) { 
            polygon.append(QPointF(pt.x, pt.y));
        } 
    } return polygon; 
}

Clipper2Lib::PathsD SignatureDesigner::initialParallelContour()
{
    using namespace Clipper2Lib;
    PathsD polygon;
    std::vector<double> path;

    for (int i = 0; i < m_curvePoints.size(); i++) {
        path.push_back(m_curvePoints[i].x());
        path.push_back(m_curvePoints[i].y());
    }

    polygon.push_back(MakePathD(path));
    return polygon;
}

void SignatureDesigner::drawParallelContour(double delta)
{
    using namespace Clipper2Lib;
    QPainter p(this);
    QPen pen(color);
    pen.setWidth(width);
    p.setPen(pen); // 设置画笔
    p.setBrush(Qt::NoBrush); // 设置画刷颜色

    PathsD contour = initialParallelContour();
    QPolygonF polygon = PathsDToQPolygonF(contour);
    p.drawPolygon(polygon); // 使用QPainter对象的drawPolygon函数，绘制一个多边形

    int i = 0;
    int stop = stopCondition(-delta);
    while (i < stop) {
        contour = InflatePaths(contour, delta, JoinType::Round, EndType::Polygon);
        polygon = PathsDToQPolygonF(contour);
        p.drawPolygon(polygon);
        i++;
    }
}

int SignatureDesigner::stopCondition(double delta)
{
    QSize size = QDesktopWidget().availableGeometry(this).size() * 0.7;
    return int(std::max(size.height() / delta, size.width() / delta));
}

void SignatureDesigner::drawZigzag(double delta)
{   
    QVector<QPointF> polygon = preparePolygon();
    double yMin, yMax;
    int maxIdx = scanRegion(polygon, yMin, yMax);
    int lineNum = floor((yMax - yMin) / delta); // 扫描线的数目
    using namespace std;
    vector<vector<double> > vertex(lineNum, vector<double>(2));    // 存横坐标
    
    int n = polygon.size();
    double eps = 0.5;
    int scanNum = 0;
    bool flag = true;
    for (int i = 0; i < n; i++) {
        if (flag) {
            if ((fabs(scanNum * delta + yMin - polygon[i].y()) < eps) && (vertex[scanNum][0]) < eps) {
                vertex[scanNum][0] = polygon[i].x();
                if (scanNum < lineNum - 1)  scanNum++;
            }
        }
        else {
            if ((fabs(scanNum * delta + yMin - polygon[i].y()) < eps) && (vertex[scanNum][1] < eps)) {
                vertex[scanNum][1] = polygon[i].x();
                if (scanNum > 1)    scanNum--;
            }
        }
        if (scanNum == lineNum - 1) {
            flag = false;
        }
    }

    QPainter p(this);
    QPen pen(color);
    pen.setWidth(width);
    p.setPen(pen);
    for (int i = 1; i < lineNum - 1; i++) {
        if (vertex[i].size() > 1) {
            p.drawLine(QPointF(vertex[i][0], yMin + i * delta), QPointF(vertex[i][1], yMin + i * delta));
        }
    }
    for (int i = 1; i < lineNum - 2; i++) {
        if (i % 2) {
            p.drawLine(QPointF(vertex[i][0], yMin + i * delta), 
                QPointF(vertex[i + 1][0], yMin + (i + 1) * delta));
        }
        else {
            p.drawLine(QPointF(vertex[i][1], yMin + i * delta),
                QPointF(vertex[i + 1][1], yMin + (i + 1) * delta));
        }
    }
}

bool SignatureDesigner::isClockwise(QVector<QPointF> polygon)
{   // 判断封闭多边形polygon是否按照顺时针索引
    double area = 0.0;
    int num = polygon.size();
    for (int i = 0; i < num + 1; i++) {
        area += polygon[i % num].x() * polygon[(i + 1) % num].y() 
            - polygon[i % num].y() * polygon[(i + 1) % num].x();
    }
    return (area < 0);
}

int SignatureDesigner::scanRegion(QVector<QPointF> polygon, double& min, double& max)
{
    int maxIdx = 0;
    min = INFINITY;
    max = -INFINITY;
    for (int i = 0; i < polygon.size(); i++) {
        if (polygon[i].y() < min) {
            min = polygon[i].y();
        }
        if (polygon[i].y() > max) {
            max = polygon[i].y();
            maxIdx = i;
        }
    }
    return maxIdx;
}

int SignatureDesigner::findMinIdx(QVector<QPointF> polygon)
{
    int idx = 0;
    for (int i = 0; i < polygon.size(); i++) {
        if (polygon[i].y() < polygon[idx].y()) {
            idx = i;
        }
    }
    return idx;
}

QVector<QPointF> SignatureDesigner::preparePolygon()
{
    QVector<QPointF> polygon = m_curvePoints;
    if (!isClockwise(polygon)) {
        std::reverse(polygon.begin(), polygon.end());
    }
    int idx = findMinIdx(polygon);
    QVector<QPointF> prepared;
    for (int i = idx; i < idx + polygon.size(); i++) {
        prepared.push_back(polygon[i % polygon.size()]);
    }
    return prepared;
}

void SignatureDesigner::newFile()
{   // 新建文件
    mode = 1;
    m_ctrlPoints.clear();
    update();
}

void SignatureDesigner::saveFile() 
{   // 保存文件
    QPainter recorder(&pixmap);
    QPen curvePen(QColor(0, 0, 0));
    curvePen.setWidth(5);
    recorder.setPen(curvePen);
    for (int i = 0; i < m_curvePoints.size() - 1; i++) {
        recorder.drawLine(m_curvePoints[i], m_curvePoints[i + 1]);
    }
    pixmap.save(toFilename, "PNG");
}

void SignatureDesigner::exitFile()
{
    close();
}

void SignatureDesigner::redoFile() 
{
    mode = 1;
    m_ctrlPoints.clear();
    update();
}

void SignatureDesigner::setOrder1()
{
    order = 1;
    update();
}

void SignatureDesigner::setOrder2()
{
    order = 2;
    update();
}

void SignatureDesigner::setOrder3()
{
    order = 3;
    update();
}

void SignatureDesigner::setWidth1()
{
    width = 1;
    update();
}

void SignatureDesigner::setWidth3()
{
    width = 3;
    update();
}

void SignatureDesigner::setWidth5()
{
    width = 5;
    update();
}

void SignatureDesigner::setWidth7()
{
    width = 7;
    update();
}

void SignatureDesigner::setColorBlack()
{
    color = { 0, 0, 0 };
    update();
}

void SignatureDesigner::setColorRed()
{
    color = { 255, 0, 0 };
    update();
}

void SignatureDesigner::setColorBlue()
{
    color = { 0, 0, 255 };
    update();
}

void SignatureDesigner::setColorYellow()
{
    color = { 255, 255, 0 };
    update();
}

void SignatureDesigner::importPicture()
{
    mode = 2;
    update();
}

void SignatureDesigner::clearPicture()
{
    mode = 1;
    update();
}

void SignatureDesigner::clearCurve()
{
    m_ctrlPoints.clear();
    update();
}

void SignatureDesigner::clearAll()
{
    m_ctrlPoints.clear();
    mode = 1;
    update();
}

void SignatureDesigner::closedCurve()
{
    isOpen = false;
    update();
}

void SignatureDesigner::openCurve()
{
    isOpen = true;
    update();
}

void SignatureDesigner::setParallel()
{
    mode = 3;
    fillMode = 1;
    update();
}

void SignatureDesigner::onSetOffsetClicked()
{
    bool bOk = false;
    offset = QInputDialog::getDouble(this,
        "Set Offset: ",
        "Please input the offset: ",
        10.0, 7.0, 25.0, 1,
        &bOk);
    update();
}

void SignatureDesigner::setZigzag()
{
    mode = 3;
    fillMode = 2;
    update();
}

void SignatureDesigner::onSetFromFilePathClicked()
{
    bool bOk = false;
    QString sName = QInputDialog::getText(this,
        "Import image (PNG): ",
        "Please input the image path",
        QLineEdit::Normal,
        fromFilename,
        &bOk
    );
    fromFilename = sName;
    update();
}