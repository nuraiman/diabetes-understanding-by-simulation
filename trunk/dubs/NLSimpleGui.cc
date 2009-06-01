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
#include "ProgramOptions.hh"
#include "ArtificialPancrease.hh"
#include "RungeKuttaIntegrater.hh"

using namespace std;



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
  
  const char* fileTypes[2] = {"dubm file", "*.dubm"};
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
    
    
