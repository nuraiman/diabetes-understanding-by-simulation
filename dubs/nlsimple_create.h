#ifndef NLSIMPLE_CREATE_H
#define NLSIMPLE_CREATE_H

#include <QtGui/QDialog>

//Forward Declarations
class NLSimple;
class ConsentrationGraph;


namespace Ui {
    class NlSimpleCreate;
}

class NlSimpleCreate : public QDialog {
    Q_OBJECT
public:
    NlSimpleCreate( NLSimple *&model, QString *fileName = NULL, QWidget *parent = 0 );
    ~NlSimpleCreate();

protected:
    void changeEvent(QEvent *e);
    void init();

private:
    Ui::NlSimpleCreate *m_ui;
    NLSimple *&m_model;
    QString *m_fileName;

    bool m_userSetTime;
    ConsentrationGraph *m_cgmsData;
    ConsentrationGraph *m_bolusData;
    ConsentrationGraph *m_insulinData; //created from m_bolusData
    ConsentrationGraph *m_carbConsumptionData;
    ConsentrationGraph *m_meterData;
    ConsentrationGraph *m_customData;
    //ConsentrationGraph *m_ExcersizeData;

    double m_minutesGraphPerPage;
    bool m_useKgs;

    enum GraphPad
    {
      kCGMS_PAD,
      kBOLUS_PAD,
      kCARB_PAD,
      kMETER_PAD,
      kCustom_PAD,
      kEXCERSIZE_PAD,
      kALL_PAD,
      kNUM_PAD
    };//enum GraphCanvas

    void findTimeLimits();
    void drawPreview( GraphPad pad );
    void updateModelGraphSize();
    void updateModelGraphSize(int tabNumber);


private slots:
    void addCgmsData();
    void addBolusData();
    void addCarbData();
    void addMeterData();
    void addCustomData();
    void constructModel();
    void openDefinedModel();
    void cancel();
    void handleTimeLimitButton();
    void enableCreateButton();
    void changeMassUnits();
    void zoomInX();
    void zoomOutX();
    void checkDisplayTimeLimitsConsistency();
};

#endif // NLSIMPLE_CREATE_H
