#ifndef WIDGET_H
#define WIDGET_H

#include "ui_SignatureDesigner.h"
#include <QWidget>
#include <QVector>
#include <QPainter>
#include <clipper2/clipper.h>

class SignatureDesigner : public QMainWindow
{
    Q_OBJECT

public:
    SignatureDesigner(QWidget* parent = nullptr);   // 构造函数
    ~SignatureDesigner();   // 析构函数

public:
    QPixmap pixmap;
    QString toFilename;
    QString fromFilename;
    qint16 order;
    qint16 width;
    QColor color;
    qint16 mode;
    bool isOpen;
    double offset;
    qint16 fillMode;

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual qint16 find_closest(QVector<QPointF> Points, QPointF p);

private slots:
    void newFile();
    void saveFile();
    void exitFile();
    void redoFile();
    void setOrder1();
    void setOrder2();
    void setOrder3();
    void setWidth1();
    void setWidth3();
    void setWidth5();
    void setWidth7();
    void setColorBlack();
    void setColorRed();
    void setColorBlue();
    void setColorYellow();
    void importPicture();
    void clearPicture();
    void clearCurve();
    void clearAll();
    void closedCurve();
    void openCurve();
    void setParallel();
    void onSetOffsetClicked();
    void setZigzag();
    void onSetFromFilePathClicked();

private:
    void drawSpline();
    bool m_dragging = false;
    QPointF m_dragStart;
    qint16 curIdx = -1;  // 记录当前操作的点在m_ctrlPoints中的下标
    qreal getBSpline(int order, int i, qreal u);
    qreal BSplineOrder1(int i, qreal u);
    qreal BSplineOrder2(int i, qreal u);
    qreal BSplineOrder3(int i, qreal u);
    QPolygonF PathsDToQPolygonF(Clipper2Lib::PathsD paths);
    Clipper2Lib::PathsD initialParallelContour();
    void drawParallelContour(double delta);
    int stopCondition(double delta);
    void drawZigzag(double delta);
    bool isClockwise(QVector<QPointF> polygon);
    int scanRegion(QVector<QPointF> polygon, double& min, double& max);
    QVector<QPointF> preparePolygon();
    int findMinIdx(QVector<QPointF> polygon);

private:
    QVector<QPointF> m_ctrlPoints;       // 控制点
    QVector<QPointF> m_curvePoints;      // 曲线上的点
    Ui::SignatureDesigner* ui;
};
#endif // WIDGET_H
