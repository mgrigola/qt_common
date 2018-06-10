#ifndef QLCDNUMBEREDITABLE_H
#define QLCDNUMBEREDITABLE_H

#include <QLCDNumber>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QRectF>
#include <QWheelEvent>

#include <QMouseEvent>
#include <QBrush>
#include <QPainter>
#include <QString>
#include <QtCore/qmath.h>

class QLCDNumberEditable : public QLCDNumber
{
Q_OBJECT
public:
    QLCDNumberEditable(QObject *parent = 0);

protected:
    void mousePressEvent(QMouseEvent* mEvent);
    void keyPressEvent(QKeyEvent* kEvent);
    void paintEvent(QPaintEvent* pEvent);
    void wheelEvent(QWheelEvent* wEvent);

private:
    void setDigitAreaPressed( QMouseEvent * e, const int areaId = -1 );
    void positionValueIncrement( const int numSteps );
    QRectF mDigitRect;
    int mDigitIdToChange;


};

#endif // QLCDNUMBEREDITABLE_H


