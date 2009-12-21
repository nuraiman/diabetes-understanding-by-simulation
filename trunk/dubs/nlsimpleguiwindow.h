#ifndef NLSIMPLEGUIWINDOW_H
#define NLSIMPLEGUIWINDOW_H

#include <QtGui/QMainWindow>

namespace Ui
{
    class NLSimpleGuiWindow;
}

class NLSimpleGuiWindow : public QMainWindow
{
    Q_OBJECT

public:
    NLSimpleGuiWindow(QWidget *parent = 0);
    ~NLSimpleGuiWindow();

 protected:
     void init();

private:
    Ui::NLSimpleGuiWindow *ui;
};

#endif // NLSIMPLEGUIWINDOW_H
