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

class DubUser;
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
};//namespace Wt

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
  void enableButton( Wt::WPushButton *button,
                     Wt::WLineEdit *edit1,
                     Wt::WLineEdit *edit2 = NULL,
                     Wt::WLineEdit *edit3 = NULL );
  void checkCredentials();
  bool validLogin( const std::string &user, std::string pass );
};//class DubsLogin : public Wt::WContainerWidget




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
  std::string email;
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
    Wt::Dbo::field(a, password,        "email" );
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



#endif // WTUSERMANAGMENT_HH
