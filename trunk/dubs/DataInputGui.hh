#ifndef DATAINPUTGUI_HH
#define DATAINPUTGUI_HH

#include <QtGui/QDialog>
#include "boost/date_time/posix_time/posix_time.hpp"

class ConsentrationGraph;

namespace Ui {
    class DataInputGui;
}

//Returns a '1' if you added points, a '0' if you clicked cancel
class DataInputGui : public QDialog {
    Q_OBJECT
public:
    DataInputGui(ConsentrationGraph *graph,
                  QString message,
                  int type,
                  QWidget *parent = 0 );


    ~DataInputGui();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::DataInputGui *m_ui;
    ConsentrationGraph *m_graph;
    int m_type;

private slots:
    void cancelOperation();
    void readFromFile();
    void readSingleInputCloseWindow();
};

#endif // DATAINPUTGUI_HH
