#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class Application : public QApplication
{
public:
    Application(int argc, char *argv[]):
        QApplication(argc, argv) {}

//    bool notify(QObject *object, QEvent *event) override
//    {
//        return QApplication::notify(object,event);
//    }
};

#endif // APPLICATION_H
