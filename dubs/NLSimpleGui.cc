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
  
  drawModel();
  drawEquations();
  
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
  geneticOptButton->SetMargins(0,0,0,0);
  horizFrame->AddFrame(geneticOptButton, buttonHint);
  geneticOptButton->Connect( "Clicked()", "NLSimpleGui", this, "doGeneticOptimization()" );
  
  TGTextButton *miniutFitButton = new TGTextButton(horizFrame,"Miniut2 Fit");
  miniutFitButton->SetTextJustify(36);
  miniutFitButton->SetMargins(0,0,0,0);
  horizFrame->AddFrame(miniutFitButton, buttonHint);
  miniutFitButton->Connect( "Clicked()", "NLSimpleGui", this, "doMinuit2Fit()" );
  
  TGTextButton *drawButton = new TGTextButton(horizFrame, "Draw" );
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
  if( !modelDefined() ) return;
  
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
  can->SetEditable( kFALSE );
}//DrawModel()


void NLSimpleGui::updateModelGraphSize()
{
  TCanvas *can = m_modelEmbededCanvas->GetCanvas();
  can->SetEditable( kTRUE );
  can->Update(); //need this or else TCanvas won't have updated axis range
  double xmin, xmax, ymin, ymax;
  can->GetRangeAxis( xmin, ymin, xmax, ymax );
  double nMinutes = xmax - xmin;
  // cout << m_minutesGraphPerPage << "  " << nMinutes << endl;
  
  //always fill up screen
  if( nMinutes < m_minutesGraphPerPage ) m_minutesGraphPerPage = nMinutes;
  
  int pageWidth = m_modelBaseTGCanvas->GetWidth();
  double nPages = nMinutes / m_minutesGraphPerPage;
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
  if( !modelDefined() ) return;
  
  m_tabWidget->SetTab(0);
  drawModel();
  m_model->geneticallyOptimizeModel( ModelDefaults::kLastPredictionWeight );
  
  drawModel();
  drawEquations();
  
  m_parFindSettingsChanged = false;
}//DoGeneticOptimization()


void NLSimpleGui::doMinuit2Fit()
{
  if( !modelDefined() ) return;

  m_model->fitModelToDataViaMinuit2( ModelDefaults::kLastPredictionWeight );
  
  drawModel();
  drawEquations();
  
  m_parFindSettingsChanged = false;
}//DoMinuit2Fit()



void NLSimpleGui::updateModelSettings(UInt_t setting)
{
  setting = setting; //keep compiler from comp[laining
  if( m_model )
  {
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
    m_startTimeEntry(NULL), m_endTimeEntry(NULL), m_createButton(NULL),
    m_userSetTime(false), m_cgmsData(NULL), m_bolusData(NULL), 
    m_insulinData(NULL), m_carbConsumptionData(NULL), 
    m_carbAbsortionGraph(NULL), m_meterData(NULL), m_baseTGCanvas(NULL), 
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
  
  TGTextButton *cgmsButton = new TGTextButton(openGraphF, "Open CGMS Data", kSELECT_CGMS_DATA);
  cgmsButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(cgmsButton, buttonHint);
  
  TGTextButton *bolusButton = new TGTextButton(openGraphF, "Open Bolus Data", kSELECT_BOLUS_DATA);
  bolusButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(bolusButton, buttonHint);
  
  TGTextButton *carbButton = new TGTextButton(openGraphF, "Open Carb Data", kSELECT_CARB_DATA);
  carbButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(carbButton, buttonHint);
  
  TGTextButton *meterButton = new TGTextButton(openGraphF, "Meter (optional)", kSELECT_METER_DATA);
  meterButton->Connect( "Clicked()", "ConstructNLSimple", this, "handleButton()" );
  openGraphF->AddFrame(meterButton, buttonHint);
  
  TGLayoutHints *buttonFramehint  = new TGLayoutHints( kLHintsRight | kLHintsExpandY,0,0,0,0);
  graphF->AddFrame(openGraphF, buttonFramehint);
  
  TGLayoutHints *graphFramehint  = new TGLayoutHints( kLHintsRight | kLHintsTop | kLHintsExpandY | kLHintsExpandX,0,0,0,0);
  AddFrame( graphF, graphFramehint );
  
  
  //Now add the create model, or cancel buttons, as well as the ending and beggining times
  TGHorizontalFrame *bottomButtonsF = new TGHorizontalFrame(this, 640, 40, kHorizontalFrame | kFitWidth);
  
  m_startTimeEntry = new TGNumberEntry( bottomButtonsF, 0.0, 6, kSTART_TIME, TGNumberFormat::kNESHourMin, TGNumberFormat::kNEANonNegative);
  m_startDateEntry = new TGNumberEntry( bottomButtonsF, 0.0, 9, kSTART_TIME, TGNumberFormat::kNESDayMYear, TGNumberFormat::kNEANonNegative);
  m_endTimeEntry   = new TGNumberEntry( bottomButtonsF, 0.0, 6, kEND_TIME, TGNumberFormat::kNESHourMin, TGNumberFormat::kNEANonNegative);
  m_endDateEntry   = new TGNumberEntry( bottomButtonsF, 0.0, 9, kEND_TIME, TGNumberFormat::kNESDayMYear, TGNumberFormat::kNEANonNegative);
  
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
  
  
  TGHorizontalFrame *createCancelF = new TGHorizontalFrame(bottomButtonsF, 75, 40, kHorizontalFrame | kFitWidth | kFitHeight);
  
  TGTextButton *cancelButton = new TGTextButton(createCancelF, "Cancel", kCANCEL);
  m_createButton = new TGTextButton(createCancelF, "Create", kCREATE);
  m_createButton->SetEnabled( kFALSE );
  TGLayoutHints *actionHint = new TGLayoutHints( kLHintsRight | kLHintsCenterY | kLHintsExpandY,0,0,0,0);
  
  createCancelF->AddFrame( m_createButton, actionHint );
  actionHint = new TGLayoutHints( kLHintsRight | kLHintsCenterY | kLHintsExpandY,0,0,0,0);
  createCancelF->AddFrame( cancelButton, actionHint );
  
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
  
  DeleteWindow();
}//CloseWindow(




void ConstructNLSimple::enableCreateButton()
{
  if( m_cgmsData && (m_insulinData || m_bolusData) 
           && ( m_carbConsumptionData || m_carbAbsortionGraph ) ) 
    m_createButton->SetEnabled( kTRUE );
}//enableCreateButton


void ConstructNLSimple::handleButton( int senderId )
{
  if( senderId < 0 )
  {
    TGButton *btn = (TGButton *) gTQSender;
    senderId = btn->WidgetId();
  }//senderId
  
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
      if( m_cgmsData ) delete m_cgmsData;
      m_cgmsData = NULL;
      new CreateGraphGui( m_cgmsData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::CgmsReading );
      drawPreviews( kCGMS_PAD );
      enableCreateButton();
    break;
    
    case kSELECT_BOLUS_DATA:
      if( m_bolusData ) delete m_bolusData;
      if( m_insulinData ) delete m_insulinData;
      m_bolusData = m_insulinData = NULL;
      new CreateGraphGui( m_bolusData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::BolusTaken );
      if( m_bolusData ) 
      {
        ConsentrationGraph insulinG = CgmsDataImport::bolusGraphToInsulinGraph
                                            ( *m_bolusData, 
                                             PersonConstants::kPersonsWeight );
        m_insulinData = new ConsentrationGraph(insulinG);
      }//if( m_bolusData )
      
      drawPreviews( kBOLUS_PAD );
      enableCreateButton();
    break;
    
    case kSELECT_CARB_DATA:
      if( m_carbAbsortionGraph )  delete m_carbAbsortionGraph;
      if( m_carbConsumptionData ) delete m_carbConsumptionData;
      m_carbAbsortionGraph = m_carbConsumptionData = NULL;
      new CreateGraphGui( m_carbConsumptionData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::GlucoseEaten );
      if( m_carbConsumptionData )
      {
        ConsentrationGraph consumpG = CgmsDataImport::carbConsumptionToSimpleCarbAbsorbtionGraph(*m_carbConsumptionData);
        m_carbAbsortionGraph = new ConsentrationGraph(consumpG);
      }//if( m_carbConsumptionData )
      
      drawPreviews( kCARB_PAD );
      enableCreateButton();
    break;
      
    case kSELECT_METER_DATA:
      if( m_meterData ) delete m_meterData;
      m_meterData = NULL;
      new CreateGraphGui( m_meterData, gClient->GetRoot(), 
                          gClient->GetDefaultRoot(), 
                          CgmsDataImport::MeterReading );
      drawPreviews( kMERTER_PAD );
      enableCreateButton();
    break;
    
    case kZOOM_PLUS:
      m_minutesGraphPerPage = 0.9 * m_minutesGraphPerPage;
      updateModelGraphSize();
    break; 
    
    case kZOOM_MINUS:
      m_minutesGraphPerPage = 1.1 * m_minutesGraphPerPage;
      updateModelGraphSize();
    break;
    
    case kSTART_TIME:
    break;
    
    case kEND_TIME:
    break;
    
    case kCREATE:
      constructModel();
      CloseWindow();
      return;
    break;
    
    case kCANCEL:
      assert( !m_model );
      CloseWindow();
      return;
    break;
    
    assert(0);
  }//switch( senderId )
}//void ConstructNLSimple::handleButton( int senderId )





void ConstructNLSimple::findTimeLimits()
{
  cout << "void ConstructNLSimple::findTimeLimits(): is not implemented yet" << endl;
}//void ConstructNLSimple::findTimeLimits()


void ConstructNLSimple::drawPreviews( GraphPad pad )
{
  TCanvas *can = m_embededCanvas->GetCanvas();
  can->SetEditable( kTRUE );
  
  can->cd( pad + 1 );
  
  switch( pad )
  {
    case kCGMS_PAD:
      if(m_cgmsData) m_cgmsData->draw( "", "", false, 0 );
    break;
      
    case kCARB_PAD:
      if( m_carbConsumptionData && m_carbAbsortionGraph )
      {
        m_carbConsumptionData->draw( "", "", false, 0 );
        m_carbAbsortionGraph->draw( "SAME l", "", false, 2 );
      }//if both defined
    break;
    
    case kMERTER_PAD:
      if(m_meterData) m_meterData->draw( "", "", false, 0 );
    break;
    
    case kBOLUS_PAD:
      if(m_bolusData) m_bolusData->draw( "", "", false, 0 );
      // m_insulinData
    break;
    
    case kNUM_PAD:
    break;
  };//switch( pad )
  
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
  double nPages = nMinutes / m_minutesGraphPerPage;
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
  // m_carbConsumptionData;
  assert( m_carbAbsortionGraph );
  // m_meterData;
  assert( m_bolusData );
  
  m_model = new NLSimple( "SimpleModel", 0, 0, m_bolusData->getT0() );
  m_model->addBolusData( *m_bolusData );
  m_model->addCgmsData( *m_cgmsData );
  m_model->addGlucoseAbsorption( *m_carbAbsortionGraph );
}//void ConstructNLSimple::constructModel()



    
    
