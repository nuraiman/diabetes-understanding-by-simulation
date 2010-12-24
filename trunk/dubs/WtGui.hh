#ifndef WTGUI_H
#define WTGUI_H
#include <string>
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WDateTime>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/WtSqlTraits>
#include <Wt/Chart/WCartesianChart>
#include <Wt/WLineF>
#include <Wt/WRectF>


//Some forward declarations
class NLSimple;
class Div;
class DateTimeSelect;
class DubEventEntry;

class DubUser;
class UsersModel;
class ConsentrationGraph;

class ClarkErrorGridGraph;

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

/*
class NLSimple
{
public:
	std::string name;
	NLSimple( const std::string &n ) { name = n; }
	~NLSimple(){};
};
*/

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
};//class WtGui


class Div : public Wt::WContainerWidget
{
  public:
  Div( const std::string &styleClass = "",
       Wt::WContainerWidget *parent = NULL );
  virtual ~Div() {}
};//class Div : Wt::WContainerWidget


class DubsLogin : public Wt::WContainerWidget
{
public:
  static const std::string cookie_name;
  typedef Wt::Signal<std::string> LoginSignal;

  DubsLogin( Wt::Dbo::Session &dbSession, Wt::WContainerWidget *parent = NULL );
  LoginSignal &loginSuccessful();

  //The cookie valuesstored in the sqlite3 database
  //  web_app_login_info.sql3, were created using the command
  //CREATE TABLE cookie_hashes ( user char(50) primary key, hash char(50) );
  //  where the has is the MD5 hash of a random float generated at
  //  cookie creatioin time.  The cookie value is formated like
  //  <user>_<hash>, so that the cookie retrieved from the client must match
  //  the information in the database

  static void insertCookie( const std::string &uname,
                            const int lifetime,
                            Wt::Dbo::Session &dbSession ); //lifetime in seconds
  static std::string isLoggedIn( Wt::Dbo::Session &dbSession ); //returns username, or a blank string on not being logged in


private:
  Wt::Dbo::Session &m_dbSession;
  Wt::WText     *m_introText;
  Wt::WLineEdit *m_username;
  Wt::WLineEdit *m_password;
  LoginSignal    m_loginSuccessfulSignal;

  void addUser();
  void checkCredentials();
  bool validLogin( const std::string &user, std::string pass );
};//class DubsLogin : public Wt::WContainerWidget



class DateTimeSelect : public Wt::WContainerWidget
{
  Wt::WDatePicker *m_datePicker;
  Wt::WSpinBox    *m_hourSelect;
  Wt::WSpinBox    *m_minuteSelect;
  Wt::WDateTime   m_top;
  Wt::WDateTime   m_bottom;
  Wt::Signal<>     m_changed;

  void validate( bool emitChanged = false );

public:
  DateTimeSelect( const std::string &label,
                  const Wt::WDateTime &initialTime,
                  Wt::WContainerWidget *parent = NULL );
  virtual ~DateTimeSelect();

  void set( const Wt::WDateTime &dateTime );
  Wt::WDateTime dateTime() const;
  void setTop( const Wt::WDateTime &top );
  void setBottom( const Wt::WDateTime &bottom );
  const Wt::WDateTime &top() const;
  const Wt::WDateTime &bottom() const;

  Wt::Signal<> &changed();
};//class DateTimeSelect



class DubEventEntry : public Wt::WContainerWidget
{
  DateTimeSelect    *m_time;
  Wt::WComboBox     *m_type;
  Wt::WLineEdit     *m_value;
  Wt::WText         *m_units;
  Wt::WPushButton   *m_button;

  Wt::Signal<WtGui::EventInformation> m_signal;

  void typeChanged();
  void reset();
  void emitEntered();

public:
  DubEventEntry( Wt::WContainerWidget *parent = NULL );
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


class DubUser
{
public:
  enum Role
  {
    Visitor = 0,
    FullUser = 1,
  };//enum Role

  std::string name;
  std::string password;
  Role        role;
  std::string currentFileName;
  std::string cookieHash;
  typedef Wt::Dbo::collection<Wt::Dbo::ptr<UsersModel> > UsersModels;
  UsersModels models;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::field(a, name,            "name" );
    Wt::Dbo::field(a, password,        "password" );
    Wt::Dbo::field(a, role,            "role" );
    Wt::Dbo::field(a, currentFileName, "currentFileName" );
    Wt::Dbo::field(a, cookieHash,      "cookieHash" );
    Wt::Dbo::hasMany(a, models, Wt::Dbo::ManyToOne, "user");
  }//presist function
};//class DubUser


class UsersModel
{
public:
  std::string fileName;
  Wt::WDateTime created;
  Wt::WDateTime modified;

  Wt::Dbo::ptr<DubUser> user;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, user, "user");
    Wt::Dbo::field(a, fileName, "fileName" );
    Wt::Dbo::field(a, created,  "created" );
    Wt::Dbo::field(a, created,  "modified" );
  }//presist function
};//class UsersModel



#endif // WTGUI_H
