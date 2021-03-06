#if !defined(CONSENTRATION_GRAPH_GUI_HH)
#define CONSENTRATION_GRAPH_GUI_HH

#include <map>

#include "TGWindow.h"
#include "TGFrame.h"
#include "TGFileDialog.h"
#include "TGListBox.h"
#include "TGNumberEntry.h"
#include "RQ_OBJECT.h"


class ConsentrationGraph;
class CreateGraphGui : public TGTransientFrame
{
  public:  
    bool m_debug; //unused currently
    
    ConsentrationGraph *&m_graph;
    
    int m_graphType; //specified by CgmsDataImport::InfoType
    
    TGTextEntry *m_filePath;
    TGFileInfo *m_FileInfo;
    TGListBox *m_graphTypeListBox;
    
    TGButton *m_selectFileButton;
    TGButton *m_openButton;
    TGButton *m_cancelButton;
    
    enum ButtonId
    {
      kSELECT_FILE_BUTTON,
      kOPEN_FILE_BUTTON,
      kCANCEL_BUTTON
    };//enum ButtonId
    
    
    //gClient->GetRoot(), gClient->GetDefaultRoot(), graphType = CgmsDataImport::InfoType )
    CreateGraphGui( ConsentrationGraph *&graph, const TGWindow *parent, const TGWindow *main, int graphType = -1 );
    virtual ~CreateGraphGui();
    
    void fileNameUpdated();
    void handleButton( int senderId = -1 );
    
    virtual void CloseWindow();
    
    ClassDef(CreateGraphGui,0)
};//CreateGraphGui




#endif //CONSENTRATION_GRAPH_GUI_HH
