// Mainframe macro generated from application: /Users/banjohik/root/bin/root.exe
// By ROOT version 5.20/00 on 2009-05-20 20:51:44
//
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
  
  m_mainFrame = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
  m_mainFrame->SetLayoutBroken(kTRUE);
  m_mainFrame->SetCleanup(kDeepCleanup);
  m_mainFrame->Connect("CloseWindow()", "TApplication", gApplication, "Terminate(=0)");
  
  if( m_model == NULL )
  {
    m_ownsModel = true;
    m_fileName = getFileName(true);
    
    if( m_fileName == "" && runApp ) return; //whatever
    
    m_model = new NLSimple( m_fileName ); //this will pop-up another file dialog
    m_model->m_gui = this;
  }//if( need to load model )
  
  // m_menuDock = new TGDockableFrame(m_mainFrame);
  // m_mainFrame->AddFrame(m_menuDock, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));
  // m_menuDock->SetWindowName("Menu Dock");
  
  m_MenuFile = new TGPopupMenu(gClient->GetRoot());
  m_MenuFile->AddEntry("&Save", SAVE_MODEL);
  m_MenuFile->AddEntry("Save &As", SAVE_MODEL_AS);
  m_MenuFile->AddSeparator();
  m_MenuFile->AddEntry("E&xit", QUIT_GUI_SESSION);
  m_MenuFile->Connect("Activated(Int_t)", "NLSimpleGui", 
                       this, "handleMenu(Int_t)");

  m_menuBar = new TGMenuBar(m_mainFrame, m_mainFrame->GetMaxWidth(), 20, kHorizontalFrame|kRaisedFrame );
  m_mainFrame->AddFrame(m_menuBar, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));
  m_menuBar->AddPopup("&File", m_MenuFile, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0));
  
    
  // tab widget
  m_tabWidget = new TGTab(m_mainFrame,745,540);
  
  // container of "Tab1"
  TGCompositeFrame *graphTab = m_tabWidget->AddTab("Graph");
  graphTab->SetLayoutManager(new TGVerticalLayout(graphTab));
  graphTab->SetLayoutBroken(kTRUE);
   
  //make a spot to draw the model
  m_modelCanvas = new TRootEmbeddedCanvas(0,graphTab,741,536-20);
  Int_t modelCanvasId = m_modelCanvas->GetCanvasWindowId();
  TCanvas *modelCanvas = new TCanvas("ModelCanvas", 10, 10, modelCanvasId);
  // modelCanvas->SetRightMargin(0.05020353);
  // modelCanvas->SetTopMargin(0.06766918);
  // modelCanvas->SetBottomMargin(0.1334586);
  m_modelCanvas->AdoptCanvas(modelCanvas);
  graphTab->AddFrame(m_modelCanvas, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX | kLHintsExpandY| kLHintsExpandX,2,2,2,2));
  
  
  ProgramOptionsGui *settingsEntry = new ProgramOptionsGui( m_tabWidget, m_model, 712, 496 );
  m_tabWidget->AddTab("Settings", settingsEntry );
  ((ProgramOptionsGui *)settingsEntry)->Connect( "valueChanged(UInt_t)", "NLSimpleGui", this, "setModelSettingChanged(UInt_t)");
  
  m_tabWidget->SetTab(0);
  m_tabWidget->Resize(m_tabWidget->GetDefaultSize());
  m_mainFrame->AddFrame(m_tabWidget, new TGLayoutHints(kLHintsExpandY| kLHintsExpandX,2,2,2,2));
  m_tabWidget->MoveResize(150,105,745,540);
   
  m_equationCanvas = new TRootEmbeddedCanvas("m_equationCanvas",m_mainFrame,550,102);
  Int_t eqnCanvasId = m_equationCanvas->GetCanvasWindowId();
  TCanvas *equationCanvas = new TCanvas("EquationCanvas", 10, 10, eqnCanvasId);
  m_equationCanvas->AdoptCanvas(equationCanvas);
  m_mainFrame->AddFrame(m_equationCanvas, new TGLayoutHints( kLHintsExpandX | kLHintsExpandY,2,2,2,2));
  m_equationCanvas->MoveResize(400,5,500,100);
  
  equationCanvas->cd();
  m_equationPt = new TPaveText(0, 0, 1.0, 1.0, "NDC");
  m_equationPt->SetBorderSize(0);
  m_equationPt->SetTextAlign(12);
  m_equationPt->Draw();
    
  
  TGHorizontalFrame *horizFrame = new TGHorizontalFrame(m_mainFrame,112,424,kHorizontalFrame);
  horizFrame->SetLayoutBroken(kTRUE);
  
  TGTextButton *geneticOptButton = new TGTextButton(horizFrame,"Genetically\n Optimize");
  geneticOptButton->SetTextJustify(36);
  geneticOptButton->SetMargins(0,0,0,0);
  geneticOptButton->SetWrapLength(-1);
  geneticOptButton->Resize(95,48);
  horizFrame->AddFrame(geneticOptButton, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
  geneticOptButton->MoveResize(8,16,95,48);
  geneticOptButton->Connect( "Clicked()", "NLSimpleGui", this, "doGeneticOptimization()" );
  
  TGTextButton *miniutFitButton = new TGTextButton(horizFrame,"Miniut2 Fit");
  miniutFitButton->SetTextJustify(36);
  miniutFitButton->SetMargins(0,0,0,0);
  miniutFitButton->SetWrapLength(-1);
  miniutFitButton->Resize(92,48);
  horizFrame->AddFrame(miniutFitButton, new TGLayoutHints(kLHintsLeft | kLHintsTop ,2,2,2,2));
  miniutFitButton->MoveResize(8,88,92,48);
  miniutFitButton->Connect( "Clicked()", "NLSimpleGui", this, "doMinuit2Fit()" );
  
  TGTextButton *drawButton = new TGTextButton(horizFrame, "Draw" );
  drawButton->SetTextJustify(36);
  drawButton->SetMargins(0,0,0,0);
  drawButton->SetWrapLength(-1);
  drawButton->Resize(92,48);
  horizFrame->AddFrame(drawButton, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
  drawButton->MoveResize(8,160,92,48);
  drawButton->Connect( "Clicked()", "NLSimpleGui", this, "drawModel()" );
  
  
  m_mainFrame->AddFrame(horizFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
  horizFrame->MoveResize(16,120,112,424);
  
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




void NLSimpleGui::drawModel()
{
  if( !modelDefined() ) return;
  
  m_tabWidget->SetTab(0);
  
  TCanvas *can = m_modelCanvas->GetCanvas();
  can->cd();
  
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
  
  // can->Print( "mainFram.C" );
  
  //need to make ROOTupdate gPad now
  can->Update();
}//DrawModel()


void NLSimpleGui::drawEquations()
{
  if( !modelDefined() ) return;
  
  TCanvas *can = m_equationCanvas->GetCanvas();
  can->cd();
  
  m_equationPt->Clear();
  
  vector<string> eqns = m_model->getEquationDescription();
  
  foreach( const string &s, eqns ) m_equationPt->AddText( s.c_str() );
  
  m_equationPt->Draw();
  
  //need to make ROOTupdate gPad now
  can->Update();
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
  cout << "settings have changed 0x" << hex << setting << dec << endl;
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
    
    
