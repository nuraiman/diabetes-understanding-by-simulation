#ifndef PROGRAMOPTIONSGUI_HH
#define PROGRAMOPTIONSGUI_HH

#include <QtGui/QFrame>

class ModelSettings;
namespace Ui { class ProgramOptionsGui; }

namespace boost{ namespace posix_time{ class ptime; } }

class ProgramOptionsGui : public QFrame {
    Q_OBJECT
public:
    ProgramOptionsGui( ModelSettings *settings, QWidget *parent = 0);
    ~ProgramOptionsGui();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ProgramOptionsGui *m_ui;
    ModelSettings *m_settings;
    bool m_useKgs;

private slots:
    void changeMassUnits();
    void getValuesFromGui();

public slots:
    void setTrainingTimeLimits( boost::posix_time::ptime startTime,
                                boost::posix_time::ptime endTime,
                                bool setToLimits );

 signals:
     void valueChanged(int whichValue);
};

#endif // PROGRAMOPTIONSGUI_HH
