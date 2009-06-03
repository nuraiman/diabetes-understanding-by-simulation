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



#include "ConsentrationGraph.hh"
#include "NLSimpleGui.hh"
#include "ResponseModel.hh"
#include "ProgramOptions.hh"
#include "ArtificialPancrease.hh"
#include "RungeKuttaIntegrater.hh"
#include "ConsentrationGraphGui.hh"
#include "CgmsDataImport.hh"

using namespace std;


ClassImp(CreateGraphGui)

CreateGraphGui::CreateGraphGui(  ConsentrationGraph *&graph, 
                                 const TGWindow *parent, const TGWindow *main ) :
                 TGTransientFrame(parent, main, 225, 150, kVerticalFrame),
                 m_debug(false), m_graph(graph), m_filePath(NULL), 
                 m_FileInfo(NULL), m_graphTypeListBox(NULL), 
                 m_selectFileButton(NULL), m_openButton(NULL), 
                 m_cancelButton(NULL)
{
  SetCleanup(kDeepCleanup);
  Connect("CloseWindow()", "CreateGraphGui", this, "CloseWindow()");
  DontCallClose();
  
  if( m_graph )
  {
    cout << "CreateGraphGui::CreateGraphGui(ConsentrationGraph *&graph, ...):"
         << " Warning, recived non-NULL graph pointer, potential memmorry leak" 
         << endl;
    m_graph = NULL;
  }//if( m_graph )
  
  const char *fileType[] = { "ConsentrationGraph",     "*.dub",
                             "text dexcom",          "*.txt",
                             "spread sheet minimed", "*.csv",
                             "All Files",              "*",
                             0, 0 };
  
  m_FileInfo = new TGFileInfo();
  m_FileInfo->fFileTypes = fileType;
  m_FileInfo->fIniDir = StrDup( "../data/" );
  
  
  TGHorizontalFrame *filePathHF = new TGHorizontalFrame(this, 200, 50, kHorizontalFrame);
  m_filePath         = new TGTextEntry( filePathHF, m_FileInfo->fIniDir );
  m_filePath->SetText( m_FileInfo->fIniDir );
  m_filePath->Connect( "TextChanged(char*)", "CreateGraphGui", this, "fileNameUpdated()");
  m_filePath->Connect( "TabPressed()", "CreateGraphGui", this, "fileNameUpdated()");
  m_filePath->Connect( "ReturnPressed()", "CreateGraphGui", this, "fileNameUpdated()");
  
  TGVerticalFrame *listBoxFrame = new TGVerticalFrame(this, 50, 50, kVerticalFrame);
  TGLabel *label = new TGLabel( listBoxFrame, "Type (csv,txt only)" );
  
  m_graphTypeListBox = new TGListBox( listBoxFrame, -1, kSunkenFrame|kDoubleBorder );
  // m_graphTypeListBox->AddEntry( "Cgms Data",CgmsDataImport::CgmsReading );
  // m_graphTypeListBox->AddEntry( "Fingerstick Readings",CgmsDataImport::MeterReading );
  // m_graphTypeListBox->AddEntry( "Cgms Calibrations",CgmsDataImport::MeterCalibration );
  // m_graphTypeListBox->AddEntry( "Glucose Eaten",CgmsDataImport::GlucoseEaten );
  // m_graphTypeListBox->AddEntry( "Bolus Taken",CgmsDataImport::BolusTaken );
  // m_graphTypeListBox->AddEntry( "I-Sig",CgmsDataImport::ISig );
  // m_graphTypeListBox->SetEnabled( kFALSE );
  m_graphTypeListBox->Connect( "Selected(Int_t)", "CreateGraphGui", this, "fileNameUpdated()");
  m_graphTypeListBox->Connect( "SelectionChanged()", "CreateGraphGui", this, "fileNameUpdated()");
  
    
  m_selectFileButton = new TGTextButton( filePathHF, "Browse", kSELECT_FILE_BUTTON );
  m_selectFileButton->Connect( "Clicked()", "CreateGraphGui", this, "handleButton()" );
  
  TGLayoutHints *hint = new TGLayoutHints( kLHintsExpandX | kLHintsLeft | kLHintsCenterY,2,2,2,2);
  filePathHF->AddFrame( m_filePath, hint );
  hint = new TGLayoutHints( kLHintsLeft | kLHintsCenterY,2,2,2,2);
  filePathHF->AddFrame( m_selectFileButton, hint );
  
  hint = new TGLayoutHints( kLHintsCenterX | kLHintsTop,2,2,2,2);
  listBoxFrame->AddFrame( label, hint );
  hint = new TGLayoutHints( kLHintsExpandY | kLHintsExpandX | kLHintsRight | kLHintsCenterY,2,2,2,2);
  listBoxFrame->AddFrame( m_graphTypeListBox, hint );
  filePathHF->AddFrame( listBoxFrame, hint );
  
  
  
  TGHorizontalFrame *buttonHF = new TGHorizontalFrame(this, 200, 50, kHorizontalFrame);
  m_openButton       = new TGTextButton( buttonHF, "Open", kOPEN_FILE_BUTTON );
  m_openButton->Connect( "Clicked()", "CreateGraphGui", this, "handleButton()" );
  m_openButton->SetEnabled( kFALSE );
  
  m_cancelButton     = new TGTextButton( buttonHF, "Cancel", kCANCEL_BUTTON );
  m_cancelButton->Connect( "Clicked()", "CreateGraphGui", this, "handleButton()" );  
  
  hint = new TGLayoutHints( kLHintsCenterX | kLHintsCenterY,2,2,2,2);
  buttonHF->AddFrame( m_openButton, hint );
  buttonHF->AddFrame( m_cancelButton, hint );
  
  hint = new TGLayoutHints( kLHintsExpandX | kLHintsExpandY | kLHintsCenterX | kLHintsTop,2,2,2,2);
  AddFrame( filePathHF, hint );
  hint = new TGLayoutHints( kLHintsExpandX | kLHintsBottom,2,2,2,2);
  AddFrame( buttonHF, hint );
  
  MapSubwindows();
  TGDimension size = GetDefaultSize();
  Resize(size);
  CenterOnParent();
  SetWMSize(size.fWidth, size.fHeight);
  SetWindowName("Concentration Graph Open");
  MapWindow();
  Resize( 350, 100 );

  fClient->WaitFor(this);
}//CreateGraphGui( constructor )




CreateGraphGui::~CreateGraphGui()
{
  CloseWindow();
}//~CreateGraphGui()


void CreateGraphGui::fileNameUpdated()
{
  string fileToOpen = m_filePath->GetText();
  
  // cout << "Updating filename with " << fileToOpen << endl;
  
  unsigned int periodPos = fileToOpen.find_last_of( "." );
  string ending;
  if( periodPos != string::npos ) 
  {
    ending = fileToOpen.substr( periodPos );
    
    //what out for cases like '../data/'
    if( periodPos != fileToOpen.length() )
    {
      if( fileToOpen[periodPos+1] == '\\' || fileToOpen[periodPos+1] == '/' ) ending = "";
    }
  }//
  // if( (ending=="") || (ending==".dub") ) m_graphTypeListBox->SetEnabled( kFALSE );
  // else m_graphTypeListBox->SetEnabled( kTRUE );
  
  if( periodPos == string::npos ) 
  {
    periodPos = fileToOpen.find_last_of( "\\/" );
    fileToOpen = fileToOpen.substr(0, periodPos );
    if(periodPos != string::npos ) 
    { 
      delete m_FileInfo->fIniDir;
      m_FileInfo->fIniDir = StrDup( fileToOpen.c_str() );
    }//if( a path ) //not robust!
  }//if( not extension on cuttent text )
  
  if( !ending.empty() )
  {
    std::ifstream ifs( m_filePath->GetText() );  //just seeing if file exists
    if( ifs.is_open() ) 
    {
      if( ending == ".dub" ) 
      {
        m_openButton->SetEnabled( kTRUE );
        m_graphTypeListBox->RemoveAll();
      }else if( true || m_graphTypeListBox->GetSelected() >= 0 ) 
      {
        if(m_graphTypeListBox->GetSelected() >= 0) m_openButton->SetEnabled();
        else
        {
          m_graphTypeListBox->RemoveAll();
          m_graphTypeListBox->AddEntry( "Cgms Data",CgmsDataImport::CgmsReading );
          m_graphTypeListBox->AddEntry( "Fingerstick Readings",CgmsDataImport::MeterReading );
          m_graphTypeListBox->AddEntry( "Cgms Calibrations",CgmsDataImport::MeterCalibration );
          m_graphTypeListBox->AddEntry( "Glucose Eaten",CgmsDataImport::GlucoseEaten );
          m_graphTypeListBox->AddEntry( "Bolus Taken",CgmsDataImport::BolusTaken );
          m_graphTypeListBox->AddEntry( "I-Sig",CgmsDataImport::ISig );
          m_graphTypeListBox->Layout();
        }//if( selection ) / else 
        // m_graphTypeListBox->
      }
    }//if( ifs.is_open() ) 
  }else 
  {
    m_graphTypeListBox->RemoveAll();
    m_openButton->SetEnabled( kFALSE );
  }
}//void CreateGraphGui::handleButton()



void CreateGraphGui::handleButton( int senderId )
{
  if( senderId < 0 )
  {
    TGButton *btn = (TGButton *) gTQSender;
    senderId = btn->WidgetId();
  }//senderId
  
  switch( senderId )
  {
    case kSELECT_FILE_BUTTON: 
    {
      new TGFileDialog(gClient->GetRoot(), gClient->GetDefaultRoot(), kFDOpen, m_FileInfo);
      if( m_FileInfo->fFilename == NULL ) return;
     
      // printf("CreateGraphGui:: Open file: %s (dir: %s)\n", fi.fFilename, fi.fIniDir);
      string newText = m_FileInfo->fFilename;
      m_filePath->SetText( newText.c_str() );
      break;
    };//case kSELECT_FILE_BUTTON: 
   
    case kOPEN_FILE_BUTTON:
    {
      string fileToOpen = m_filePath->GetText();
      if( fileToOpen.empty() ) return;
      
      if( m_graphTypeListBox->GetNumberOfEntries() > 0 ) 
      {
        CgmsDataImport::InfoType graphType = CgmsDataImport::InfoType( m_graphTypeListBox->GetSelected() );
        // cout << "Getting graph type " << graphType << " from " << fileToOpen << endl;
        ConsentrationGraph graph = CgmsDataImport::importSpreadsheet( fileToOpen, graphType );
        m_graph = new ConsentrationGraph( graph );
        CloseWindow();
      } else
      {
        // cout << "About to open graph " << fileToOpen << endl;
        m_graph = new ConsentrationGraph( fileToOpen );
        
        //in principle I should check that I can open the file
        CloseWindow();
      }//if( is from spreadsheet ) / else

      break;
    };//case kOPEN_FILE_BUTTON:
    
    case kCANCEL_BUTTON:
      CloseWindow();
    break;
  };//switch( senderId )
  
};//handleButton()



void CreateGraphGui::CloseWindow()
{
  // delete m_FileInfo;
  DeleteWindow();
}//CloseWindow()


