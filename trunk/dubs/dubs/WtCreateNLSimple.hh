#ifndef WTCREATENLSIMPLE_HH
#define WTCREATENLSIMPLE_HH

#include <Wt/WContainerWidget>



class NLSimple;
class Div;
class ConsentrationGraph;
class DateTimeSelect;

namespace Wt
{
  class WText;
  class WLineF;
  class WRectF;
  class WDialog;
  class WComboBox;
  class WLineEdit;
  class WDateTime;
  class WPopupMenu;
  class WTableView;
  class WTabWidget;
  class WFileUpload;
  class WDatePicker;
  class WBorderLayout;
  class WDoubleSpinBox;
  class WStandardItemModel;
  namespace Chart
  {
    class WCartesianChart;
  };//namespace Chart
};//namespace Wt





class WtCreateNLSimple : public Wt::WContainerWidget
{
public:
  WtCreateNLSimple( Wt::WContainerWidget *parent = 0 );
  virtual ~WtCreateNLSimple();

  Wt::Signal<> &created();
  Wt::Signal<> &canceled();
  boost::shared_ptr<NLSimple> model();


protected:
    void init();

    enum DataType
    {
      kCGMS_ENTRY,
      kBOLUS_ENTRY,
      kCARB_ENTRY,
      kMETER_ENTRY,
      //kCustom_ENTRY,
      //kEXCERSIZE_ENTRY,
      //kALL_ENTRY,
      kNUM_DataType
    };//enum kNUM_DataType


private:
    boost::shared_ptr<NLSimple>        m_model;

    bool m_userSetTime;
    std::vector<ConsentrationGraph *>  m_datas;
    Wt::WPushButton                   *m_createButton;
    Wt::WPushButton                   *m_cancelButton;
    DateTimeSelect                    *m_endTime;
    DateTimeSelect                    *m_startTime;
    Wt::WDoubleSpinBox                *m_weightInput;
    Wt::WDoubleSpinBox                *m_basalInsulin;
    Wt::WStandardItemModel            *m_graphModel;
    Wt::Chart::WCartesianChart        *m_graph;
    std::vector<Wt::WFileUpload *>     m_fileUploads;
    std::vector<Wt::WText *>           m_sourceDescripts;

    Wt::Signal<> m_created;
    Wt::Signal<> m_canceled;

    void doEmit( Wt::Signal<> &signal );

    void findTimeLimits();
    void drawPreview();

    void addData( DataType type );

    void constructModel();
    void handleTimeLimitButton();
    void enableCreateButton();
    void checkDisplayTimeLimitsConsistency();

    void failedUpload( ::int64_t type ); //type should be of type DataType

};//class WtCreateNLSimple

#endif // WTCREATENLSIMPLE_HH
