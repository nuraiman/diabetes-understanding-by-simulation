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

class NLSimpleGui
{
    RQ_OBJECT("NLSimpleGui")
  public:
    NLSimple *m_model;
    bool m_ownsModel;
    std::string m_fileName;
    
    TGMainFrame *m_mainFrame;
    TGTab *m_tabWidget;
    TRootEmbeddedCanvas *m_modelCanvas;
    
    TRootEmbeddedCanvas *m_equationCanvas;
    TPaveText *m_equationPt;
    
    // TGDockableFrame *m_menuDock;
    TGMenuBar *m_menuBar;
    TGPopupMenu *m_MenuFile;
    
    bool m_parFindSettingsChanged;
    TGNumberEntry *m_cgmsDelayEntry;
    TGNumberEntry *m_indivCgmsUncertEntry;
    TGNumberEntry *m_predAheadTimeEntry;
    TGNumberEntry *m_lastPredWeightEntry;
    TGNumberEntry *m_integrationDtEntry;
  
  
    //If model is not secified, pops up a file dialog to load from file
    NLSimpleGui( NLSimple *model = NULL, bool runApp = false );
    
    ~NLSimpleGui();
    
    static std::string getFileName( bool forOpening = true );
    
    void drawModel();
    void drawEquations();
    void doGeneticOptimization();
    void doMinuit2Fit();
    
    void handleMenu(Int_t menuAction);
    
    
    void modelSettingChanged(); 
    
    bool modelDefined();
    
    enum MenuActions
    {
      SAVE_MODEL,
      SAVE_MODEL_AS,
      QUIT_GUI_SESSION
    };//enum MenuActions
};//NLSimpleGui


#endif //NL_SIMPLE_GUI_HH
