#ifndef NLSIMPLEGUIWINDOW_H
#define NLSIMPLEGUIWINDOW_H

#include <string>
#include <QtGui/QMainWindow>

class NLSimple;
class TPaveText;
class ConsentrationGraph;
class QButtonGroup;

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

private:
    Ui::NLSimpleGuiWindow *ui;
    NLSimple *m_model;
    bool m_ownsModel;
    std::string m_fileName;
    TPaveText *m_equationPt;
    QButtonGroup *m_clarkeButtonGroup;

private slots:
    void openExistingModel();
    void openNewModel();
    void saveModel();
    void saveModelAs();
    void quit();

    void drawModel();
    void drawEquations();
    void doGeneticOptimization();
    void doMinuit2Fit();
    void addCgmsData();
    void addCarbData();
    void addMeterData();
    void addCustomEventData();
    void refreshPredictions();
    void updateDelayAndError();
    void zoomModelPreviewPlus();
    void zoomModelPreviewMinus();
    void zoomModelPreviewPlus( double factor ); //amount = fraction of current width
    void cleanupClarkAnalysis();
    void refreshClarkAnalysis();
    void drawClarkAnalysis( const ConsentrationGraph &xGraph,
                            const ConsentrationGraph &yGraph,
                            bool isMeterVCgms );
};

#endif // NLSIMPLEGUIWINDOW_H
