#ifndef WTGUI_H
#define WTGUI_H
#include <map>
#include <string>
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WDateTime>
#include <Wt/Dbo/ptr>
#include <Wt/Dbo/Session>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Chart/WCartesianChart>
#include <Wt/WLineF>
#include <Wt/WRectF>

#include "ArtificialPancrease.hh"
#include "WtUserManagment.hh"

//Some forward declarations
class NLSimple;
class WtGui;
class Div;
class DateTimeSelect;
class DubEventEntry;

class DubUser;
class UsersModel;
class ModelSettings;
class WtConsGraphModel;
class ConsentrationGraph;

class ClarkErrorGridGraph;
class WtModelSettingsGui;
class MemVariableSpinBox;
class MemGuiTimeDate;

class NLSimplePtr;
class NLSimpleDisplayModel;
class WChartWithLegend;
class WtGeneralArrayModel;

namespace Wt
{
  class WText;
  class WLineF;
  class WRectF;
  class WDialog;
  class WSpinBox;
  class WComboBox;
  class WTableView;
  class WLineEdit;
  class WDateTime;
  class WPopupMenu;
  class WTableView;
  class WTabWidget;
  class WFileUpload;
  class WDatePicker;
  class WBorderLayout;
  class WStandardItemModel;
};//namespace Wt



//Since the gui is multithreaded, but NLSimple is not designed to be multithreaded
//  we'll use a shared pointer that also contains a mutex, so whenever we need to
//  access the NLSimple model of a gui, we can safely do it through this
//  shared pointer / mutex.  This implmenetation asumes the parent WtGui is
//  longer lived than the NLSimplePtr object.
//
//The count(WtGui *) functions can also be used to determine if the model
//  has been accessed (and hence potentially modified) since
//  resetCount(WtGui *) has last been called.
//  I haven't cleaned the code up to really take advantage of this yet
typedef boost::shared_ptr<NLSimple> NLSimpleShrdPtr;
typedef boost::recursive_mutex::scoped_lock RecursiveScopedLock;

class NLSimplePtr : public NLSimpleShrdPtr
{
  WtGui *m_parent;
  boost::shared_ptr<RecursiveScopedLock> m_lock;
  static boost::recursive_mutex  sm_nLockMapMutex;
  static std::map<WtGui *, int> sm_nLocksMap;

public:
  NLSimplePtr( WtGui *gui,
               const bool waite = true,
               const std::string &failureMessage = "" );
  virtual ~NLSimplePtr(){}
  bool has_lock() const;
  bool operator!() const;

  static int count( WtGui *gui );
  static void resetCount( WtGui *gui );
};//class NLSimplePtr




class WtGui : public Wt::WApplication
{
public:
  enum ErrorRegions
  {
    kRegionA = 1, kRegionB, kRegionC, kRegionD, kRegionE, NumErrorRegions
  };//enum ErrorRegions

  struct ModelGraphPair
  {
    Wt::WStandardItemModel     *model;
    Wt::Chart::WCartesianChart *graph;
  };//struct ModelGraphPair


  enum DataSources
  {
    kTimeData,
    kCgmsData,
    kFreePlasmaInsulin,
    kGlucoseAbsRate,
    kMealData,
    kFingerStickData,
    kCustomEventData,
    kPredictedBloodGlucose,
    kPredictedInsulinX,
    NumDataSources
  };//enum DataSources

  enum EntryType
  {
    kNotSelected,
    kBolusTaken,
    kGlucoseEaten,
    kMeterReading,
    kMeterCalibration,
    kCgmsReading,
    kGenericEvent,
    kNumEntryType
  };//enum EntryType


  struct EventInformation
  {
    Wt::WDateTime dateTime;
    EntryType     type;
    double        value;
  };//struct EventInformation


  public:
    WtGui( const Wt::WEnvironment& env, DubUserServer &server );
    virtual ~WtGui();

    void saveModel( const std::string &fileName );
    void saveCurrentModel(); //calls saveModel( m_userDbPtr->currentFileName )
    const std::string &currentFileName();
    void deleteModelFile( const std::string &fileName );
    void setModel( const std::string &fileName );
    void setModelFileName( const std::string &fileName );
    std::string formFileSystemName( const std::string &internalName );
    void enableOpenModelDialogOkayButton( Wt::WPushButton *button, Wt::WTableView *view );

    //requireLogin() will NOT require you to type a password if detects you
    //  are already logged in through the use of cookies
    void requireLogin();
    void init( const std::string username );
    void logout();
    void checkLogout( Wt::WString username );

    void resetGui();
    void openModelDialog();
    void deleteModel( const std::string &model );

    void newModel();
    void updateDataRange();
    void zoomToFullDateRange();
    void syncDisplayToModel();
    void updateClarkAnalysis();
    void updateClarkAnalysis( const ConsentrationGraph &xGraph,
                              const ConsentrationGraph &yGraph,
                              bool isCgmsVMeter,
                              bool isUsingCalibrationData );
    void updateDisplayedDateRange();
    void saveModelAsDialog();
    void saveModelConfirmation();

    void addDataDialog();
    void addData( EventInformation info );
    void addData( EntryType type, Wt::WFileUpload *fileUpload );

    void delRawData( Wt::WTableView *view );
    void enableRawDataDelButton( Wt::WTableView *view, Wt::WPushButton *button );


    boost::recursive_mutex &modelMutex() { return m_modelMutex; }
    DateTimeSelect *getBeginTimePicker() { return m_bsBeginTimePicker; }
    DateTimeSelect *getEndTimePicker() { return m_bsEndTimePicker; }


  private:
    //m_model should never be accessed in any situation where multithreaded
    //  access is any possibility, instead, a NLSimplePtr object should be
    //  instatiated and used for thread safety
    NLSimpleShrdPtr m_model;
    boost::recursive_mutex  m_modelMutex;

    Wt::Dbo::ptr<DubUser> m_userDbPtr;

    DubUserServer &m_server;
    boost::signals::connection m_logoutConnection;

    Wt::Dbo::backend::Sqlite3 m_dbBackend;
    Wt::Dbo::Session m_dbSession;

    Div  *m_upperEqnDiv;
    Wt::WPopupMenu *m_fileMenuPopup;
    Wt::WTabWidget *m_tabs;

    boost::shared_ptr<NLSimpleDisplayModel> m_nlSimleDisplayModel;

    Wt::WStandardItemModel     *m_bsModel;
    WChartWithLegend           *m_bsGraph;
    DateTimeSelect             *m_bsBeginTimePicker;
    DateTimeSelect             *m_bsEndTimePicker;

    Wt::WStandardItemModel     *m_errorGridModel;
    ClarkErrorGridGraph        *m_errorGridGraph;
    Div                        *m_errorGridLegend;

    Wt::WTableView             *m_rawDataView;
    Wt::WPushButton            *m_delDataButton;

    std::vector<WtConsGraphModel*> m_inputModels;


    friend class NLSimplePtr;
    //friend class DubEventEntry;
    //friend class WtGeneticallyOptimize;
};//class WtGui







class DubEventEntry : public Wt::WContainerWidget
{
  DateTimeSelect    *m_time;
  Wt::WComboBox     *m_type;
  Wt::WComboBox     *m_customTypes;
  Wt::WLineEdit     *m_value;
  Wt::WText         *m_units;
  Wt::WPushButton   *m_button;
  Wt::WCheckBox     *m_saveModel;
  Wt::Signal<WtGui::EventInformation> m_signal;
  WtGui             *m_wtgui;

  void typeChanged();
  void reset();
  void emitEntered();

  void setTimeToNow();
  void setTimeToLastData();


public:
  DubEventEntry( WtGui *wtguiparent, Wt::WContainerWidget *parent = NULL );
  virtual ~DubEventEntry();

   Wt::Signal<WtGui::EventInformation> &entered();
};//class DubEventEntry


class ClarkErrorGridGraph: public Wt::Chart::WCartesianChart
{
public:
  ClarkErrorGridGraph( Wt::Chart::ChartType type, Wt::WContainerWidget *parent = NULL );
  virtual ~ClarkErrorGridGraph(){}

protected:
  virtual void 	paint( Wt::WPainter &painter, const Wt::WRectF &rectangle= Wt::WRectF() ) const;
private:
  Wt::WLineF line( const double &x1, const double &y1,
                   const double &x2, const double &y2 ) const;
  Wt::WRectF rect( const double &x1, const double &y1 ) const;
};//class ClarkErrorGridGraph



class WtModelSettingsGui: public Wt::WContainerWidget
{
  //This class hooks the ModelSettings object (that is a member of NLSimple)
  //  up to a gui so a user can edit the settings of the ModelSettings object.
  //  Note that m_endTrainingTime, and m_startTrainingTime are not hooked up.

private:
  typedef std::vector<MemVariableSpinBox *> SpinBoxes;

public:
  WtModelSettingsGui( ModelSettings *modelSettings,
                      Wt::WContainerWidget *parent = NULL  );
  virtual ~WtModelSettingsGui();

  Wt::Signal<> &changed();
  Wt::Signal<> &predictionChanged();

private:
  ModelSettings       *m_settings;
  SpinBoxes            m_memVarSpinBox;
  Wt::Signal<>         m_changed;            //emitted when any setting is changed
  Wt::Signal<>         m_predictionChanged;  //emitted when a setting that effects
                                             //   the predictions are changed
  void init();
  void emitChanged();
  void emitPredictionChanged();
};//class WtModelSettingsGui



class WtGeneticallyOptimize: public Wt::WContainerWidget
{
  WtGui *m_parentWtGui;

  Wt::WBorderLayout          *m_layout;
  Wt::WStandardItemModel     *m_graphModel;
  Wt::Chart::WCartesianChart *m_graph;

  Wt::WStandardItemModel     *m_chi2Model;
  Wt::Chart::WCartesianChart *m_chi2Graph;

  Wt::WPushButton *m_startOptimization;
  Wt::WPushButton *m_stopOptimization;
  Wt::WPushButton *m_minuit2Optimization;

  Wt::WCheckBox   *m_saveAfterEachGeneration;

  boost::mutex m_continueMutex;
  bool m_continueOptimizing;

  boost::mutex m_beingOptimizedMutex;
  std::vector<double> m_bestChi2;

  MemGuiTimeDate *m_endTrainingTimeSelect;
  MemGuiTimeDate *m_startTrainingTimeSelect;

public:
  WtGeneticallyOptimize( WtGui *wtGuiParent,
                         Wt::WContainerWidget *parent = NULL  );
  virtual ~WtGeneticallyOptimize();


  //below is meant to be called after each new generation of potential models
  //  has been evaluated, the continueOptimizing() fcn will return false if the
  //  user would like to stop the optimization process
  void optimizationUpdateFcn( double bestChi2 );
  bool continueOptimizing();
  void setContinueOptimizing( const bool doContinue );
  void startOptimization(); //call this to start the optimization
  void doGeneticOptimization();  //lanuched as a thread by startOptimization()
  void doMinuit2Optimization();
  void startMinuit2Optimization();
  void syncGraphDataToNLSimple();
  void updateDateSelectLimits();
};//class WtGeneticallyOptimize


class WtCustomEventTab: public Wt::WContainerWidget
{
protected:
  WtGui                          *m_parentWtGui;
  Wt::WBorderLayout              *m_layout;

  int m_currentCE;
  WtGeneralArrayModel            *m_currentCEModel;
  Wt::Chart::WCartesianChart     *m_currentCEChart;

  Wt::WTableView                 *m_eventTypesView;
  Wt::WStandardItemModel         *m_eventTypesModel;

public:
  WtCustomEventTab( WtGui *wtGuiParent,
                    Wt::WContainerWidget *parent = NULL );
  virtual ~WtCustomEventTab();

  int selectedEventType() const;

  void updateAvailableEventTypes();
  void displaySelectedModel();


  void addCustomEventDialog();
  void defineDefaultDexcomEvents();
  void validateCustomEventNameAndID( Wt::WLineEdit *name,
                                     Wt::WSpinBox *id,
                                     Wt::WPushButton *button );
  void defineCustomEvent( const int recordType,
                          const std::string name,
                          const TimeDuration eventDuration,
                          const int eventDefType, //of type EventDefType, see ResponseModel.hh
                          const int nPoints );

  bool undefineCustomEvent( int index );
  void confirmUndefineCustomEventDialog(); //currently selected event type
};//class WtCustomEventTab:


#endif // WTGUI_H
