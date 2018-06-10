#include "QLCDNumberEditable.h"

QLCDNumberEditable::QLCDNumberEditable(QObject *parent) :
    QLCDNumber(),
    mDigitIdToChange(-1)
{
    setFocusPolicy(Qt::ClickFocus);
}

void QLCDNumberEditable::setDigitAreaPressed( QMouseEvent * e, const int areaId )
{
    int ndigits = this->digitCount();
    int digitSpace = this->smallDecimalPoint() ? 2 : 1;
    int xSegLen = this->width()*5/(ndigits*(5 + digitSpace) + digitSpace);
    int ySegLen = this->height()*5/12;
    int segLen = ySegLen > xSegLen ? xSegLen : ySegLen;
    int xAdvance = segLen*( 5 + digitSpace )/5;
    int xOffset = ( this->width() - ndigits*xAdvance + segLen/5 )/2;

    /* Use the predefined Id if necessary */
    if ( areaId != -1 )
    {
        this->mDigitRect.setRect(xOffset + xAdvance*areaId, this->y(), xAdvance, this->height() );
        return;
    }

    int xPos = e->pos().x();
    int itemId = -1;

    for ( int i = 0; i < ndigits; i++ )
    {
        if ((xPos >= xOffset + xAdvance*i) && (xPos < xOffset + xAdvance*(i + 1)))
        {
            itemId = i;
            this->mDigitRect.setRect(xOffset + xAdvance*itemId, this->y(), xAdvance, this->height() );
            break;
        }
    }

    this->mDigitIdToChange = itemId;
}

void QLCDNumberEditable::mousePressEvent ( QMouseEvent * e )
{
    this->setDigitAreaPressed(e);
    update();
}

void QLCDNumberEditable::PaintEvent(QPaintEvent * e)
{
    if (!( this->mDigitIdToChange == -1 ))
    {
        QPainter Painter(this);
        Painter.setBrush(QBrush(Qt::black, Qt::Dense7Pattern));
        Painter.drawRect(this->mDigitRect);
    }
    QLCDNumber::paintEvent(e);
}

void QLCDNumberEditable::keyPressEvent ( QKeyEvent * event )
{
    int key = event->key();

/* Filter out all non digit keys */
    if (( key >= Qt::Key_0 ) && ( key <= Qt::Key_9 ))
    {
        if ( this->mDigitIdToChange != -1 )
        {
            /* Convert double value to a string representation */
            QString lcdStr;
            lcdStr.setNum(int(this->value()));

            /* Insert starting zeros in lcdStr representation */
            int l = this->digitCount() - lcdStr.length();
            for (int i = 0; i < l; i++)
            {
                lcdStr.insert(0,"0");
            }

            /* Convert key value to string representation */
            QString keyVal;
            keyVal.setNum( key - Qt::Key_0 );

            /* Replace selected digit with the desired one */
            lcdStr.replace(this->mDigitIdToChange, 1, keyVal);

            /* Update lcdnumber */
            this->display(lcdStr.toDouble());

            /* Switch to the next digit item */
            if ( this->mDigitIdToChange == this->digitCount() - 1 )
            {
                this->mDigitIdToChange = 0;
            }
            else
            {
                this->mDigitIdToChange++;
            }

            this->setDigitAreaPressed(NULL, this->mDigitIdToChange);
            update();
        }
    }

    /* Enter + Esc press event handler */
    if (( key == Qt::Key_Return ) || ( key == Qt::Key_Escape ))
    {
        this->mDigitIdToChange = -1;
        update();
    }
}

void QLCDNumberEditable::pheelEvent ( QWheelEvent * event )
{
    if ( this->mDigitIdToChange != -1 )
    {
        /* Number of steps within a single event = delta() / (15*8) */
        this->positionValueIncrement(event->delta()/(15*8));
    }
}

void QLCDNumberEditable::positionValueIncrement( const int numSteps )
{
    qreal multiplyer = qPow( 10, this->digitCount() - 1 - this->mDigitIdToChange );
    this->display(this->value() + numSteps*multiplyer );
}
