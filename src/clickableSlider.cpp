
#include "clickableSlider.h"

#include <QSlider>


void ClickableSlider::mousePressEvent ( QMouseEvent * event )
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    if (event->button() == Qt::LeftButton &&
            sr.contains(event->pos()) == false)
    {
        int newVal;
        if (orientation() == Qt::Vertical)
            newVal = minimum() + ((maximum()-minimum()) * (height()-event->y())) / height();
        else
            newVal = minimum() + ((maximum()-minimum()) * event->x()) / width();

        if (invertedAppearance() == true)
        {
            // setValue( maximum() - newVal );
            m_player->setPosition((maximum() - newVal) * 1000);
        }
        else
        {
            // setValue(newVal);
            m_player->setPosition((newVal) * 1000);
        }
        event->accept();
    }
    QSlider::mousePressEvent(event);
}

