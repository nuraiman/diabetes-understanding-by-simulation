#ifndef WTUSERMANAGMENT_HH
#define WTUSERMANAGMENT_HH

#include <string>
#include <Wt/WContainerWidget>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/WtSqlTraits>

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



#endif // WTUSERMANAGMENT_HH
