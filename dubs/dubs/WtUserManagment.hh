#ifndef WTUSERMANAGMENT_HH
#define WTUSERMANAGMENT_HH

#include <map>
#include <string>
#include <Wt/WContainerWidget>
#include <Wt/WString>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/WtSqlTraits>
#include <boost/thread.hpp>

#include "dubs/DubUser.hh"

class UsersModel;

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
}//namespace Wt

class DubUserServer : public Wt::WObject
{
  //This is a simple class adapted from the SimpleChat example included
  //  with Wt, to create a server such that a user may only be logged
  //  in once at a time.  At a minimum each WApplication session should
  //  be connected to the userLogIn() signal so that if the same user logs
  //  in elsewhere it can quit

public:
  DubUserServer();
  bool login(const std::string& user, const std::string &sessionId);
  void logout(const std::string& user);
  Wt::Signal<std::string> &userLogIn() { return m_userLoggedIn; }
  Wt::Signal<std::string> &userLogOut() { return m_userLoggedOut; }

  const std::string sessionId( const std::string user ) const;

  //typedef std::set<Wt::WString> UserSet;
  typedef std::map<std::string,std::string> UserToSessionMap;
  UserToSessionMap users();

private:
  Wt::Signal<std::string>       m_userLoggedIn;
  Wt::Signal<std::string>       m_userLoggedOut;
  boost::recursive_mutex        m_mutex;

  UserToSessionMap              m_users;
};//DubUserServer class



class SerializedModel;
class OptimizationChi2;

//USE_SerializedModel == true has yet to be tested
#define USE_SerializedModel 0

class UsersModel
{
public:
  std::string fileName;
  Wt::WDateTime created;
  Wt::WDateTime modified;

  Wt::Dbo::ptr<DubUser> user;
  Wt::Dbo::collection< Wt::Dbo::ptr<OptimizationChi2> > chi2s;

  Wt::WDateTime displayBegin;
  Wt::WDateTime displayEnd;

#if(USE_SerializedModel)
  Wt::Dbo::ptr<SerializedModel> serializedModel;
#else
  //TODO - serializedData should be stored in a seperate table, so this way
  //       everytime 'displayBegin' or 'displayEnd' is changed, the serialized
  //       data doesnt have to be changed
  std::string serializedData;
#endif

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, user,       "user");
    Wt::Dbo::field(a, fileName,       "fileName" );
    Wt::Dbo::field(a, created,        "created" );
    Wt::Dbo::field(a, modified,       "modified" );
    Wt::Dbo::field(a, displayBegin,   "displayBegin" );
    Wt::Dbo::field(a, displayEnd,     "displayEnd" );

#if(USE_SerializedModel)
    Wt::Dbo::field(a, serializedModel,"serializedModel" );
#else
    Wt::Dbo::field(a, serializedData, "serializedData" );
#endif
    Wt::Dbo::hasMany(a, chi2s, Wt::Dbo::ManyToOne, "usermodel");
  }//persist function
};//class UsersModel


class SerializedModel
{
public:
  Wt::Dbo::ptr<UsersModel> usermodel;
  std::string data;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, usermodel, "usermodel");
    Wt::Dbo::field(a, data, "data" );
  }//persist function
};//class SerializedModel


class OptimizationChi2
{
public:
  int generation;
  double chi2;

  Wt::Dbo::ptr<UsersModel> usermodel;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, usermodel, "usermodel");
    Wt::Dbo::field(a, generation, "generation" );
    Wt::Dbo::field(a, chi2,  "chi2" );
  }//presist function
};//class UsersModel


DBO_EXTERN_TEMPLATES(UsersModel);
DBO_EXTERN_TEMPLATES(SerializedModel);
DBO_EXTERN_TEMPLATES(OptimizationChi2);

#endif // WTUSERMANAGMENT_HH
