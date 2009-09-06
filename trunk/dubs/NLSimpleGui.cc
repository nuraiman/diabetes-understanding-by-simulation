#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <string>
#include <stdio.h>
#include <math.h>  //contains M_PI
#include <stdlib.h>
#include <fstream>
#include <algorithm> //min/max_element
#include <float.h> // for DBL_MAX

#include "TApplication.h"
#include "TList.h"
#include "TAxis.h"
#include "TGraph.h"


#include "NLSimpleGui.hh"
#include "ResponseModel.hh"
#include "CgmsDataImport.hh"
#include "ProgramOptions.hh"
#include "ArtificialPancrease.hh"
#include "RungeKuttaIntegrater.hh"
#include "ConsentrationGraphGui.hh"

using namespace std;

ClassImp(ConstructNLSimple)


NLSimpleGui::NLSimpleGui( NLSimple *model, bool runApp )
{
  m_model = model;
  m_fileName = "";
  m_ownsModel = false;
  m_parFindSettingsChanged = false;
  m_minutesGraphPerPage = 3 * 60 * 24; //3 days
  
  m_mainFrame = new TGMainFrame(gClient->GetRoot(),1000,1000,kMainFrame | kVerticalFrame);
  m_mainFrame->SetCleanup(kDeepCleanup);
  //m_mainFrame->SetLayoutManager(new TGTileLayout(m_mainFrame, 8));
  m_mainFrame->Connect("CloseWindow()", "TApplication", gApplication, "Terminate(=0)");
  
  if( m_model == NULL )
  {
    m_ownsModel = true;
    m_fileName = getFileName(true);
    
    if( m_fileName == "" && runApp ) return; //whatever
    
    m_model = new NLSimple( m_fileName ); //this will pop-up another file dialog
    m_model->m_gui = this;
  }//if( need to load model )
  
  
  initializeMenu();
  
  
  TGHorizontalFrame *eqnF = new TGHorizontalFrame(m_mainFrame, 400, 100, kHorizontalFrame | kFitWidth | kFitHeight);
  createEquationsCanvas( eqnF );
  //Add Equations to Canvas
  TGLayoutHints *equationCanvasHint = new TGLayoutHints( kLHintsRight | kLHintsTop,2,2,2,2);
  eqnF->AddFrame(m_equationCanvas, equationCanvasHint);
  eqnF->Resize(eqnF->GetDefaultSize());
  
  
  TGHorizontalFrame *graphButtonF = new TGHorizontalFrame(m_mainFrame, 1000, 700, kHorizontalFrame);
  // tab widget
  m_tabWidget = new TGTab(graphButtonF,0.5*graphButtonF->GetWidth(),graphButtonF->GetHeight());
  addGraphTab();
  addProgramOptionsTab();
  m_tabWidget->SetTab(0);
  m_tabWidget->Resize(m_tabWidget->GetDefaultSize());
  
  //Add buttons to frame
  TGVerticalFrame *buttonFrame = createButtonsFrame(graphButtonF);
  TGLayoutHints *buttonFrameHint = new TGLayoutHints( kLHintsLeft | kLHintsTop,2,2,2,2);
  graphButtonF->AddFrame(buttonFrame, buttonFrameHint); 
  TGLayoutHints *tabHint = new TGLayoutHints(kLHintsExpandY | kLHintsExpandX | kLHintsRight | kLHintsBottom,2,2,2,2);
  graphButtonF->AddFrame(m_tabWidget, tabHint);
  graphButtonF->Resize(graphButtonF->GetDefaultSize());
  
  TGLayoutHints *graphButtonHint = new TGLayoutHints( kLHintsCenterX | kLHintsExpandY | kLHintsExpandX |kLHintsTop,2,2,2,2);
  TGLayoutHints *fileMenuHint = new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0);
  
  
  
  m_mainFrame->AddFrame(m_menuBar, fileMenuHint);
  m_mainFrame->AddFrame(eqnF, equationCanvasHint);
  m_mainFrame->AddFrame(graphButtonF, graphButtonHint);
  
  
  
  
  m_mainFrame->SetWindowName( "NLSimple" );
  m_mainFrame->SetMWMHints( kMWMDecorAll, kMWMFuncAll, kMWMInputModeless );
  
  //Next lines make the widgets visible
  m_mainFrame->MapSubwindows();
  m_mainFrame->Resize(m_mainFrame->GetDefaultSize());
  m_mainFrame->MapWindow();
  
  m_mainFrame->Resize(900,650);
  
  updateModelSettings(0);
  drawModel();
  drawEquations();
  drawModel();  //first time it is drawn, it's weird; so this is a complete hack
  if( runApp ) gApplication->Run(kTRUE);
}//NLSimpleGui()


NLSimpleGui::~NLSimpleGui()
{
  m_mainFrame->Cleanup();
  delete m_mainFrame;
  
  if( m_ownsModel ) delete m_model;
}//NLSimpleGui



void NLSimpleGui::initializeMenu()
{
  assert( m_mainFrame );
  m_MenuFile = new TGPopupMenu(gClient->GetRoot());
  m_MenuFile->AddEntry("&Save", SAVE_MODEL);
  m_MenuFile->AddEntry("Save &As", SAVE_MODEL_AS);
  m_MenuFile->AddSeparator();
  m_MenuFile->AddEntry("E&xit", QUIT_GUI_SESSION);
  m_MenuFile->Connect("Activated(Int_t)", "NLSimpleGui", 
                       this, "handleMenu(Int_t)");

  m_menuBar = new TGMenuBar(m_mainFrame, m_mainFrame->GetWidth(), 20, kHorizontalFrame| kRaisedFrame );
  
  TGLayoutHints *hint = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 0, 0, 0);
  m_menuBar->AddPopup("&File", m_MenuFile, hint );
  
  
  m_menuBar->Resize(m_menuBar->GetDefaultSize());
  
}// initializeMenu()



void NLSimpleGui::addGraphTab()
{
  int width = m_tabWidget->GetWidth();
  int height =  m_tabWidget->GetHeight();
  
  // container of "Tab1"
  TGCompositeFrame *graphTabFrame = m_tabWidget->AddTab("Graph");
  graphTabFrame->SetLayoutManager(new TGVerticalLayout(graphTabFrame));
   
  //Every once in a while I like ROOT, then comes along something so simple, 
  //  like adding scroll bars to a graph, and I remember ROOT is a huge pile
  //  of shit, the workaround below to get scroll bars took forever to figure out
  //Make a TGCanvas  dimentions
  //Make a TGCompositeFrame that is container for above TGCanvas
  //make a TRootEmbeddedCanvas //m_modelEmbededCanvas, add this to TGCompositeFrams
  //make a TCanvas, add to above TRootEmbeddedCanvas
  TGLayoutHints *hintBorderExpand  = new TGLayoutHints(kLHintsExpandY| kLHintsExpandX,2,2,2,2);
  TGLayoutHints *hintNoBorderExpandY  = new TGLayoutHints(kLHintsCenterY| kLHintsCenterX | kLHintsExpandY| kLHintsExpandX,0,0,0,0);
  
  m_modelBaseTGCanvas  = new TGCanvas( graphTabFrame, width, height, kFitWidth | kFitHeight );
  m_modelBaseFrame     = new TGCompositeFrame( m_modelBaseTGCanvas->GetViewPort(), 10, 10, kVerticalFrame );
  m_modelBaseTGCanvas->SetContainer( m_modelBaseFrame );
  
  m_modelEmbededCanvas = new TRootEmbeddedCanvas(0,m_modelBaseFrame,10,10);
  m_modelBaseFrame->AddFrame( m_modelEmbededCanvas, hintNoBorderExpandY );
  Int_t modelCanvasId  = m_modelEmbededCanvas->GetCanvasWindowId();
  TCanvas *modelCanvas = new TCanvas("ModelGraphCanvas", 10, 10, modelCanvasId);
  modelCanvas->SetBottomMargin( 0.11 );
  modelCanvas->SetRightMargin( 0.02 );
  modelCanvas->SetLeftMargin( 0.08 );
  m_modelEmbededCanvas->AdoptCanvas(modelCanvas);
  m_modelEmbededCanvas->SetWidth( 1500 );
   
  graphTabFrame->AddFrame( m_modelBaseTGCanvas, hintBorderExpand);
  
  
  // m_modelEmbededCanvas->SetWidth( 1500 );
}//NLSimpleGui::addGraphTab()

void NLSimpleGui::addProgramOptionsTab()
{
  int width = m_tabWidget->GetWidth();
  int height =  m_tabWidget->GetHeight();
  
  ProgramOptionsGui *settingsEntry = new ProgramOptionsGui( m_tabWidget, m_model, width, height );
  m_tabWidget->AddTab("Settings", settingsEntry );
  ((ProgramOptionsGui *)settingsEntry)->Connect( "valueChanged(UInt_t)", "NLSimpleGui", this, "setModelSettingChanged(UInt_t)");
}//addProgramOptionsTab



void NLSimpleGui::createEquationsCanvas( const TGFrame *parent )
{
  int width = parent->GetWidth();
  int height = parent->GetHeight();
    
  m_equationCanvas = new TRootEmbeddedCanvas("m_equationCanvas",parent, width, height, kSunkenFrame| kDoubleBorder| kFitWidth | kFitHeight);
  Int_t eqnCanvasId = m_equationCanvas->GetCanvasWindowId();
  TCanvas *equationCanvas = new TCanvas("EquationCanvas", width, height, eqnCanvasId);
  m_equationCanvas->AdoptCanvas(equationCanvas);
    
  equationCanvas->cd();
  m_equationPt = new TPaveText(0, 0, 1.0, 1.0, "NDC");
  m_equationPt->SetBorderSize(0);
  m_equationPt->SetTextAlign(12);
  m_equationPt->Draw();
  equationCanvas->SetEditable( kFALSE );
}//NLSimpleGui::createEquationsCanvas()


TGVerticalFrame *NLSimpleGui::createButtonsFrame( const TGFrame *parent)
{
  int width = 0.2 * parent->GetWidth();
  int height = parent->GetHeight();
  
  TGVerticalFrame *horizFrame = new TGVerticalFrame(parent, width, height, kVerticalFrame);
  TGLayoutHints *buttonHint = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY| kLHintsTop | kLHintsCenterX,10,10,10,10);
  
  TGTextButton *geneticOptButton = new TGTextButton(horizFrame,"Genetically\n Optimize");
  geneticOptButton->SetTextJustify(36);
  geneticOptButton->SetMargins(5,5,5,5);
  horizFrame->AddFrame(geneticOptButton, buttonHint);
  geneticOptButton->Connect( "Clicked()", "NLSimpleGui", this, "doGeneticOptimization()" );
  
  TGTextButton *miniutFitButton = new TGTextButton(horizFrame,"Baysian\n Fine Tune");
  miniutFitButton->SetTextJustify(36);
  miniutFitButton->SetMargins(5,5,5,5);
  horizFrame->AddFrame(miniutFitButton, buttonHint);
  miniutFitButton->Connect( "Clicked()", "NLSimpleGui", this, "doMinuit2Fit()" );
  
  
  TGTextButton *addCgmsButton = new TGTextButton(horizFrame,"Add CGMS Data");
  addCgmsButton->SetTextJustify(36);
  addCgmsButton->SetMargins(1,1,1,1);
  horizFrame->AddFrame(addCgmsButton, buttonHint);
  addCgmsButton->Connect( "Clicked()", "NLSimpleGui", this, "addCgmsData()" );
  
  TGTextButton *addCarbButton = new TGTextButton(horizFrame,"Add Carb Data");
  addCarbButton->SetTextJustify(36);
  addCarbButton->SetMargins(1,1,1,1);
  horizFrame->AddFrame(addCarbButton, buttonHint);
  addCarbButton->Connect( "Clicked()", "NLSimpleGui", this, "addCarbData()" );
  
  TGTextButton *addeMeterButton = new TGTextButton(horizFrame,"Add Meter Data");
  addeMeterButton->SetTextJustify(36);
  addeMeterButton->SetMargins(1,1,1,1);
  horizFrame->AddFrame(addeMeterButton, buttonHint);
  addeMeterButton->Connect( "Clicked()", "NLSimpleGui", this, "addMeterData()" );
  
  
  TGTextButton *drawButton = new TGTextButton(horizFrame, "Redraw" );
  drawButton->SetTextJustify(36);
  drawButton->SetMargins(0,0,0,0);
  horizFrame->AddFrame(drawButton, buttonHint);
  drawButton->Connect( "Clicked()", "NLSimpleGui", this, "drawModel()" );
  
  
  TGHorizontalFrame *zoomFrame = new TGHorizontalFrame(horizFrame, width, 30, kHorizontalFrame);
  TGLayoutHints *zoomHint = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY| kLHintsCenterY,0,0,0,0);
  
  TGTextButton *plusButton = new TGTextButton(zoomFrame, "+" );
  plusButton->SetTextJustify(36);
  plusButton->SetMargins(0,0,0,0);
  plusButton->Connect( "Clicked()", "NLSimpleGui", this, "zoomXAxis(=0.9)" );
  zoomFrame->AddFrame(plusButton, zoomHint );
  
  TGTextButton *minusButton = new TGTextButton(zoomFrame, "-" );
  minusButton->SetTextJustify(36);
  minusButton->SetMargins(0,0,0,0);
  minusButton->Connect( "Clicked()", "NLSimpleGui", this, "zoomXAxis(=1.1)" );
  zoomFrame->AddFrame(minusButton, zoomHint );
  
  TGLayoutHints *zoomFrameHint = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY| kLHintsBottom,10,10,10,10);
  horizFrame->AddFrame(zoomFrame, zoomFrameHint);
  
  
  horizFrame->Resize(horizFrame->GetDefaultSize());
  
  return horizFrame;
}//createButtonsFrame()



bool NLSimpleGui::modelDefined()
{
  if( !m_model ) return false;
  if( m_model->m_paramaters.size() != NLSimple::NumNLSimplePars ) return false;
  
  foreach( double d, m_model->m_paramaters ) if( d == kFailValue ) return false;
  
  return true;
}//bool modelDefined();


std::string NLSimpleGui::getFileName( bool forOpening )
{
  assert( gApplication );
  
  const char* fileTypes[] = {"dubm file", "*.dubm", 0, 0};
  TGFileInfo fi;
  fi.fFileTypes = fileTypes;
  fi.fIniDir    = StrDup("../data/");

  if( forOpening ) new TGFileDialog(gClient->GetRoot(), gClient->GetDefaultRoot(), kFDOpen, &fi);
  else             new TGFileDialog(gClient->GetRoot(), gClient->GetDefaultRoot(), kFDSave, &fi);
  
  // printf("Open file: %s (dir: %s)\n", fi.fFilename, fi.fIniDir);
  if( fi.fFilename == NULL ) return "";
    
  return fi.fFilename;
}//getFileName


void NLSimpleGui::zoomXAxis( double amount )
{
  
  m_minutesGraphPerPage = amount * m_minutesGraphPerPage;
  
  assert( m_minutesGraphPerPage > 0.0 );
  m_tabWidget->SetTab(0);
  updateModelGraphSize();
}//void zoomXAxis( double amount );



void NLSimpleGui::drawModel()
{
  if( !m_model ) return;
  
  m_tabWidget->SetTab(0);
  
  TCanvas *can = m_modelEmbededCanvas->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );
  
  //lets ovoid memmory clutter
  TObject *obj;
  TList *list = can->GetListOfPrimitives();
  for( TIter nextObj(list); (obj = nextObj()); )
  {
    string className = obj->ClassName();
    if( className != "TFrame" ) delete obj;
  }//for(...)
  
  const bool pause = false;
  m_model->draw( pause );
  
  updateModelGraphSize();
}//DrawModel()


void NLSimpleGui::updateModelGraphSize()
{
  TCanvas *can = m_modelEmbededCanvas->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );
  can->Update(); //need this or else TCanvas won't have updated axis range
 
  double xmin, xmax, ymin, ymax;
  can->GetRangeAxis( xmin, ymin, xmax, ymax );
  double nMinutes = xmax - xmin;
  // cout << m_minutesGraphPerPage << "  " << nMinutes << endl;
  
  //always fill up screen
  if( nMinutes < m_minutesGraphPerPage ) m_minutesGraphPerPage = nMinutes;
  
  int pageWidth = m_modelBaseTGCanvas->GetWidth();
  double nPages = min(5.0, nMinutes / m_minutesGraphPerPage);
  
  //We can run out of memmorry oif we let the canvas get too large
  //  so we need to protect against that posibility
  if( nPages >= 5.0 )  m_minutesGraphPerPage = nMinutes / 5.0;
  
  m_modelEmbededCanvas->SetWidth( nPages * pageWidth );
  
  //need to make ROOT update gPad now
  int w = m_modelEmbededCanvas->GetWidth();
  int h = m_modelBaseTGCanvas->GetHeight();
  int scrollWidth = m_modelBaseTGCanvas->GetHScrollbar()->GetHeight();
  can->SetCanvasSize( w, h-scrollWidth );
  can->SetWindowSize( w + (w-can->GetWw()), h+(h-can->GetWh()) -scrollWidth);
  m_modelBaseFrame->SetHeight( h - scrollWidth);
  m_modelBaseFrame->SetSize( m_modelBaseFrame->GetSize() );
  
  m_modelEmbededCanvas->SetHeight( h -scrollWidth);
  m_modelEmbededCanvas->SetSize( m_modelEmbededCanvas->GetSize() );
  can->Update();
  
  m_mainFrame->MapSubwindows();
  can->SetEditable( kFALSE );
}//void NLSimpleGui::updateModelGraphSize()


void NLSimpleGui::drawEquations()
{
  if( !modelDefined() ) return;
  
  TCanvas *can = m_equationCanvas->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );
  
  m_equationPt->Clear();
  
  vector<string> eqns = m_model->getEquationDescription();
  
  foreach( const string &s, eqns ) m_equationPt->AddText( s.c_str() );
  
  m_equationPt->Draw();
  
  //need to make ROOTupdate gPad now
  can->Update();
  can->SetEditable( kFALSE );
}//DrawEquations()



void NLSimpleGui::handleMenu(Int_t menuAction)
{
  switch(menuAction)
  {
    case SAVE_MODEL_AS:
    {
      string newFileName = getFileName(false);
      if( newFileName == "" ) return;  //user clicked cancel
      
      m_fileName = newFileName; //
      if( !m_model ) return;  //probably shouldn't ever happen
      
      m_model->saveToFile(m_fileName);
      break;
    };
    
    case SAVE_MODEL:
      if( m_fileName == "" ) m_fileName = getFileName(false);
     
      if( !m_model ) return;  //probably shouldn't ever happen
      if( m_fileName == "" ) return;
      
      m_model->saveToFile(m_fileName);
    break;
    
    case QUIT_GUI_SESSION:
      gApplication->Terminate(0);
    break;
    
    assert(0);
  };//switch(menuAction)
}//handleMenu


void NLSimpleGui::doGeneticOptimization()
{
  if( !m_model ) return;
  
  m_tabWidget->SetTab(0);
  drawModel();
  m_model->geneticallyOptimizeModel( ModelDefaults::kLastPredictionWeight );
  
  drawModel();
  drawEquations();
  
  m_parFindSettingsChanged = false;
}//DoGeneticOptimization()


void NLSimpleGui::doMinuit2Fit()
{
  if( !m_model ) return;

  m_model->fitModelToDataViaMinuit2( ModelDefaults::kLastPredictionWeight );
  
  drawModel();
  drawEquations();
  
  m_parFindSettingsChanged = false;
}//DoMinuit2Fit()



void NLSimpleGui::addCgmsData()
{
  // const ConsentrationGraph &cgmsG = m_model->m_cgmsData;
  // double lastCgmsValue = cgmsG.value(cgmsG.getEndTime());
  new InputSimpleData( &(m_model->m_cgmsData),
                                    gClient->GetRoot(), 
                                    gClient->GetDefaultRoot(),
                                    TString("Enter new CGMS Data"),
                                    -1.0,
                                    int(CgmsDataImport::CgmsReading) ); 
}//void NLSimpleGui::addCgmsData()


void NLSimpleGui::addCarbData()
{
  cout << "You need to complete NLSimpleGui::addCarbData()" << endl;
  // InputSimpleData::InputSimpleData( &(m_model->m_cgmsData)
                                    // gClient->GetRoot(), 
                                    // gClient->GetDefaultRoot(),
                                    // "Enter new Carb Data",
                                    // m_model->m_cgmsData.back().m_value,
                                    // CgmsDataImport::CgmsReading ); 
}//void addCarbData()


void NLSimpleGui::addMeterData()
{
  cout << "You need to complete NLSimpleGui::addMeterData()" << endl;
}//void addMeterData()


void NLSimpleGui::refreshPredictions()
{
  cout << "You need to complete NLSimpleGui::refreshPredictions()" << endl;
}//void refreshPredictions()


void NLSimpleGui::updateModelSettings(UInt_t setting)
{
  setting = setting; //keep compiler from comp[laining
  if( m_model )
  {
    m_model->m_basalGlucoseConcentration = PersonConstants::kBasalGlucConc;
    m_model->m_cgmsDelay = ModelDefaults::kDefaultCgmsDelay;
    m_model->m_predictAhead = ModelDefaults::kPredictAhead;
    m_model->m_dt = ModelDefaults::kIntegrationDt;
  }//
}//void updateModel()




void NLSimpleGui::setModelSettingChanged(UInt_t setting)
{
  // cout << "settings have changed 0x" << hex << setting << dec << endl;
  // cout << "Changing " <<  m_model->m_cgmsDelay << "   " 
       // << m_model->m_predictAhead << "   "
       // << m_model->m_dt << "   " << " to ";
  
  m_parFindSettingsChanged = true;
  updateModelSettings(setting);
  
  // cout << "Changing " <<  m_model->m_cgmsDelay << "   " 
       // << m_model->m_predictAhead << "   "
       // << m_model->m_dt << endl;
  
  Emit( "modelSettingChanged()");
}//modelSettingChanged()





ConstructNLSimple::ConstructNLSimple( NLSimple *&model, 
                                      const TGWindow *parent, 
                                      const TGWindow *main ) :
    TGTransientFrame(parent, main, 640, 520, kVerticalFrame),
    m_model(model),
    m_startTimeEntry(NULL), m_endTimeEntry(NULL), m_basalInsulinAmount(NULL),
    m_createButton(NULL),
    m_userSetTime(false), m_cgmsData(NULL), m_bolusData(NULL), 
    m_insulinData(NULL), m_carbConsumptionData(NULL), 
    m_carbAbsortionGraph(NULL), m_meterData(NULL), 
    m_cgmsButton(NULL), m_bolusButton(NULL), 
    m_carbButton(NULL), m_meterButton(NULL),
    m_baseTGCanvas(NULL), 
    m_baseFrame(NULL), m_embededCanvas(NULL), m_minutesGraphPerPage(3 * 60 * 24)
{
  SetCleanup(kDeepCleanup);
  Connect("CloseWindow()", "CreateGraphGui", this, "CloseWindow()");
  DontCallClose();
  
  if( m_model )
  {
    cout << "ConstructNLSimple::ConstructNLSimple(ConsentrationGraph *&graph, ...):"
         << " Warning, recived non-NULL graph pointer, potential memmorry leak" 
         << endl;
    m_model = NULL;
  }//if( m_graph )
  
  
  
  
  TGHorizontalFrame *openDefinedF = new TGHorizontalFrame(this, 160, 50, kHorizontalFrame | kFitWidth);
  TGLayoutHints *openDefinedHint = new TGLayoutHints( kLHintsCenterX | kLHintsTop,2,2,2,2);
  TGTextButton *openDefined = new TGTextButton(openDefinedF, "Open Defined Model", kSELECT_MODEL_FILE);
  openDefined->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openDefinedF->AddFrame(openDefined, openDefinedHint);
  
  TGTextButton *cancelButton = new TGTextButton(openDefinedF, "Cancel", kCANCEL);
  m_createButton = new TGTextButton(openDefinedF, "Create", kCREATE);
  m_createButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  cancelButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  m_createButton->SetEnabled( kFALSE );
  TGLayoutHints *actionHinta = new TGLayoutHints( kLHintsRight | kLHintsCenterY | kLHintsExpandY,0,15,0,0);
  openDefinedF->AddFrame( m_createButton, actionHinta );
  actionHinta = new TGLayoutHints( kLHintsRight | kLHintsCenterY | kLHintsExpandY,0,0,0,0);
  openDefinedF->AddFrame( cancelButton, actionHinta );

  openDefinedHint = new TGLayoutHints( kLHintsCenterX | kLHintsTop| kLHintsExpandX,2,2,2,2);
  AddFrame( openDefinedF, openDefinedHint );
  
  TGHorizontalFrame *graphF = new TGHorizontalFrame(this, 550, 430, kHorizontalFrame | kFitWidth | kFitHeight);
  
  TGVerticalFrame *zoomF = new TGVerticalFrame(graphF, 15, 430, kVerticalFrame );
  TGLayoutHints *zoomButtonHint = new TGLayoutHints( kLHintsCenterX | kLHintsCenterY | kLHintsExpandX,1,2,1,2);
  
  TGTextButton *zoomIn = new TGTextButton(zoomF, "+", kZOOM_PLUS);
  zoomIn->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  zoomF->AddFrame(zoomIn, zoomButtonHint);
  
  TGTextButton *zoomOut = new TGTextButton(zoomF, "-", kZOOM_MINUS);
  zoomOut->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  zoomF->AddFrame(zoomOut, zoomButtonHint);
  
  
  TGLayoutHints *zoomHints  = new TGLayoutHints(kLHintsCenterY | kLHintsLeft,2,2,2,2);
  graphF->AddFrame( zoomF, zoomHints );
  
  
  //use below to open each Concentration graph
  //CreateGraphGui( ConsentrationGraph *&graph, const TGWindow *parent, const TGWindow *main, int graphType = -1 );
  TGLayoutHints *hintBorderExpand  = new TGLayoutHints(kLHintsExpandY| kLHintsExpandX | kLHintsCenterX | kLHintsCenterY,2,2,2,2);
  TGLayoutHints *hintNoBorderExpand  = new TGLayoutHints( kLHintsExpandY| kLHintsExpandX | kLHintsRight,0,0,0,0);
  
  
  m_baseTGCanvas  = new TGCanvas( graphF, 450, 430, kFitWidth | kFitHeight );
  m_baseFrame     = new TGCompositeFrame( m_baseTGCanvas->GetViewPort(), 450, 430, kVerticalFrame );
  m_baseTGCanvas->SetContainer( m_baseFrame );
  
  m_embededCanvas = new TRootEmbeddedCanvas(0,m_baseFrame,450,430);
  m_baseFrame->AddFrame( m_embededCanvas, hintNoBorderExpand );
  Int_t modelCanvasId  = m_embededCanvas->GetCanvasWindowId();
  TCanvas *canvas = new TCanvas("GraphCanvas", 450, 430, modelCanvasId);
  m_embededCanvas->AdoptCanvas(canvas);
  canvas->Divide( 1, kNUM_PAD );
  graphF->AddFrame( m_baseTGCanvas, hintBorderExpand );
  
  //okay, now for buttons to open the graphs
  TGVerticalFrame *openGraphF = new TGVerticalFrame(graphF, 75, 430, kVerticalFrame | kFitWidth | kFitHeight);
  TGLayoutHints *buttonHint = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY| kLHintsTop | kLHintsCenterX,10,10,10,10);
  
  m_cgmsButton = new TGTextButton(openGraphF, "Open CGMS Data", kSELECT_CGMS_DATA);
  m_cgmsButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(m_cgmsButton, buttonHint);
  
  m_bolusButton = new TGTextButton(openGraphF, "Open Bolus Data", kSELECT_BOLUS_DATA);
  m_bolusButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(m_bolusButton, buttonHint);
  
  m_carbButton = new TGTextButton(openGraphF, "Open Carb Data", kSELECT_CARB_DATA);
  m_carbButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(m_carbButton, buttonHint);
  
  m_meterButton = new TGTextButton(openGraphF, "Meter (optional)", kSELECT_METER_DATA);
  m_meterButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(m_meterButton, buttonHint);
  
  TGLayoutHints *buttonFramehint  = new TGLayoutHints( kLHintsRight | kLHintsExpandY,0,0,0,0);
  graphF->AddFrame(openGraphF, buttonFramehint);
  
  TGLayoutHints *graphFramehint  = new TGLayoutHints( kLHintsRight | kLHintsTop | kLHintsExpandY | kLHintsExpandX,0,0,0,0);
  AddFrame( graphF, graphFramehint );
  
  
  //Now add the create model, or cancel buttons, as well as the ending and beggining times
  TGHorizontalFrame *bottomButtonsF = new TGHorizontalFrame(this, 640, 40, kHorizontalFrame | kFitWidth);
  
  m_startTimeEntry     = new TGNumberEntry( bottomButtonsF, 0.0, 6, kSTART_TIME, TGNumberFormat::kNESHourMin, TGNumberFormat::kNEANonNegative);
  m_startDateEntry     = new TGNumberEntry( bottomButtonsF, 0.0, 9, kSTART_TIME, TGNumberFormat::kNESDayMYear, TGNumberFormat::kNEANonNegative);
  m_endTimeEntry       = new TGNumberEntry( bottomButtonsF, 0.0, 6, kEND_TIME, TGNumberFormat::kNESHourMin, TGNumberFormat::kNEANonNegative);
  m_endDateEntry       = new TGNumberEntry( bottomButtonsF, 0.0, 9, kEND_TIME, TGNumberFormat::kNESDayMYear, TGNumberFormat::kNEANonNegative);
  m_basalInsulinAmount = new TGNumberEntry( bottomButtonsF, 0.0, 4, kBASAL_AMOUNT, TGNumberFormat::kNESRealTwo, TGNumberFormat::kNEANonNegative);
    
  TGLayoutHints *buttonTimehint = new TGLayoutHints( kLHintsLeft | kLHintsCenterY,0,0,0,5);
  TGLayoutHints *texthint = new TGLayoutHints( kLHintsLeft | kLHintsCenterY,20,0,0,0);
  TGLabel *label = new TGLabel(bottomButtonsF, "Start Time");
  bottomButtonsF->AddFrame( label, texthint );
  bottomButtonsF->AddFrame( m_startDateEntry, buttonTimehint );
  bottomButtonsF->AddFrame( m_startTimeEntry, buttonTimehint );
  
  label = new TGLabel(bottomButtonsF, "End Time");
  bottomButtonsF->AddFrame( label, texthint );
  bottomButtonsF->AddFrame( m_endDateEntry, buttonTimehint );
  bottomButtonsF->AddFrame( m_endTimeEntry, buttonTimehint );
  
  label = new TGLabel(bottomButtonsF, "Basal Insulin (U/h)");
  bottomButtonsF->AddFrame( label, texthint );
  bottomButtonsF->AddFrame( m_basalInsulinAmount, buttonTimehint );
  
  m_startTimeEntry->SetEditDisabled( kTRUE );  //doesnt' actually do what I want it too
  m_endTimeEntry->SetEditDisabled( kTRUE );
  m_startDateEntry->SetEditDisabled( kTRUE );
  m_endDateEntry->SetEditDisabled( kTRUE );
    
  //should try explicitly casting below to TGNumberEntry
  m_startTimeEntry->Connect( "ValueSet(Long_t)", "ConstructNLSimple", this, "handleTimeLimitButton()" );
  m_endTimeEntry->Connect( "ValueSet(Long_t)", "ConstructNLSimple", this, "handleTimeLimitButton()" );
  m_startDateEntry->Connect( "ValueSet(Long_t)", "ConstructNLSimple", this, "handleTimeLimitButton()" );
  m_endDateEntry->Connect( "ValueSet(Long_t)", "ConstructNLSimple", this, "handleTimeLimitButton()" );
  m_basalInsulinAmount->Connect( "ValueSet(Long_t)", "ConstructNLSimple", this, "enableCreateButton()" );
  
  TGHorizontalFrame *createCancelF = new TGHorizontalFrame(bottomButtonsF, 75, 40, kHorizontalFrame | kFitWidth | kFitHeight);
    
  TGLayoutHints *creatCancelHint  = new TGLayoutHints( kLHintsRight | kLHintsExpandY,0,15,0,5);
  bottomButtonsF->AddFrame( createCancelF, creatCancelHint );
  
  
  TGLayoutHints *bottomButtonhint  = new TGLayoutHints( kLHintsLeft | kLHintsBottom | kLHintsExpandX,0,0,2,2);
  AddFrame( bottomButtonsF, bottomButtonhint);
    
  MapSubwindows();
  TGDimension size = GetDefaultSize();
  Resize(size);
  CenterOnParent();
  SetWMSize(size.fWidth, size.fHeight);
  SetWindowName("Open or Construct NLSimple Model");
  MapWindow();
  Resize( 640, 520 );

  fClient->WaitFor(this);
}//ConstructNLSimple










ConstructNLSimple::~ConstructNLSimple()
{ 
  CloseWindow();
}//~ConstructNLSimple()


void ConstructNLSimple::CloseWindow()
{
  //I know you don't need the if's below
  if( m_cgmsData )            delete m_cgmsData;
  if( m_insulinData )         delete m_insulinData;
  if( m_carbConsumptionData ) delete m_carbConsumptionData;
  if( m_carbAbsortionGraph )  delete m_carbAbsortionGraph;
  if( m_meterData )           delete m_meterData;
  if( m_bolusData )           delete m_bolusData;
  
  m_cgmsData = NULL;
  m_insulinData = NULL;
  m_carbConsumptionData = NULL;
  m_carbAbsortionGraph = NULL;
  m_meterData = NULL;
  m_bolusData = NULL;
  
  DeleteWindow();
}//CloseWindow(




void ConstructNLSimple::enableCreateButton()
{
  double insalinValue = m_basalInsulinAmount->GetNumber();
  
  if( m_cgmsData && (m_insulinData || m_bolusData) 
           && ( m_carbConsumptionData || m_carbAbsortionGraph )
           && (insalinValue > 0.0) ) 
    m_createButton->SetEnabled( kTRUE );
}//enableCreateButton


//When handleButton is called for one of the time limits entry boxes,
//  the widget ID isn't actually the ID assigned to the entry widget,
//  so using this seperate function get araound that
void ConstructNLSimple::handleTimeLimitButton()
{
  m_userSetTime = true;
  for( GraphPad pad = kCGMS_PAD; pad < kNUM_PAD; pad = GraphPad(pad + 1) )
    drawPreviews( pad );
}//void ConstructNLSimple::handleTimeLimitButton()


void ConstructNLSimple::handleButton()
{
  TGButton *btn = (TGButton *) gTQSender;
  int senderId = btn->WidgetId();
  
  switch( senderId )
  {
    case kSELECT_MODEL_FILE:
    {
      const char* fileTypes[4] = {"dubm file", "*.dubm", 0, 0};
      TGFileInfo fi;
      fi.fFileTypes = fileTypes;
      fi.fIniDir    = StrDup("../data/");
      new TGFileDialog(gClient->GetRoot(), gClient->GetDefaultRoot(), kFDOpen, &fi);
  
      if( fi.fFilename == NULL ) return;
      
      m_model = new NLSimple( fi.fFilename );
    
      CloseWindow();
      break;
    }//case kSELECT_MODEL_FILE:
    
      
    case kSELECT_CGMS_DATA:
    {
      ConsentrationGraph *newCgmsData = NULL;
      new CreateGraphGui( newCgmsData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::CgmsReading );
      
      if( !m_cgmsData )
      {
        m_cgmsData = newCgmsData;
        if(m_cgmsData) m_cgmsButton->SetText( "Add More\nCGMS Data" );
      }else if(newCgmsData)
      {
        m_cgmsData->addNewDataPoints( *newCgmsData );
        delete newCgmsData;
      }//if(first data) / else
      
      
      findTimeLimits();
      drawPreviews( kCGMS_PAD );
      drawPreviews( kCARB_PAD );
      drawPreviews( kMERTER_PAD );
      drawPreviews( kBOLUS_PAD );
      enableCreateButton();
      break;
    }//case kSELECT_CGMS_DATA:
    
    case kSELECT_BOLUS_DATA:
    {
      ConsentrationGraph *newBolusData = NULL;
      new CreateGraphGui( newBolusData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::BolusTaken );
      if( !m_bolusData )
      {
        m_bolusData = newBolusData;
        if( m_bolusData ) m_bolusButton->SetText( "Add More\nBolus Data" );
      }else if( newBolusData ) 
      {
        m_bolusData->addNewDataPoints( *newBolusData );
        delete newBolusData;
      }//if / else
      
      findTimeLimits();
      drawPreviews( kBOLUS_PAD );
      drawPreviews( kCGMS_PAD );
      drawPreviews( kCARB_PAD );
      drawPreviews( kMERTER_PAD );
      
      enableCreateButton();
      break;
    }//case kSELECT_BOLUS_DATA:
    
    case kSELECT_CARB_DATA:
    {
      // if( m_carbAbsortionGraph )  delete m_carbAbsortionGraph;
      // if( m_carbConsumptionData ) delete m_carbConsumptionData;
      ConsentrationGraph *newCarbData = NULL;
      
      new CreateGraphGui( newCarbData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::GlucoseEaten );
      if( !m_bolusData )
      {
        m_carbConsumptionData = newCarbData;
        if( m_carbConsumptionData ) m_carbButton->SetText( "Add More\nCarb Data" );
      }else if( newCarbData ) 
      {
        m_carbConsumptionData->addNewDataPoints( *newCarbData );
        delete newCarbData;
      }//if / else
      
      findTimeLimits();
      drawPreviews( kCARB_PAD );
      drawPreviews( kCGMS_PAD );
      drawPreviews( kMERTER_PAD );
      drawPreviews( kBOLUS_PAD );
      
      enableCreateButton();
      break;
    }//case kSELECT_CARB_DATA:
    
    case kSELECT_METER_DATA:
    {
      
      ConsentrationGraph *newMeterData = NULL;
      new CreateGraphGui( newMeterData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::MeterReading );
      
      if( !m_meterData) 
      {
        m_meterData = newMeterData;
        if( m_meterData ) m_meterButton->SetText( "Add More\nMeter Data" );
      }else if( newMeterData )
      {
        m_meterData->addNewDataPoints( *newMeterData );
        delete newMeterData;
      }//if(first time data ) / else(adding more data)
    
      drawPreviews( kMERTER_PAD );
      enableCreateButton();
      break;
    }//case kSELECT_METER_DATA:
    
    case kZOOM_PLUS:
      m_minutesGraphPerPage = 0.9 * m_minutesGraphPerPage;
      updateModelGraphSize();
    break; 
    
    case kZOOM_MINUS:
      m_minutesGraphPerPage = 1.1 * m_minutesGraphPerPage;
      updateModelGraphSize();
    break;
    
    case kSTART_TIME:
    case kEND_TIME:
      handleTimeLimitButton();
    break;
    
    case kBASAL_AMOUNT:
      enableCreateButton();
    break;
    
    case kCREATE:
      constructModel();
      CloseWindow();
      return;
    break;
    
    case kCANCEL:
      assert( !m_model );
      CloseWindow();
      // return;
    break;
    
    assert(0);
  }//switch( senderId )
}//void ConstructNLSimple::handleButton( int senderId )


// 

void ConstructNLSimple::findTimeLimits()
{
  //Don't let user control time span before loading any graphs
  if( m_userSetTime )
  {
    m_userSetTime = ( m_cgmsData || m_insulinData || m_bolusData 
                      || m_carbConsumptionData || m_carbAbsortionGraph
                      || m_meterData );
  }//if( m_userSetTime )
  
  if( m_userSetTime ) return;
  
  PosixTime endTime = kGenericT0;
  PosixTime startTime = kGenericT0;
  
  if( m_cgmsData )
  {
    endTime = m_cgmsData->getEndTime();
    startTime = m_cgmsData->getStartTime();
  }//if( m_cgmsData )
  
  if( m_bolusData )
  {
    PosixTime end = m_bolusData->getEndTime() + TimeDuration( 24, 0, 0, 0 );
    PosixTime start = m_bolusData->getStartTime() - TimeDuration( 24, 0, 0, 0 );
    startTime = std::max( startTime, start );
    endTime = std::min( endTime, end );
  }//if( m_bolusData )
  
  if( m_carbConsumptionData )
  {
    PosixTime end = m_carbConsumptionData->getEndTime() + TimeDuration( 24, 0, 0, 0 );
    PosixTime start = m_carbConsumptionData->getStartTime() - TimeDuration( 24, 0, 0, 0 );
    startTime = std::max( startTime, start );
    endTime = std::min( endTime, end );
  }//if( m_carbConsumptionData )
  
  
  
  if( (endTime != kGenericT0) && (startTime != kGenericT0) )
  {
    m_startTimeEntry->SetEditDisabled( kFALSE );
    m_endTimeEntry->SetEditDisabled( kFALSE );
    m_startDateEntry->SetEditDisabled( kFALSE );
    m_endDateEntry->SetEditDisabled( kFALSE );
    
    // PosixTime endTime = m_cgmsData->getEndTime();
    // PosixTime startTime = m_cgmsData->getStartTime();
    // cout << "StartTime=" << startTime << " and endTime=" << endTime << endl;
    
    int startHours = startTime.time_of_day().hours();
    int startMinutes = startTime.time_of_day().minutes();
    int endHours = endTime.time_of_day().hours();
    int endMinutes = endTime.time_of_day().minutes();
    
    int startIntTime = 60*startHours + startMinutes;
    int endIntTime = 60*endHours + endMinutes;
    
    int startYear = startTime.date().year();
    int startMonth = startTime.date().month();
    int startDay = startTime.date().day();
    int endYear = endTime.date().year();
    int endMonth = endTime.date().month();
    int endDay = endTime.date().day();
    
    int startIntDate = 10000 * startYear + 100 * startMonth + startDay;
    int endIntDate = 10000 * endYear + 100 * endMonth + endDay;
    
    m_startTimeEntry->SetLimits(TGNumberFormat::kNELLimitMinMax, startIntTime, endIntTime);
    m_endTimeEntry->SetLimits(TGNumberFormat::kNELLimitMinMax, startIntTime, endIntTime);
    m_startDateEntry->SetLimits(TGNumberFormat::kNELLimitMinMax, startIntDate, endIntDate);
    m_endDateEntry->SetLimits(TGNumberFormat::kNELLimitMinMax, startIntDate, endIntDate);
    
    m_startTimeEntry->SetTime(startHours, startMinutes, 0 );
    m_endTimeEntry->SetTime(endHours, endMinutes, 0 );
    m_startDateEntry->SetDate( startYear, startMonth, startDay );
    m_endDateEntry->SetDate( endYear, endMonth, endDay );
  }//if( we have time limits )
  
}//void ConstructNLSimple::findTimeLimits()


void ConstructNLSimple::drawPreviews( GraphPad pad )
{
  TCanvas *can = m_embededCanvas->GetCanvas();
  can->SetEditable( kTRUE );
  
  can->cd( pad + 1 );
  vector<TGraph *>graphs;
  
  
  int year, month, day, hour, min, sec;
  m_startTimeEntry->GetTime(hour, min, sec);
  const TimeDuration startTimeDur( hour, min, sec, 0);
  
  m_endTimeEntry->GetTime(hour, min, sec);
  const TimeDuration endTimeDur( hour, min, sec, 0);
  
  m_startDateEntry->GetDate(year, month, day);
  const boost::gregorian::date startDate( year, month, day );
  
  m_endDateEntry->GetDate(year, month, day);
  const boost::gregorian::date endDate( year, month, day );
  
  PosixTime endTime( endDate, endTimeDur );
  PosixTime startTime( startDate, startTimeDur );
  
  if( !m_cgmsData ) endTime = startTime = kGenericT0;
  
  
  switch( pad )
  {
    case kCGMS_PAD:
      if(m_cgmsData) graphs.push_back( m_cgmsData->getTGraph(startTime, endTime) ); 
    break;
      
    case kCARB_PAD:
      if( m_carbConsumptionData /*&& m_carbAbsortionGraph*/ )
      {
        graphs.push_back( m_carbConsumptionData->getTGraph(startTime, endTime) );
        // graphs.push_back( m_carbAbsortionGraph->getTGraph(startTime, endTime) );
      }//if both defined
    break;
    
    case kMERTER_PAD:
      if(m_meterData) graphs.push_back( m_meterData->getTGraph(startTime, endTime) );
    break;
    
    case kBOLUS_PAD:
      if(m_bolusData) graphs.push_back( m_bolusData->getTGraph(startTime, endTime) );
      // m_insulinData
    break;
    
    case kNUM_PAD:
    break;
  };//switch( pad )
  
  for( size_t i = 0; i < graphs.size(); ++i )
  {
    if( graphs[i]->GetN() < 2 ) 
    {
      cout << "Refusing to display " << graphs[i]->GetN() << " points" << endl;
      continue;
    }//if( graphs[i]->GetN() < 2 ) 
    
    graphs[i]->SetLineColor( i+1 );
    
    string drawOptions;
    if( i==0 ) drawOptions += 'A';
    if( pad == kCGMS_PAD ) drawOptions += 'l';
    else                   drawOptions += '*';
    
    graphs[i]->Draw( drawOptions.c_str() );
  }//for( size_t i = 0; i < graphs.size(); ++i )
  
  updateModelGraphSize();
}//drawPreviews


void ConstructNLSimple::updateModelGraphSize()
{
  TCanvas *can = m_embededCanvas->GetCanvas();
  can->SetEditable( kTRUE );
  can->Update(); //need this or else TCanvas won't have updated axis range
  
  // can->cd(1);
  double xmin, xmax, ymin, ymax;
  can->GetRangeAxis( xmin, ymin, xmax, ymax );
  double nMinutes = xmax - xmin;
  // cout << m_minutesGraphPerPage << "  " << nMinutes << endl;
  // cout << "X Range of " << xmin << " to " << xmax << endl; 
  
  //always fill up screen
  if( nMinutes < m_minutesGraphPerPage ) m_minutesGraphPerPage = nMinutes;
  
  int pageWidth = m_baseTGCanvas->GetWidth();
  double nPages = min(5.0, nMinutes / m_minutesGraphPerPage);
  
  //We can run out of memmorry oif we let the canvas get too large
  //  so we need to protect against that posibility
  if( nPages >= 5.0 )  m_minutesGraphPerPage = nMinutes / 5.0;
  
  m_embededCanvas->SetWidth( nPages * pageWidth );

   
  //need to make ROOT update gPad now
  int w = m_embededCanvas->GetWidth();
  int h = m_baseTGCanvas->GetHeight();
  int scrollWidth = m_baseTGCanvas->GetHScrollbar()->GetHeight();
  can->SetCanvasSize( w, h-scrollWidth );
  can->SetWindowSize( w + (w-can->GetWw()), h+(h-can->GetWh()) -scrollWidth);
  
  m_baseFrame->SetHeight( h - scrollWidth);
  m_baseFrame->SetSize( m_baseFrame->GetSize() );
  
  m_embededCanvas->SetHeight( h -scrollWidth);
  m_embededCanvas->SetSize( m_embededCanvas->GetSize() );
  can->Update();
  
  MapSubwindows();
  can->SetEditable( kFALSE );
}//void ConstructNLSimple::updateModelGraphSize()


void ConstructNLSimple::constructModel()
{
  assert( m_cgmsData );
  // m_insulinData;
  assert( m_carbConsumptionData );
  // assert( m_carbAbsortionGraph );
  // m_meterData;
  assert( m_bolusData );
  
  int year, month, day, hour, min, sec;
  m_startTimeEntry->GetTime(hour, min, sec);
  const TimeDuration startTimeDur( hour, min, sec, 0);
  
  m_endTimeEntry->GetTime(hour, min, sec);
  const TimeDuration endTimeDur( hour, min, sec, 0);
  
  m_startDateEntry->GetDate(year, month, day);
  const boost::gregorian::date startDate( year, month, day );
  
  m_endDateEntry->GetDate(year, month, day);
  const boost::gregorian::date endDate( year, month, day );
  
  PosixTime endTime( endDate, endTimeDur );
  PosixTime startTime( startDate, startTimeDur );
  
  // cout << "Creating NLSimple Model with data from " << startDate 
       // << " to " << endTime << endl;
  
  m_bolusData->trim( startTime, endTime );
  m_cgmsData->trim( startTime, endTime );
  // m_carbAbsortionGraph->trim( startTime, endTime );
  m_carbConsumptionData->trim( startTime, endTime );
  
  double insPerHour = m_basalInsulinAmount->GetNumber();
  insPerHour /= PersonConstants::kPersonsWeight;
  assert( insPerHour > 0.0 );

  m_model = new NLSimple( "SimpleModel", insPerHour, PersonConstants::kBasalGlucConc, m_bolusData->getStartTime() );
  m_model->addBolusData( *m_bolusData );
  m_model->addCgmsData( *m_cgmsData );
  // m_model->addGlucoseAbsorption( *m_carbAbsortionGraph );
  m_model->addGlucoseAbsorption( *m_carbConsumptionData );
  
  // m_model->m_freePlasmaInsulin = *m_bolusData;
  // m_model->m_cgmsData = *m_cgmsData;
  // m_model->m_glucoseAbsorbtionRate = *m_carbAbsortionGraph;
}//void ConstructNLSimple::constructModel()







InputSimpleData::InputSimpleData( ConsentrationGraph *graph,
                                  const TGWindow *parent, 
                                  const TGWindow *main,
                                  TString message,
                                  double defaultValue,
                                  int type
                                 ) :
    TGTransientFrame(parent, main, 320, 200, kVerticalFrame),
    m_graph(graph),
    m_type(type),
    m_dateEntry(NULL), m_timeEntry(NULL), m_valueEntry(NULL),
    m_fileButton(NULL)
{
  SetCleanup(kDeepCleanup);
  Connect("CloseWindow()", "InputSimpleData", this, "CloseWindow()");
  DontCallClose();
  assert( m_graph );
 
  TGLayoutHints *buttonHint = new TGLayoutHints( kLHintsCenterY, 5,5,2,2);
  TGHorizontalFrame *buttonRowF = new TGHorizontalFrame(this, 320, 100, kHorizontalFrame | kFitWidth);
  
  TGTextButton *okButton = new TGTextButton(buttonRowF, "Okay" );
  okButton->Connect( "Clicked()", "InputSimpleData", this, "readSingleInputCloseWindow()" );
  buttonRowF->AddFrame(okButton, buttonHint);
  
  TGTextButton *cancelButton = new TGTextButton(buttonRowF, "Cancel" );
  cancelButton->Connect( "Clicked()", "InputSimpleData", this, "CloseWindow()" );
  buttonRowF->AddFrame(cancelButton, buttonHint);
  
  m_fileButton = new TGTextButton(buttonRowF, "From File" );
  m_fileButton->Connect( "Clicked()", "InputSimpleData", this, "readFromFile()" );
  buttonRowF->AddFrame(m_fileButton, buttonHint);
  
  TGLayoutHints *buttonRowHint = new TGLayoutHints( kLHintsCenterX | kLHintsBottom,5,5,2,2);
  AddFrame( buttonRowF, buttonRowHint );
  
  
  TGHorizontalFrame *dateTimeF = new TGHorizontalFrame(this, 320, 100, kHorizontalFrame | kFitWidth);
  
  const PosixTime lastTime = m_graph->getEndTime();
  if( defaultValue == kFailValue ) 
  {
    defaultValue = m_graph->value(lastTime);
  }//if( defaultValue == kFailValue ) 
  
  const int widgetId = -1;
  m_timeEntry  = new TGNumberEntry( dateTimeF, 0.0, 6, widgetId, TGNumberFormat::kNESHourMin, TGNumberFormat::kNEANonNegative);
  m_dateEntry  = new TGNumberEntry( dateTimeF, 0.0, 9, widgetId, TGNumberFormat::kNESDayMYear, TGNumberFormat::kNEANonNegative);
  m_valueEntry = new TGNumberEntry( dateTimeF, defaultValue, 5, widgetId, TGNumberFormat::kNESRealTwo,  TGNumberFormat::kNEANonNegative);
  
  // const PosixTime currentTime(boost::posix_time::second_clock::local_time());
  const PosixTime currentTime = lastTime;
  const int defaultHours = currentTime.time_of_day().hours();
  const int defaultMinutes = currentTime.time_of_day().minutes();
  const int defaultYear = currentTime.date().year();
  const int defaultMonth = currentTime.date().month();
  const int defaultDay = currentTime.date().day();      
  m_timeEntry->SetTime( defaultHours, defaultMinutes, 0 );
  m_dateEntry->SetDate( defaultYear, defaultMonth, defaultDay );
  
  TGLayoutHints *texthint = new TGLayoutHints( kLHintsLeft | kLHintsCenterY,20,0,0,0);
  TGLabel *datelabel = new TGLabel( dateTimeF, "Date/Time");
  TGLabel *valueLabel = new TGLabel( dateTimeF, "Value");
  dateTimeF->AddFrame( datelabel, texthint );
  dateTimeF->AddFrame( m_dateEntry, buttonRowHint);
  dateTimeF->AddFrame( m_timeEntry, buttonRowHint);
  dateTimeF->AddFrame( valueLabel, texthint );
  dateTimeF->AddFrame(m_valueEntry, buttonRowHint);
  
  TGLayoutHints *inputRowHint = new TGLayoutHints( kLHintsCenterX | kLHintsTop,5,5,2,2);
  AddFrame( buttonRowF, inputRowHint );
    
  MapSubwindows();
  TGDimension size = GetDefaultSize();
  Resize(size);
  CenterOnParent();
  SetWMSize(size.fWidth, size.fHeight);
  SetWindowName(message);
  MapWindow();
  Resize( 320, 200 );

  fClient->WaitFor(this);
}//ConstructNLSimple


void InputSimpleData::readSingleInputCloseWindow()
{
  int hour, min, sec, year, month, day;
  m_timeEntry->GetTime(hour, min, sec);
  m_dateEntry->GetDate(year, month, day);
  
  const TimeDuration theTime( hour, min, sec, 0);
  const boost::gregorian::date theDate( year, month, day );
  
  double value = m_valueEntry->GetNumber();
  PosixTime timeEntered( theDate, theTime );
  cout << "Entered time of " << timeEntered << " with a value of " << value << endl;
  
  m_graph->addNewDataPoint( timeEntered, value );
  
  CloseWindow();
}//void InputSimpleData::readSingleInputCloseWindow()


InputSimpleData::~InputSimpleData()
{
  CloseWindow();
}//InputSimpleData::~InputSimpleData()


void InputSimpleData::CloseWindow()
{
  DeleteWindow();
}//void InputSimpleData::CloseWindow();


void InputSimpleData::readFromFile()
{
  GraphType graphType = m_graph->getGraphType();
  
  //Following test will probably fail my first time through...
  switch( m_type )
  {
    case CgmsDataImport::CgmsReading:       assert( graphType == GlucoseConsentrationGraph ); break;
    case CgmsDataImport::MeterReading:      assert( graphType == GlucoseConsentrationGraph ); break;
    case CgmsDataImport::MeterCalibration:  assert( graphType == GlucoseConsentrationGraph ); break;
    case CgmsDataImport::GlucoseEaten:      assert( graphType == GlucoseConsumptionGraph );   break;
    case CgmsDataImport::BolusTaken:        assert( graphType == BolusGraph );                break;
    case CgmsDataImport::ISig:              assert( graphType == NumGraphType );              break;
    
    assert(0);
  };//switch(type)
  
  
  ConsentrationGraph *newData = NULL;
  new CreateGraphGui( newData, gClient->GetRoot(), 
                      gClient->GetDefaultRoot(), m_type );
  if( !newData ) return;
  
  m_graph->addNewDataPoints( *newData );
  delete newData;
  CloseWindow();
}//void InputSimpleData::readFromFile()

    
    
