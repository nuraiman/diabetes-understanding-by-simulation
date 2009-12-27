#include "nlsimpleguiwindow.h"
#include "ui_nlsimpleguiwindow.h"
#include "ProgramOptionsGui.hh"

#include "MiscGuiUtils.hh"
#include "ResponseModel.hh"
#include "nlsimple_create.h"
#include "DataInputGui.hh"
#include "CgmsDataImport.hh"
#include "ArtificialPancrease.hh"

#include <QApplication>
#include <QMenu>
#include <QString>
#include <QFileDialog>
#include <QButtonGroup>
#include <QRadioButton>
#include <QPushButton>
#include <QScrollBar>

#include <string>
#include <vector>

#include "TH1F.h"
#include "TPaveText.h"
#include "TLegend.h"


using namespace std;

NLSimpleGuiWindow::NLSimpleGuiWindow( NLSimple *model, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::NLSimpleGuiWindow), m_model(model),
      m_ownsModel(model==NULL), m_fileName(""), m_equationPt(NULL),
      m_programOptionsGui(NULL)
{
  ui->setupUi(this);
  if( !m_model ) openNewModel();
  if( !m_model ) quit();

  QMenu *fileMenu = ui->menuBar->addMenu( "&File" );
  QAction *newAction = fileMenu->addAction( "&new model" );
  QAction *openAction = fileMenu->addAction( "&open file" );
  QAction *saveAction = fileMenu->addAction( "&save" );
  QAction *saveAsAction = fileMenu->addAction( "save &as" );
  QAction *quitAction = fileMenu->addAction( "&quit" );

  connect( newAction, SIGNAL(triggered()), this, SLOT(openNewModel()) );
  connect( openAction, SIGNAL(triggered()), this, SLOT(openExistingModel()) );
  connect( saveAction, SIGNAL(triggered()), this, SLOT(saveModel()) );
  connect( saveAsAction, SIGNAL(triggered()), this, SLOT(saveModelAs()) );
  connect( quitAction, SIGNAL(triggered()), this, SLOT(quit()) );


  TCanvas *can = ui->m_mathFormulaWidget->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );
  m_equationPt = new TPaveText(0, 0, 1.0, 1.0, "NDC");
  m_equationPt->SetBorderSize(0);
  m_equationPt->SetTextAlign(12);
  m_equationPt->Draw();
  can->SetEditable( kFALSE );
  can->Update();


  //Add a button group so CLarke Error Grid buttons are mutually exclusive
  m_clarkeButtonGroup = new QButtonGroup( this );
  m_clarkeButtonGroup->addButton( ui->clarkCgmsVMeterRadioButton, 0 );
  m_clarkeButtonGroup->addButton( ui->clarkePredVCGMSRadioButton, 1 );
  ui->clarkCgmsVMeterRadioButton->setChecked(true);
  connect( m_clarkeButtonGroup, SIGNAL(buttonClicked(int)),
          this, SLOT(refreshClarkAnalysis()) );

  //Make so Clarke Grid displays nice
  can = ui->clarkeErrorGridWidget->GetCanvas();
  can->cd();
  can->Range(-45.17185,-46.4891,410.4746,410.6538);
  can->SetFillColor(0);
  can->SetBorderMode(0);
  can->SetBorderSize(2);
  can->SetRightMargin(0.031);
  can->SetTopMargin(0.024);
  can->SetFrameBorderMode(0);

  QTabWidget *tw = ui->tabWidget;
  tw->setTabText( tw->indexOf(ui->modelDisplyTab), "Display"    );
  tw->setTabText( tw->indexOf(ui->optionsTab),     "Options"    );
  tw->setTabText( tw->indexOf(ui->clarkeGridTab),  "Error Grid" );

  ui->endDisplayTime->setCalendarPopup(true);
  ui->startDisplayTime->setCalendarPopup(true);
  // ui->endDisplayTime->setDisplayFormat("MMM dd yy hh:mm AP");
  // ui->startDisplayTime->setDisplayFormat("MMM dd yy hh:mm AP");

  connect( ui->geneticOptimizeButton, SIGNAL(clicked()), this, SLOT(doGeneticOptimization()) );
  connect( ui->baysianFineTuneButton, SIGNAL(clicked()), this, SLOT(doMinuit2Fit())          );
  connect( ui->addCgmsButton,         SIGNAL(clicked()), this, SLOT(addCgmsData())           );
  connect( ui->addMealDataButton,     SIGNAL(clicked()), this, SLOT(addCarbData())           );
  connect( ui->addMeterDataButton,    SIGNAL(clicked()), this, SLOT(addMeterData())          );
  connect( ui->addCustonDataBustom,   SIGNAL(clicked()), this, SLOT(addCustomEventData())    );
  connect( ui->redrawButton,          SIGNAL(clicked()), this, SLOT(drawModel())             );
  connect( ui->zoomIn,                SIGNAL(clicked()), this, SLOT(zoomModelPreviewPlus())  );
  connect( ui->zoomOut,               SIGNAL(clicked()), this, SLOT(zoomModelPreviewMinus()) );
  connect( ui->zoomIn,                SIGNAL(clicked()), this, SLOT(zoomModelPreviewPlus())  );
  connect( ui->endDisplayTime,        SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(checkDisplayTimeLimitsConsistency()) );
  connect( ui->startDisplayTime,      SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(checkDisplayTimeLimitsConsistency()) );
  connect( ui->endDisplayTime,        SIGNAL(editingFinished()), this, SLOT(drawModel())    );
  connect( ui->startDisplayTime,      SIGNAL(editingFinished()), this, SLOT(drawModel())    );

  init();
}//NLSimpleGuiWindow constructor



void NLSimpleGuiWindow::init()
{
  if( !m_model ) return;

  m_model->m_gui = this;

  ui->tabWidget->setCurrentWidget(ui->optionsTab);

  if(m_programOptionsGui) delete m_programOptionsGui; // will probably cause a seg fault..
  m_programOptionsGui = new ProgramOptionsGui( &(m_model->m_settings) , ui->optionsTab);
  QLayout *layout = new QVBoxLayout( ui->optionsTab );
  layout->addWidget( m_programOptionsGui );
  m_programOptionsGui->setLayout( layout );


  ui->endDisplayTime->setDateTime( posixTimeToQTime(kGenericT0) );
  ui->startDisplayTime->setDateTime( posixTimeToQTime(kGenericT0) );
  setMinMaxDisplayLimits();

  drawModel();
  drawEquations();
  refreshClarkAnalysis();

  ui->tabWidget->setCurrentWidget(ui->modelDisplyTab);
}//void NLSimpleGuiWindow::init()




NLSimpleGuiWindow::~NLSimpleGuiWindow()
{
  delete ui;
  if( m_model && m_ownsModel ) delete m_model;
}//NLSimpleGuiWindow::~NLSimpleGuiWindow()



TCanvas *NLSimpleGuiWindow::getModelCanvas()
{
  ui->tabWidget->setCurrentWidget(ui->modelDisplyTab);
  TCanvas *can = ui->m_modelDisplayWidget->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );

  return can;
}//TVirtualPad *NLSimpleGuiWindow::getModelCanvas()



void NLSimpleGuiWindow::drawModel()
{
  TCanvas *can = getModelCanvas();
  cleanCanvas( can, "TFrame" ); //lets ovoid memmory clutter

  PosixTime endTime = qtimeToPosixTime( ui->endDisplayTime->dateTime() );
  PosixTime startTime = qtimeToPosixTime( ui->startDisplayTime->dateTime() );

  TimeDuration deltaGeneric = (endTime-kGenericT0);
  if( deltaGeneric.is_negative() ) deltaGeneric = -deltaGeneric;
  if( deltaGeneric < TimeDuration(0,0,2,0) ) endTime = kGenericT0;

  deltaGeneric = (startTime-kGenericT0);
  if( deltaGeneric.is_negative() ) deltaGeneric = -deltaGeneric;
  if( deltaGeneric < TimeDuration(0,0,2,0) ) startTime = kGenericT0;


  m_model->draw( false, startTime, endTime );
  can->SetEditable( kFALSE );
  can->Update();
}//void NLSimpleGuiWindow::drawModel()


void NLSimpleGuiWindow::drawEquations()
{
  TCanvas *can = ui->m_mathFormulaWidget->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );
  m_equationPt->Clear();
  vector<string> eqns = m_model->getEquationDescription();

  foreach( const string &s, eqns ) m_equationPt->AddText( s.c_str() );

  m_equationPt->Draw();

  //need to make ROOTupdate gPad now
  can->Update();
  can->SetEditable( kFALSE );


  can->SetEditable( kFALSE );
  can->Update();
}// void NLSimpleGuiWindow::drawEquations()


void NLSimpleGuiWindow::updateCustomEventTab()
{
}//void updateCustomEventTab()

void NLSimpleGuiWindow::doGeneticOptimization()
{
  if( !m_model ) return;

  drawModel();
  m_model->geneticallyOptimizeModel( m_model->m_settings.m_lastPredictionWeight );

  drawModel();
  drawEquations();
}// void NLSimpleGuiWindow::doGeneticOptimization()


void NLSimpleGuiWindow::doMinuit2Fit()
{
  if( !m_model ) return;

  m_model->fitModelToDataViaMinuit2( m_model->m_settings.m_lastPredictionWeight );

  drawModel();
  drawEquations();
}// void NLSimpleGuiWindow::doMinuit2Fit()


void NLSimpleGuiWindow::addCgmsData()
{
  DataInputGui inputGui( &(m_model->m_cgmsData),
                         "Enter new CGMS Data",
                         int(CgmsDataImport::CgmsReading),
                         this );
  inputGui.show();
  inputGui.raise();

  const int returnCode = inputGui.exec();
  if( returnCode )
  {
    setMinMaxDisplayLimits();
    drawModel();
  }//if( returnCode )
}// void NLSimpleGuiWindow::addCgmsData()


void NLSimpleGuiWindow::addCarbData()
{
  DataInputGui inputGui( &(m_model->m_mealData),
                         "Enter new Meal Data",
                         int(CgmsDataImport::GlucoseEaten),
                         this );
  inputGui.show();
  inputGui.raise();

  const int returnCode = inputGui.exec();
  if( returnCode )
  {
    setMinMaxDisplayLimits();
    drawModel();
  }//if( returnCode )
}// void NLSimpleGuiWindow::addCarbData()


void NLSimpleGuiWindow::addMeterData()
{
  DataInputGui inputGui( &(m_model->m_fingerMeterData),
                         "Enter new Meter Data",
                         int(CgmsDataImport::MeterReading),
                         this );
  inputGui.show();
  inputGui.raise();

  const int returnCode = inputGui.exec();
  if( returnCode )
  {
    setMinMaxDisplayLimits();
    drawModel();
  }//if( returnCode )
}// void NLSimpleGuiWindow::addMeterData()


void NLSimpleGuiWindow::addCustomEventData()
{
  DataInputGui inputGui( &(m_model->m_customEvents),
                         "Enter new Meter Data",
                         int(CgmsDataImport::GenericEvent),
                         this );
  inputGui.show();
  inputGui.raise();

  const int returnCode = inputGui.exec();
  if( returnCode )
  {
    setMinMaxDisplayLimits();
    drawModel();
  }//if( returnCode )
}// void NLSimpleGuiWindow::addCustomEventData()


void NLSimpleGuiWindow::refreshPredictions()
{
}// void NLSimpleGuiWindow::refreshPredictions()




void NLSimpleGuiWindow::zoomModelPreview( double factor )
{
  TQtWidget *qtWidget = ui->m_modelDisplayWidget;
  QScrollArea *scrollArea = ui->m_modelDisplayScrollArea;
  TCanvas *can = qtWidget->GetCanvas();
  can->SetEditable( kTRUE );
  can->Update(); //need this or else TCanvas won't have updated axis range

  double xmin, xmax, ymin, ymax;
  can->GetRangeAxis( xmin, ymin, xmax, ymax );
  const double nMinutes = xmax - xmin;

  if( nMinutes > 1 )
  {
    int canvasWidth = qtWidget->width();
    int scrollAreaWidth = scrollArea->width();

    int newWidth = factor * canvasWidth;
    newWidth = max(newWidth, scrollAreaWidth);
    newWidth = min(newWidth, 5*scrollAreaWidth);

    if( newWidth == scrollAreaWidth )
       scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    else scrollArea->setHorizontalScrollBarPolicy ( Qt::ScrollBarAsNeeded );

    int h = scrollArea->height() - scrollArea->verticalScrollBar()->height();
    qtWidget->setMinimumSize( newWidth, h );
    qtWidget->setMaximumSize( newWidth, h );
  }//if( nMinutes > 1 )

  can->Update();
  can->SetEditable( kFALSE );
}//void NLSimpleGuiWindow::zoomModelPreview( double factor )

void NLSimpleGuiWindow::zoomModelPreviewPlus()
{
  zoomModelPreview(1.1);
}// void NLSimpleGuiWindow::zoomModelPreviewPlus()


void NLSimpleGuiWindow::zoomModelPreviewMinus()
{
  zoomModelPreview(0.9);
}// void NLSimpleGuiWindow::zoomModelPreviewMinus()

void NLSimpleGuiWindow::setMinMaxDisplayLimits()
{
  //Probably isn't necassary to change widgets, but I was having some trouble
  //  before getting a NULL painter with TQWidget...
  QWidget *origTabWidget = ui->tabWidget->currentWidget();
  ui->tabWidget->setCurrentWidget(ui->modelDisplyTab);

  PosixTime start(kGenericT0), end(kGenericT0);
  if( !m_model->m_cgmsData.empty() )
  {
    end   = m_model->m_cgmsData.getEndTime();
    start = m_model->m_cgmsData.getStartTime();

    m_programOptionsGui->setTrainingTimeLimits(start, end, true);

    if( !m_model->m_predictedBloodGlucose.empty() )
    {
      end   = max( end, m_model->m_predictedBloodGlucose.getEndTime() );
      start = min( start, m_model->m_predictedBloodGlucose.getStartTime() );
    }//if( have CGMS data )
  }else if( !m_model->m_predictedBloodGlucose.empty() )
  {
    end   = m_model->m_predictedBloodGlucose.getEndTime();
    start = m_model->m_predictedBloodGlucose.getStartTime();
  }//if( have CGMS data )

  //m_model->m_glucoseAbsorbtionRate
  //m_model->m_freePlasmaInsulin
  //m_model->m_predictedInsulinX

  if( (start==kGenericT0) || (end==kGenericT0) ) return;

  ui->endDisplayTime->setDateTimeRange( posixTimeToQTime(start), posixTimeToQTime(end) );
  ui->startDisplayTime->setDateTimeRange( posixTimeToQTime(start), posixTimeToQTime(end) );

  // if( posixTimeToQTime(kGenericT0) == ui->endDisplayTime->dateTime() )
   //  ui->endDisplayTime->setDateTime( posixTimeToQTime(end) );
  // if( posixTimeToQTime(kGenericT0) == ui->startDisplayTime->dateTime() )
    // ui->startDisplayTime->setDateTime( posixTimeToQTime(start) );

  ui->endDisplayTime->setDateTime( posixTimeToQTime(end) );
  ui->startDisplayTime->setDateTime( posixTimeToQTime(start) );

  ui->tabWidget->setCurrentWidget(origTabWidget);
}//void NLSimpleGuiWindow:setMinMaxDisplayLimits()


//Check to make sure display time comes before end display time
void NLSimpleGuiWindow::checkDisplayTimeLimitsConsistency()
{
  QDateTimeEdit *endEntry = ui->endDisplayTime;
  QDateTimeEdit *startEntry = ui->startDisplayTime;

  if( startEntry->dateTime() <= endEntry->dateTime() )
    return;
  if( posixTimeToQTime(kGenericT0) == endEntry->dateTime() )
    return;
  if( posixTimeToQTime(kGenericT0) == startEntry->dateTime() )
    return;

  QObject *caller = QObject::sender();
  if( caller == dynamic_cast<QObject *>(endEntry) )
  {
    endEntry->setDateTime( startEntry->dateTime() );
  }else if( caller == dynamic_cast<QObject *>(startEntry) )
  {
    startEntry->setDateTime( endEntry->dateTime() );
  }else
  {
    cout << "void NLSimpleGuiWindow::checkDisplayTimeLimitsConsistency()"
         << " error in caller logic" << endl;
    endEntry->setDateTime( startEntry->dateTime() );
  }//
}//void NLSimpleGuiWindow::checkDisplayTimeLimitsConsistency()


void NLSimpleGuiWindow::cleanupClarkAnalysis()
{
  TCanvas *can = ui->clarkeErrorGridWidget->GetCanvas();
  cleanCanvas( can, "TFrame" );

  can = ui->clarkeLegendWidget->GetCanvas();
  cleanCanvas( can, "TFrame" );

  can = ui->clarkResultsWidget->GetCanvas();
  cleanCanvas( can, "TFrame" );
}// void NLSimpleGuiWindow::cleanupClarkAnalysis()


void NLSimpleGuiWindow::refreshClarkAnalysis()
{
  if( !m_model ) return;
  const ConsentrationGraph &cmgsGraph = m_model->m_cgmsData;
  const ConsentrationGraph &meterGraph = m_model->m_fingerMeterData;
  const ConsentrationGraph &predictedGraph = m_model->m_predictedBloodGlucose;

  bool isCmgsVMeter = (m_clarkeButtonGroup->checkedId() == 0 );

  if( isCmgsVMeter ) drawClarkAnalysis( meterGraph, cmgsGraph, isCmgsVMeter );
  else               drawClarkAnalysis( cmgsGraph, meterGraph, isCmgsVMeter );
}// void NLSimpleGuiWindow::refreshClarkAnalysis()



void NLSimpleGuiWindow::drawClarkAnalysis( const ConsentrationGraph &xGraph,
                                           const ConsentrationGraph &yGraph,
                                           bool isCgmsVMeter )
{
  ui->tabWidget->setCurrentWidget(ui->clarkeGridTab);
  cleanupClarkAnalysis();

  TimeDuration cmgsDelay(0,0,0,0);

  if( isCgmsVMeter )
  {
    TCanvas *can = ui->clarkResultsWidget->GetCanvas();
    can->cd();
    can->SetEditable( kTRUE );
    TPaveText *delayErrorEqnPt = new TPaveText(0, 0, 1.0, 1.0, "NDC");
    delayErrorEqnPt->SetBorderSize(0);
    delayErrorEqnPt->SetTextAlign(12);
    cmgsDelay = m_model->findCgmsDelayFromFingerStick();
    double sigma = 1000.0 * m_model->findCgmsErrorFromFingerStick(cmgsDelay);
    sigma = static_cast<int>(sigma + 0.5) / 10.0; //nearest tenth of a percent

    string delayStr = "Delay=";
    delayStr += boost::posix_time::to_simple_string(cmgsDelay).substr(3,5);
    delayStr += "   ";
    ostringstream uncertDescript;
    uncertDescript << "#sigma_{cgms}^{finger}=" << sigma << "%";

    delayErrorEqnPt->AddText( uncertDescript.str().c_str() );
    delayErrorEqnPt->AddText( delayStr.c_str() );
    delayErrorEqnPt->Draw();
    can->SetEditable( kFALSE );
    can->Update();
  }//if( isCgmsVMeter )


  TCanvas *can = ui->clarkeErrorGridWidget->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );
  vector<TObject *> clarkesObj;
  clarkesObj = getClarkeErrorGridObjs( yGraph, xGraph, cmgsDelay, true );

  assert( dynamic_cast<TH1 *>(clarkesObj[0]) );
  dynamic_cast<TH1 *>(clarkesObj[0])->GetYaxis()->SetTitleOffset(1.3);

  clarkesObj[0]->Draw("SCAT");
  clarkesObj[1]->Draw("SCAT SAME");
  clarkesObj[2]->Draw("SCAT SAME");
  clarkesObj[3]->Draw("SCAT SAME");
  clarkesObj[4]->Draw("SCAT SAME");
  TLegend *leg = dynamic_cast<TLegend *>( clarkesObj[5] );
  assert( leg );

  //Now draw all the boundry lines
  for( size_t i=6; i < clarkesObj.size(); ++i ) clarkesObj[i]->Draw();
  can->SetEditable( kFALSE );
  can->Update();
  // can->ResizePad();
  // ui->clarkeErrorGridWidget->Refresh();

  can = ui->clarkeLegendWidget->GetCanvas();
  can->cd();
  leg->SetX1(-0.1);
  leg->SetX2(1.1);
  leg->SetY1(0.0);
  leg->SetY2(1.0);
  leg->Draw();
  can->SetEditable( kFALSE );
  can->Update();
}// void NLSimpleGuiWindow::drawPredictedClarkAnalysis()



void NLSimpleGuiWindow::openExistingModel()
{
  QString fileName;
  NLSimple *newModel = openNLSimpleModelFile( fileName, this );

  if( !newModel ) return;
  if( m_model && m_ownsModel ) delete m_model;
  m_model = newModel;
  m_fileName = fileName.toStdString();
  init();
}//void NLSimpleGuiWindow::openExistingModel()


void NLSimpleGuiWindow::openNewModel()
{
  NLSimple *newModel = NULL;
  QString newFileName;
  NlSimpleCreate createDialog( newModel, &newFileName, this );
  createDialog.show();
  createDialog.raise();
  createDialog.setModal(true);
  createDialog.exec();

  if( newModel )
  {
    if( m_model && m_ownsModel ) delete m_model;
    m_model = newModel;
    m_ownsModel = true;
    m_fileName = newFileName.toStdString();
  }//if( we succesfully opened a new model )

  if( !m_model ) close();
}//void NLSimpleGuiWindow::openNewModel()


void NLSimpleGuiWindow::saveModel()
{
  if( m_fileName != "" )
  {
    m_model->saveToFile(m_fileName);
  }else
  {
    QString name = QFileDialog::getSaveFileName ("../../../../data/",
                                    "Dub Model (*.dubm)",
                                    this,
                                    "Save File Dialog",
                                    "Choose a file name to save under" );
    if( name.length() )
    {
      m_model->saveToFile(name.toStdString());
      m_fileName = name.toStdString();
      cout << "Updated model file: "
           << m_fileName << " to current state" << endl;
    }//if( succesfully got a file name )
  }//if( already have a file name ) / else
}//void NLSimpleGuiWindow::saveModel()


void NLSimpleGuiWindow::saveModelAs()
{
  QString name = QFileDialog::getSaveFileName ("../../../../data/",
                                    "Dub Model (*.dubm)",
                                    this,
                                    "Save File Dialog",
                                    "Choose a file name to save under" );
  if( name.length() )
  {
    m_model->saveToFile(name.toStdString());
    m_fileName = name.toStdString();
    cout << "Saved current model as " << m_fileName << endl;
    //if( m_ownsModel ) delete m_model;
    //model = NLSimple(m_fileName);
  }// if( succesfully got a file name )
}//void NLSimpleGuiWindow::saveModelAs()


void NLSimpleGuiWindow::quit()
{
  close();
}//void NLSimpleGuiWindow::quit()

