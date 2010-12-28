#ifndef WTGUI_H
#define WTGUI_H
#include <map>
#include <string>
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"
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

//Some forward declarations
class NLSimple;
class Div;
class DateTimeSelect;
class DubEventEntry;

class DubUser;
class UsersModel;
class ModelSettings;
class ConsentrationGraph;

class ClarkErrorGridGraph;
class WtModelSettingsGui;
class MemVariableSpinBox;


namespace Wt
{
  class WText;
  class WLineF;
  class WRectF;
  class WDialog;
  class WSpinBox;
  class WComboBox;
  class WLineEdit;
  class WDateTime;
  class WPopupMenu;
  class WTableView;
  class WTabWidget;
  class WDatePicker;
  class WBorderLayout;
  class WStandardItemModel;
};//namespace Wt




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
    WtGui( const Wt::WEnvironment& env );
    virtual ~WtGui(){ /*delete m_model;*/ }

    void saveModel( const std::string &fileName );
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

    void resetGui();
    void openModelDialog();
    void deleteModel( const std::string &model );

    void newModel();
    void updateDataRange();
    void zoomToFullDateRange();
    void syncDisplayToModel();
    void updateClarkAnalysis( const ConsentrationGraph &xGraph,
                              const ConsentrationGraph &yGraph,
                              bool isCgmsVMeter );
    void updateDisplayedDateRange();
    void saveModelAsDialog();
    void saveModelConfirmation();

    void addData( EventInformation info );

  private:
    NLSimple *m_model;

    Wt::Dbo::ptr<DubUser> m_userDbPtr;


    Wt::Dbo::backend::Sqlite3 m_dbBackend;
    Wt::Dbo::Session m_dbSession;

    Div  *m_upperEqnDiv;
    Wt::WPopupMenu *m_actionMenuPopup;
    Wt::WTabWidget *m_tabs;

    Wt::WStandardItemModel     *m_bsModel;
    Wt::Chart::WCartesianChart *m_bsGraph;
    DateTimeSelect             *m_bsBeginTimePicker;
    DateTimeSelect             *m_bsEndTimePicker;

    Wt::WStandardItemModel     *m_errorGridModel;
    ClarkErrorGridGraph        *m_errorGridGraph;
    Div                        *m_errorGridLegend;

    Wt::WTableView             *m_customEventsView;
    Wt::WStandardItemModel     *m_customEventsModel;
    std::vector<ModelGraphPair> m_customEventGraphs;

    friend class DubEventEntry;
};//class WtGui








class DubEventEntry : public Wt::WContainerWidget
{
  DateTimeSelect    *m_time;
  Wt::WComboBox     *m_type;
  Wt::WLineEdit     *m_value;
  Wt::WText         *m_units;
  Wt::WPushButton   *m_button;
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



#endif // WTGUI_H
