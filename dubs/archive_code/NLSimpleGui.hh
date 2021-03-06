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
namespace boost{ namespace posix_time{ struct ptime; } }

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
    TPaveText *m_delayErrorEqn;
    
    TRootEmbeddedCanvas *m_errorGridCanvas;
    TRootEmbeddedCanvas *m_errorLegendCanvas;
    TRootEmbeddedCanvas *m_delaySimgaCanvas;
    TRootEmbeddedCanvas *m_customEventCanvas;
    TGRadioButton *m_clarkeMeterButton;
    TGRadioButton *m_clarkePredictionButton;
    TGListBox *m_customEventListBox;
    
    // TGDockableFrame *m_menuDock;
    TGMenuBar *m_menuBar;
    TGPopupMenu *m_MenuFile;
    
    bool m_parFindSettingsChanged;
  
  
    //If model is not secified, pops up a file dialog to load from file
    NLSimpleGui( NLSimple *model = NULL, bool runApp = false );
    ~NLSimpleGui();
    
    void initializeMenu();       // make the File menu
    void addGraphTab();          //add graph tab to m_tabWidget
    void addProgramOptionsTab(); //add options tab to m_tabWidget
    void addErrorGridTab();      //add Clarke Error Grid  tab to m_tabWidget
    enum CustomEventID
    {
      kCE_ListBox = 89,
      kCE_AddDefinition,
      kCE_DeleteDefinition
    };//enum CustomEventID
    void addCustomEventTab();
    void updateCustomEventTab();
    void drawSelectedCustomEvent();
    void handleCustomEventButton( Int_t senderId );
    void createEquationsCanvas( const TGFrame *parent );
    TGVerticalFrame *createButtonsFrame( const TGFrame *parent );
    
    
    static std::string getFileName( bool forOpening = true );
    
    void drawModel();
    void drawEquations();
    void doGeneticOptimization();
    void doMinuit2Fit();
    void addCgmsData();  //unimpleneted as of yet
    void addCarbData();
    void addMeterData();
    void addCustomEventData();
    void refreshPredictions();
    void updateDelayAndError();
    
    void handleMenu(Int_t menuAction);
    
    void zoomXAxis( double amount );  //amount is fractional, 0.9, or 1.1 is 10% decrease/increas
    void updateModelGraphSize();
    
    void updateModelSettings(UInt_t);
    void setModelSettingChanged(UInt_t);  // *SIGNAL*
    
    enum ClarkeWidgetId
    {
      kCGMS_V_METER_BUTTON,
      kPREDICTION_V_CGMS_BUTTON
    };//enum ClarkeWidgetId
    
    void handleClarkeButton( Int_t senderId );
    void cleanupClarkAnalysis();
    void refreshClarkAnalysis();
    void drawMeterClarkAnalysis();
    void drawPredictedClarkAnalysis();
    
    
    bool modelDefined();
    
    enum MenuActions
    {
      SAVE_MODEL,
      SAVE_MODEL_AS,
      QUIT_GUI_SESSION
    };//enum MenuActions
};//NLSimpleGui


class InputSimpleData : public TGTransientFrame
{
  public:
    ConsentrationGraph *m_graph;
    int m_type; //specified by InfoType
    TGNumberEntry *m_dateEntry;
    TGNumberEntry *m_timeEntry;
    TGNumberEntry *m_valueEntry;
    TGTextButton  *m_fileButton;
    
    InputSimpleData( ConsentrationGraph *graph, 
                     const TGWindow *parent, 
                     const TGWindow *main,
                     TString message,
                     double defaultValue,
                     const boost::posix_time::ptime defaultTime,
                     int type
                   );
    virtual ~InputSimpleData();
    virtual void CloseWindow();
    void readFromFile();
    void readSingleInputCloseWindow();
    
    ClassDef(InputSimpleData,0)
};//class InputSimpleData




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
    TGNumberEntry *m_basalInsulinAmount;
    
    TGButton *m_createButton;
    
    bool m_userSetTime;
    ConsentrationGraph *m_cgmsData;
    ConsentrationGraph *m_bolusData;
    ConsentrationGraph *m_insulinData; //created from m_bolusData
    ConsentrationGraph *m_carbConsumptionData;
    ConsentrationGraph *m_meterData;
    
    TGTextButton *m_cgmsButton;
    TGTextButton *m_bolusButton;
    TGTextButton *m_carbButton;
    TGTextButton *m_meterButton;
 
    
    TGCanvas *m_baseTGCanvas;              //This makes the scroll bar for graph
    TGCompositeFrame *m_baseFrame;         //This hold TCanvas
    TRootEmbeddedCanvas *m_embededCanvas;  //change size of this to zoom
    double m_minutesGraphPerPage;
    
    
    enum GraphPad
    {
      kCGMS_PAD,
      kBOLUS_PAD,
      kCARB_PAD,
      kMERTER_PAD,
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
      
      kBASAL_AMOUNT,
      
      kCREATE,  
      kCANCEL
    };//enum ButtonId
    
    
    //gClient->GetRoot(), gClient->GetDefaultRoot(), graphType = CgmsDataImport::InfoType )
    ConstructNLSimple( NLSimple *&model, const TGWindow *parent, const TGWindow *main);
    virtual ~ConstructNLSimple();
    
    void enableCreateButton();
    void handleButton();
    void handleTimeLimitButton();  //kinda a hack, WidgetId for timeLimit buttons seems messed up see src
    
    void findTimeLimits();
    void drawPreviews( GraphPad pad );
    void updateModelGraphSize();
    void constructModel();
    
    virtual void CloseWindow();
    
    ClassDef(ConstructNLSimple,0)
};//class ConstructNLSimple : public TGTransientFrame




class ConstructCustomEvent : public TGTransientFrame
{
  public:  
    NLSimple      *&m_model;
    TGComboBox    *m_idEntry;
    TGTextEntry   *m_nameEntry;
    TGNumberEntry *m_nPointsEntry;
    TGNumberEntry *m_durationEntry;
    TGButtonGroup *m_eventTypeGroup;
    
    ConstructCustomEvent( NLSimple *&model, const TGWindow *parent, const TGWindow *main);
    virtual ~ConstructCustomEvent();
    virtual void CloseWindow();
    
    void handleButton( Int_t );
    
    bool addEventDefToModel();
    
    enum
    {
      kOkayButtonId,
      kCancelButtonId,
      kComboBoxId,
      kIdEntryId,
      kNameEntryId,
      kNPointsEntryId,
      kDurationId,
      kIndependantEffectId,
      kMultiplyInsulinId,
      kMultiplyCarbConsumedId,
    };//enum
    
  ClassDef(ConstructCustomEvent,0)
};//class ConstructCustomEvent : public TGTransientFrame








#endif //NL_SIMPLE_GUI_HH
