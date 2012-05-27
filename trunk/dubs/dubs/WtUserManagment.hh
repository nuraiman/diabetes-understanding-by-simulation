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
  bool login(const Wt::WString& user, const std::string &sessionId);
  void logout(const Wt::WString& user);
  Wt::Signal<Wt::WString> &userLogIn() { return m_userLoggedIn; }
  Wt::Signal<Wt::WString> &userLogOut() { return m_userLoggedOut; }

  const std::string sessionId( const Wt::WString user ) const;

  //typedef std::set<Wt::WString> UserSet;
  typedef std::map<Wt::WString,std::string> UserToSessionMap;
  UserToSessionMap users();

private:
  Wt::Signal<Wt::WString>       m_userLoggedIn;
  Wt::Signal<Wt::WString>       m_userLoggedOut;
  boost::recursive_mutex        m_mutex;

  UserToSessionMap              m_users;
};//DubUserServer class



class OptimizationChi2;

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

  std::string serializedData;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, user,       "user");
    Wt::Dbo::field(a, fileName,       "fileName" );
    Wt::Dbo::field(a, created,        "created" );
    Wt::Dbo::field(a, modified,       "modified" );
    Wt::Dbo::field(a, displayBegin,   "displayBegin" );
    Wt::Dbo::field(a, displayEnd,     "displayEnd" );
    Wt::Dbo::field(a, serializedData, "serializedData" );
    Wt::Dbo::hasMany(a, chi2s, Wt::Dbo::ManyToOne, "usermodel");
  }//persist function
};//class UsersModel


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
DBO_EXTERN_TEMPLATES(OptimizationChi2);

#endif // WTUSERMANAGMENT_HH
