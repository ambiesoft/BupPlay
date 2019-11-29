#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

class QSlider;

// https://stackoverflow.com/questions/11132597/qslider-mouse-direct-jump
class ClickableSlider : public QSlider
{
    QMediaPlayer *m_player = nullptr;
public:
    explicit ClickableSlider(QMediaPlayer *player,
                      Qt::Orientation orientation,
                      QWidget *parent = nullptr) :
        QSlider(orientation,parent),
        m_player(player) {}
protected:
  void mousePressEvent ( QMouseEvent * event );
};

#endif // CLICKABLESLIDER_H
