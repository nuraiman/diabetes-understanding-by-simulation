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
#include <Wt/WContainerWidget>
#include <Wt/Dbo/Session>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Chart/WCartesianChart>
#include <Wt/WLineF>
#include <Wt/WRectF>
#include <Wt/Dbo/QueryModel>

#include "ArtificialPancrease.hh"
#include "WtUserManagment.hh"
#include "dubs/DubsSession.hh"

//Some forward declarations
class NLSimple;
class WtGui;
class Div;
class DateTimeSelect;
class DubEventEntry;

class DubUser;
class UsersModel;
class TimeTextPair;
class ModelSettings;
class WtConsGraphModel;
class WtTimeRangeVecModel;
class ConsentrationGraph;

class ClarkErrorGridGraph;
class WtModelSettingsGui;
class MemVariableSpinBox;
class MemGuiTimeDate;
class OverlayCanvas;

class WtNotesTab;
class NLSimplePtr;
class WtCreateNLSimple;
class WChartWithLegend;
class WtNotesVectorModel;
class WtGeneralArrayModel;
class NLSimpleDisplayModel;
class WtExcludeTimeRangesTab;

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
  class WTextArea;
  class WPopupMenu;
  class WTableView;
  class WTabWidget;
  class WFileUpload;
  class WDatePicker;
  class WBorderLayout;
  class WStandardItemModel;
}//namespace Wt



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






class WtGui : public Wt::WContainerWidget
{
  //TODO 20120527
  //Remove all calls to WMesageBox::exec(...) - it causes deadlock if you force logout user
  //Seperate the WApplication logic from the user logic

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
    WtGui( Wt::Dbo::ptr<DubUser> user, Wt::WApplication *app, Wt::WContainerWidget *parent = 0 );
    virtual ~WtGui();

    void saveModel( const std::string &fileName );
    void saveCurrentModel(); //calls saveModel( m_userDbPtr->currentFileName )
    const std::string &currentFileName();
    void deleteModelFile( const std::string &fileName );
    void setModel( const std::string &fileName );
    void setModelFileName( const std::string &fileName );
    std::string formFileSystemName( const std::string &internalName );
    void enableOpenModelDialogOkayButton( Wt::WPushButton *button, Wt::WTableView *view );


    void init();
    void resetGui();
    void openModelDialog();
    void finishOpenModelDialog( Wt::WDialog *dialog, Wt::WTableView *view );
    void deleteModel( const std::string &model );

    void newModel();
    void createNLSimpleDialogFinished( WtCreateNLSimple *creator,  Wt::WDialog *dialog );

    void userDragZoomedBsGraph( int x0, int y0, Wt::WMouseEvent event );
    void updateDataRange();
    void zoomToFullDateRange();
    void zoomMostRecentDay();
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

    void refreshInsConcFromBoluses();
    void refreshClucoseConcFromMealData();

    void showNextTimePeriod();
    void showPreviousTimePeriod();

    Wt::WApplication *app() { return m_app; }
    boost::recursive_mutex &modelMutex() { return m_modelMutex; }
    DateTimeSelect *getBeginTimePicker() { return m_bsBeginTimePicker; }
    DateTimeSelect *getEndTimePicker() { return m_bsEndTimePicker; }

    boost::shared_ptr<NLSimpleDisplayModel> getSimpleSimDisplayModel() { return m_nlSimleDisplayModel; }

    void tabClickedCallback( int clickedINdex );

    Wt::Dbo::ptr<DubUser> dubUserPtr(){ return m_userDbPtr; }
//    Wt::Dbo::Session &dbSession() { return m_dubsSession; }

  private:

    Wt::WApplication *m_app;

    //m_model should never be accessed in any situation where multithreaded
    //  access is any possibility, instead, a NLSimplePtr object should be
    //  instatiated and used for thread safety
    NLSimpleShrdPtr m_model;
    boost::recursive_mutex  m_modelMutex;

    Wt::Dbo::ptr<DubUser> m_userDbPtr;



    Div  *m_upperEqnDiv;
    Wt::WPopupMenu *m_fileMenuPopup;
    Wt::WTabWidget *m_tabs;

    boost::shared_ptr<NLSimpleDisplayModel> m_nlSimleDisplayModel;

    Wt::WStandardItemModel     *m_bsModel;
    WChartWithLegend           *m_bsGraph;
    OverlayCanvas              *m_bsGraphOverlay;
    DateTimeSelect             *m_bsBeginTimePicker;
    DateTimeSelect             *m_bsEndTimePicker;

    Wt::WStandardItemModel     *m_errorGridModel;
    ClarkErrorGridGraph        *m_errorGridGraph;
    Div                        *m_errorGridLegend;

    Wt::WPushButton            *m_nextTimePeriodButton;
    Wt::WPushButton            *m_previousTimePeriodButton;

    WtNotesTab                 *m_notesTab;
    WtExcludeTimeRangesTab     *m_excludeTimeRangeTab;

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
  Wt::Dbo::QueryModel< Wt::Dbo::ptr<OptimizationChi2> > *m_chi2DbModel;

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
                                     Wt::WAbstractSpinBox *id,
                                     Wt::WPushButton *button );
  void defineCustomEvent( const int recordType,
                          const std::string name,
                          const TimeDuration eventDuration,
                          const int eventDefType, //of type EventDefType, see ResponseModel.hh
                          const int nPoints );

  bool undefineCustomEvent( int index );
  void confirmUndefineCustomEventDialog(); //currently selected event type
};//class WtCustomEventTab:


class WtNotesTab : public Wt::WContainerWidget
{
protected:
  WtGui                      *m_parentWtGui;
  DateTimeSelect             *m_dateSelect;
  WtNotesVectorModel         *m_model;
  Wt::WTableView             *m_tableView;
  Wt::WTextArea              *m_textArea;

  TimeTextPair *m_beingEdited;  //this should really be replaced with an index to the point being edited
                                //  as the pointer can change if an element is added/removed

  Wt::WCheckBox   *m_saveCheckBox;
  Wt::WPushButton *m_newNoteButton;
  Wt::WPushButton *m_saveButton;
  Wt::WPushButton *m_cancelButton;
  Wt::WPushButton *m_deleteButton;


public:
  WtNotesTab( WtGui *wtGuiParent, Wt::WContainerWidget *parent = NULL );
  virtual ~WtNotesTab(){}

  void newEntry();
  void cancelEdit();
  void saveCurrent( const bool askUserFirst = true );
  void removeCurrentEntry();
  void handleSelectionChange();

  void updateViewTable();
};//class WtNotesTab : public Wt::WContainerWidget


class WtExcludeTimeRangesTab : public Wt::WContainerWidget
{
protected:
  WtGui                      *m_parentWtGui;

  WChartWithLegend           *m_chart;
  NLSimpleDisplayModel       *m_displayModel;

  Wt::WTableView             *m_view;
  WtTimeRangeVecModel        *m_listModel;

  Wt::WPushButton            *m_deleteButton;
  Wt::WPushButton            *m_enableAddNewRangeButton;

  DateTimeSelect             *m_startExcludeSelect;
  DateTimeSelect             *m_endExcludeSelect;
  Wt::WPushButton            *m_addRangeButton;
  Wt::WCheckBox              *m_saveModel;

  Wt::WText                  *m_description;

public:
  WtExcludeTimeRangesTab( WtGui *parentWtGui, Wt::WContainerWidget *parent = NULL );
  virtual ~WtExcludeTimeRangesTab(){}

  void displaySelected();
  void allowUserToEnterNewRange();
  void addEnteredRangeToModel();
  void deleteSelectedRange();
  void finishDeleteSelectedDialog( Wt::WDialog *dialog,
                                   const Wt::WModelIndex selected,
                                   Wt::WCheckBox *save );
  void updateGraphWithUserRange();
  void updateDataRangeDates();
};//class WtExcludeTimeRangesTab : public Wt::WContainerWidget



#endif // WTGUI_H
