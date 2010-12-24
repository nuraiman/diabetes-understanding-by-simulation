#ifndef NLSIMPLEGUIWINDOW_H
#define NLSIMPLEGUIWINDOW_H

#include <string>
#include <QtGui/QMainWindow>

class NLSimple;
class TPaveText;
class ConsentrationGraph;
class QButtonGroup;
class TCanvas;
class ProgramOptionsGui;
class QStandardItemModel;

namespace Ui
{
    class NLSimpleGuiWindow;
}

class NLSimpleGuiWindow : public QMainWindow
{
    Q_OBJECT

public:
    NLSimpleGuiWindow( NLSimple *model = NULL, QWidget *parent = 0);
    ~NLSimpleGuiWindow();

 protected:
     void init();
     TCanvas *getModelCanvas();
     int getIdOfSelectedCustomEvent();

private:
    Ui::NLSimpleGuiWindow *ui;
    NLSimple *m_model;
    bool m_ownsModel;
    std::string m_fileName;
    TPaveText *m_equationPt;
    QButtonGroup *m_clarkeButtonGroup;
    ProgramOptionsGui *m_programOptionsGui;
    QStandardItemModel *m_customEventList;

private slots:
    void openExistingModel();
    void openNewModel();
    void saveModel();
    void saveModelAs();
    void quit();

    void drawModel();
    void drawEquations();
    void updateCustomEventTab();
    void doGeneticOptimization();
    void doMinuit2Fit();
    void addCgmsData();
    void addCarbData();
    void addMeterData();
    void addCustomEventData();
    void refreshPredictions();
    void zoomModelPreviewPlus();
    void zoomModelPreviewMinus();
    void zoomModelPreview( double factor ); //amount = fraction of current width
    void setMinMaxDisplayLimits();
    void checkDisplayTimeLimitsConsistency();
    void cleanupClarkAnalysis();
    void refreshClarkAnalysis();
    void drawClarkAnalysis( const ConsentrationGraph &xGraph,
                            const ConsentrationGraph &yGraph,
                            bool isMeterVCgms );
    void addCustomEventDef();
    void deleteCustomEventDef();
    void drawSelectedCustomEvent();

    friend class NLSimple;
};

#endif // NLSIMPLEGUIWINDOW_H
