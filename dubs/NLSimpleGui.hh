#ifndef NL_SIMPLE_GUI_HH
#define NL_SIMPLE_GUI_HH

#include "TGFileDialog.h"
#include "TGFSContainer.h"
#include "TGDockableFrame.h"
#include "TGLabel.h"
#include "TGScrollBar.h"
#include "TGShutter.h"
#include "TG3DLine.h"
#include "TGuiBldHintsEditor.h"
#include "TGMdiMainFrame.h"
#include "TGMsgBox.h"
#include "TGTab.h"
#include "TGNumberEntry.h"
#include "TGListBox.h"
#include "TGStatusBar.h"
#include "TGListTree.h"
#include "TGComboBox.h"
#include "TGuiBldHintsButton.h"
#include "TGFSComboBox.h"
#include "TGToolTip.h"
#include "TGButton.h"
#include "TGToolBar.h"
#include "TGCanvas.h"
#include "TGFrame.h"
#include "TGButtonGroup.h"
#include "TGMdiFrame.h"
#include "TGListView.h"
#include "TGMdiDecorFrame.h"
#include "TGLayout.h"
#include "TGXYLayout.h"
#include "TGMdiMenu.h"
#include "TGSplitter.h"
#include "TGMenu.h"
#include "TGuiBldEditor.h"
#include "TRootGuiBuilder.h"
#include "TGuiBldDragManager.h"

#include "TPaveText.h"
#include "TCanvas.h"
#include "TRootEmbeddedCanvas.h"
#include "TQObject.h"
#include "RQ_OBJECT.h"

// #include "ResponseModel.hh"
//Do not include any boost headers (or headers that include boost headers) here
//  ROOT preproccessor wiggs the fuck out

//We have to have this class defined in a seperate file from NLSimple
//  cause ROOT CINT sucks ass, and we have to hide boost from it, and things
//  like explicit copy constructors (cause roots gui sucks)

class NLSimple;
class ConsentrationGraph;


class NLSimpleGui
{
    RQ_OBJECT("NLSimpleGui")
  public:
    NLSimple *m_model;
    bool m_ownsModel;
    std::string m_fileName;
    
    TGMainFrame *m_mainFrame;
    TGTab *m_tabWidget;
    
    TGCanvas *m_modelBaseTGCanvas;              //This makes the scroll bar for graph
    TGCompositeFrame *m_modelBaseFrame;         //This hold TCanvas
    TRootEmbeddedCanvas *m_modelEmbededCanvas;  //change size of this to zoom
    double m_minutesGraphPerPage;
    
    TRootEmbeddedCanvas *m_equationCanvas;
    TPaveText *m_equationPt;
    
    // TGDockableFrame *m_menuDock;
    TGMenuBar *m_menuBar;
    TGPopupMenu *m_MenuFile;
    
    bool m_parFindSettingsChanged;
  
  
    //If model is not secified, pops up a file dialog to load from file
    NLSimpleGui( NLSimple *model = NULL, bool runApp = false );
    ~NLSimpleGui();
    
    void initializeMenu();// make the File menu
    void addGraphTab();  //add graph tab to m_tabWidget
    void addProgramOptionsTab();//add options tab to m_tabWidget
    void createEquationsCanvas( const TGFrame *parent );
    TGVerticalFrame *createButtonsFrame( const TGFrame *parent );
    
    
    static std::string getFileName( bool forOpening = true );
    
    void drawModel();
    void drawEquations();
    void doGeneticOptimization();
    void doMinuit2Fit();
    
    void handleMenu(Int_t menuAction);
    
    void zoomXAxis( double amount );  //amount is fractional, 0.9, or 1.1 is 10% decrease/increas
    void updateModelGraphSize();
    
    void updateModelSettings(UInt_t);
    void setModelSettingChanged(UInt_t);  // *SIGNAL*
    
    bool modelDefined();
    
    enum MenuActions
    {
      SAVE_MODEL,
      SAVE_MODEL_AS,
      QUIT_GUI_SESSION
    };//enum MenuActions
};//NLSimpleGui



//To open (from .dubm file )or construct a NLSimple object from spreadsheets
//The 'create' button will be enabled once you have selected:
//  m_cgmsData, m_insulinData, (m_carbConsumptionData OR m_carbAbsortionGraph)
//  and m_meterData
class ConstructNLSimple : public TGTransientFrame
{
  public:  
    bool m_debug;
    
    NLSimple *&m_model;
    
    TGNumberEntry *m_startDateEntry;
    TGNumberEntry *m_endDateEntry;
    TGNumberEntry *m_startTimeEntry;
    TGNumberEntry *m_endTimeEntry;
    
    TGButton *m_createButton;
    
    bool m_userSetTime;
    ConsentrationGraph *m_cgmsData;
    ConsentrationGraph *m_bolusData;
    ConsentrationGraph *m_insulinData; //created from m_bolusData
    ConsentrationGraph *m_carbConsumptionData;
    ConsentrationGraph *m_carbAbsortionGraph;  //calculated from m_carbConsumptionData
    ConsentrationGraph *m_meterData;
    
    
    
    TGCanvas *m_baseTGCanvas;              //This makes the scroll bar for graph
    TGCompositeFrame *m_baseFrame;         //This hold TCanvas
    TRootEmbeddedCanvas *m_embededCanvas;  //change size of this to zoom
    double m_minutesGraphPerPage;
    
    
    enum GraphPad
    {
      kCGMS_PAD,
      kCARB_PAD,
      kMERTER_PAD,
      kBOLUS_PAD,
      kNUM_PAD
    };//enum GraphCanvas
    
    enum ButtonId
    {
      kSELECT_MODEL_FILE,
      
      kSELECT_CGMS_DATA,
      kSELECT_BOLUS_DATA,
      kSELECT_CARB_DATA,
      kSELECT_METER_DATA,
      
      kZOOM_PLUS,
      kZOOM_MINUS,
      
      kSTART_TIME,
      kEND_TIME,
      
      kCREATE,  
      kCANCEL
    };//enum ButtonId
    
    
    //gClient->GetRoot(), gClient->GetDefaultRoot(), graphType = CgmsDataImport::InfoType )
    ConstructNLSimple( NLSimple *&model, const TGWindow *parent, const TGWindow *main);
    virtual ~ConstructNLSimple();
    
    bool enableCreateButton();
    void handleButton( int senderId = -1 );
    
    void findTimeLimits();
    void drawPreviews();
    void constructModel();
    
    virtual void CloseWindow();
    
    ClassDef(ConstructNLSimple,0)
};//class ConstructNLSimple : public TGTransientFrame



#endif //NL_SIMPLE_GUI_HH
