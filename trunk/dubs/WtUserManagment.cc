#include <set>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fstream>
#include <cmath>

#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/algorithm/string.hpp"

#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WBorderLayout>
#include <Wt/WTabWidget>
#include <Wt/WStandardItemModel>
#include <Wt/WTableView>
#include <Wt/WDatePicker>
#include <Wt/WLabel>
#include <Wt/WDialog>
#include <Wt/WLengthValidator>
#include <Wt/Dbo/QueryModel>
#include <Wt/WLineEdit>
#include <Wt/WText>
#include <Wt/WTable>
#include <Wt/WPushButton>
#include <Wt/WEnvironment>
#include <Wt/WRegExpValidator>


#include "TH1.h"
#include "TH1F.h"
#include "TH2.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TRandom3.h"
#include "TSystem.h"


#include "WtUserManagment.hh"
#include "ArtificialPancrease.hh"


using namespace Wt;
using namespace std;


const std::string DubsLogin::cookie_name = "dubsuserid";


DubsLogin::DubsLogin(  Wt::Dbo::Session &dbSession, WContainerWidget *parent ):
   WContainerWidget( parent ),
   m_dbSession( dbSession ),
   m_introText( NULL ),
   m_username( NULL ),
   m_password( NULL )
{
  setInline(false);
  setStyleClass( "login_box" );
   //if( WServer::httpPort() != .... ) then don't serve the page, redirect to a static error page

   WText *title = new WText("Login", this);
   title->decorationStyle().font().setSize(WFont::XLarge);

   m_introText = new WText("<p>Welcome to Diabetes Understanding by Simulation</p>", this);

   WTable *layout = new WTable(this);
   WLabel *usernameLabel = new WLabel("User name: ", layout->elementAt(0, 0));
   layout->elementAt(0, 0)->resize(WLength(14, WLength::FontEx), WLength::Auto);
   m_username = new WLineEdit( layout->elementAt(0, 1) );
   usernameLabel->setBuddy(m_username);

   WLabel *passwordLabel = new WLabel("Password: ", layout->elementAt(1, 0));
   m_password = new WLineEdit(layout->elementAt(1, 1));
   m_password->setEchoMode(WLineEdit::Password);
   passwordLabel->setBuddy(m_password);

   new WBreak(this);

   WPushButton *loginButton = new WPushButton("Login", this);
   WPushButton *addUserButton = new WPushButton("New User", this);
   m_username->setFocus();

   WApplication::instance()->globalEnterPressed().connect( this, &DubsLogin::checkCredentials );
   m_username->enterPressed().connect( this, &DubsLogin::checkCredentials );
   m_password->enterPressed().connect( this, &DubsLogin::checkCredentials );
   loginButton->clicked().connect( this, &DubsLogin::checkCredentials );
   addUserButton->clicked().connect( this, &DubsLogin::addUser );
}//DubsLogin( initializer )



std::string DubsLogin::isLoggedIn( Wt::Dbo::Session &dbSession )
{
  string value = "";
  try{ value = wApp->environment().getCookie( cookie_name ); }
  catch(...) { return ""; }

  const size_t pos = value.find( '_' );
  if( pos == string::npos ) return "";

  const string name = value.substr( 0, pos );
  const string hash = value.substr( pos+1, string::npos );

  string found_hash = "";

  Dbo::Transaction transaction(dbSession);
  Dbo::ptr<DubUser> user = dbSession.find<DubUser>().where("name = ?").bind( name );
  if( user ) found_hash = user->cookieHash;
  transaction.commit();

  if( (found_hash==hash) && (hash!="") ) return name;
  return "";
}//bool DubsLogin::checkForCookie()


void DubsLogin::insertCookie( const std::string &uname,
                              const int lifetime,
                              Wt::Dbo::Session &dbSession )
{
  static TRandom3 tr3(0);
  const string random_str = boost::lexical_cast<string>( tr3.Rndm() );
  TMD5 md5obj;
  md5obj.Update( (UChar_t *)random_str.c_str(), random_str.length() );
  md5obj.Final();
  string hash = md5obj.AsString();

  const string cookie_value = (lifetime > 0) ? (uname + "_" + hash) : string("");

  Dbo::Transaction transaction(dbSession);
  Dbo::ptr<DubUser> user = dbSession.find<DubUser>().where("name = ?").bind(uname);
  transaction.commit();

  if( !user )
  {
    wApp->doJavaScript( "alert( \"Couldn\'t find user" + uname + " in database to set cookie for\" )", true );
  }else
  {
    Dbo::Transaction transaction(dbSession);
    user.modify()->cookieHash = hash;
    transaction.commit();
    wApp->setCookie( cookie_name, cookie_value, lifetime ); //keep cookie for 25 days
  }//if( found the user ) / else

}//void DubsLogin::insertCookie()

void DubsLogin::enableButton( WPushButton *button, WLineEdit *edit1, WLineEdit *edit2, WLineEdit *edit3 )
{
  bool valid = true;
  valid = (valid && (edit1->validate() == WValidator::Valid));
  if( edit2 ) valid = (valid && (edit2->validate() == WValidator::Valid));
  if( edit3 ) valid = (valid && (edit3->validate() == WValidator::Valid));
  if( valid ) button->enable();
  else        button->disable();
}//void enableButton(...)


void DubsLogin::addUser()
{
  WDialog dialog( "Add User" );
  //dialog.setModal( false );
  new WText( "User Name: ", dialog.contents() );
  WLineEdit *userNameEdit = new WLineEdit( dialog.contents() );
  userNameEdit->setText( "" );
  WLengthValidator *val = new WLengthValidator( 3, 50 );
  userNameEdit->setValidator( val );
  new WText( " Password: ", dialog.contents() );
  WLineEdit *passwordEdit = new WLineEdit( dialog.contents() );
  new WBreak( dialog.contents() );
  WLabel *emailLabel = new WLabel( "Email Address: ", dialog.contents() );
  WLineEdit *emailEdit = new WLineEdit( dialog.contents() );
  emailEdit->setTextSize( 25 );
  WRegExpValidator *emailValidator = new WRegExpValidator( "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}" );
  emailEdit->setValidator( emailValidator );
  emailLabel->setBuddy( emailEdit );
  passwordEdit->setText( "" );
  val = new WLengthValidator( 3, 50 );
  passwordEdit->setValidator( val );

  WContainerWidget *newUserTextDiv = new WContainerWidget( dialog.contents() );
  newUserTextDiv->setStyleClass( "newUserTextDiv" );
  newUserTextDiv->setInline( false );

  new WText( "This will create a limited account"
             " untill you email"
             " <a href=\"mailto:wcjohnson@ucdavis.edu\">wcjohnson@ucdavis.edu</a>"
             " to give you full usage capabilities; sorry about this but some potential"
             " actions take a ton of cpu resources and Im running this off of my"
             " laptop right now, and I would like to make sure I can provide you"
             " with the information and help to effictively use this tool.",
             XHTMLUnsafeText, newUserTextDiv );

  WPushButton *ok = new WPushButton( "Ok", dialog.contents() );
  ok->disable();
  WPushButton *cancel = new WPushButton( "Cancel", dialog.contents() );

  userNameEdit->changed().connect( boost::bind( &DubsLogin::enableButton, this, ok, userNameEdit, passwordEdit, emailEdit ) );
  passwordEdit->changed().connect( boost::bind( &DubsLogin::enableButton, this, ok, userNameEdit, passwordEdit, emailEdit ) );
  emailEdit->changed().connect( boost::bind( &DubsLogin::enableButton, this, ok, userNameEdit, passwordEdit, emailEdit ) );

  userNameEdit->enterPressed().connect( passwordEdit, &WFormWidget::setFocus );
  passwordEdit->enterPressed().connect( emailEdit, &WFormWidget::setFocus );
  emailEdit->enterPressed().connect( ok, &WPushButton::setFocus );
  ok->clicked().connect( &dialog, &WDialog::accept );
  cancel->clicked().connect( &dialog, &WDialog::reject );
  userNameEdit->escapePressed().connect( &dialog, &WDialog::reject );
  passwordEdit->escapePressed().connect( &dialog, &WDialog::reject );
  emailEdit->escapePressed().connect( &dialog, &WDialog::reject );
  userNameEdit->setFocus();
  dialog.exec();

  if( dialog.result() == WDialog::Rejected )
  {
    //delete dialog;
    return;
  }//if user canceled

  const string name = userNameEdit->text().narrow();
  const string pword = passwordEdit->text().narrow();
  const string email = emailEdit->text().narrow();

  Dbo::ptr<DubUser> user;

  {
    Dbo::Transaction transaction(m_dbSession);
    user = m_dbSession.find<DubUser>().where("name = ?").bind( name );
    transaction.commit();
  }

  if( user || name.empty() || pword.empty() )
  {
    wApp->doJavaScript( "alert( \"User '" + name + "'' exists or invalid\" )", true );
    //delete dialog;
    //dialog = NULL;
    addUser();
  }else
  {
    TMD5 md5obj;
    md5obj.Update( (UChar_t *)pword.c_str(), pword.length() );
    md5obj.Final();
    const string pwordhash = md5obj.AsString();

    DubUser *user = new DubUser();
    user->name = name;
    user->password = pwordhash;
    user->email = email;
    user->currentFileName = "example";//ProgramOptions::ns_defaultModelFileName;
    const string example_file_name = "../data/" + name + "_example.dubm";
    gSystem->CopyFile( "../data/example.dubm", example_file_name.c_str(), kFALSE );
    user->cookieHash = "";

    //it's cheesy, but if the user name doedn't start with 'dub', make them a
    //  visitor, unless they email me to change it

    if( name.substr(0, 3) == "dub"  ) user->role = DubUser::FullUser;
    else                              user->role = DubUser::Visitor;
    //using boost::algorithm::ends_with;
    //if( ends_with( name, "guest" ) ) user->role = DubUser::Visitor;
    //else                             user->role = DubUser::FullUser;

    Dbo::ptr<DubUser> userptr;

    {
      Dbo::Transaction transaction(m_dbSession);
      userptr = m_dbSession.add( user );
      if( !transaction.commit() ) cerr << "\nDid not commit adding the user\n" << endl;
    }

    UsersModel *defaultModel  = new UsersModel();
    defaultModel->user        = userptr;
    defaultModel->fileName    = user->currentFileName;
    defaultModel->created     = WDateTime::fromPosixTime( kGenericT0 );
    defaultModel->modified    = WDateTime::fromPosixTime( kGenericT0 );

    {
      Dbo::Transaction transaction(m_dbSession);
      Dbo::ptr<UsersModel> usermodelptr = m_dbSession.add( defaultModel );
      if( !transaction.commit() ) cerr << "\nDid not commit adding the user model\n" << endl;
    }

  }//if( user exists ) / else

  //delete dialog;
  m_username->setText( name );
}//void addUser()


void DubsLogin::checkCredentials()
{
   const std::string user = m_username->text().narrow();
   const std::string pass = m_password->text().narrow();

   if( validLogin( user, pass ) )
   {
     insertCookie( user, 25*24*3600, m_dbSession );
     m_loginSuccessfulSignal.emit( user );
   } else
   {
      m_introText->setText("<p>You entered an invalid password or username."
                           "</p><p>Please try again.</p>" );
      m_introText->decorationStyle().setForegroundColor( Wt::WColor( 186, 17, 17 ) );
      m_username->setText("");
      m_password->setText("");
      WWidget *p = m_introText->parent();
      if( p ) p->decorationStyle().setBackgroundColor( Wt::WColor( 242, 206, 206 ) );
   }//if( username / password ) / else failure
}//void DubsLogin::checkCredentials()



bool DubsLogin::validLogin( const std::string &user, std::string pass )
{
  TMD5 md5obj;
  md5obj.Update( (UChar_t *)pass.c_str(), pass.length() );
  md5obj.Final();
  pass = md5obj.AsString();

  Dbo::Transaction transaction(m_dbSession);
  Dbo::ptr<DubUser> userptr = m_dbSession.find<DubUser>().where("name = ?").bind( user );
  transaction.commit();

  return (userptr && (userptr->password == pass));
}//bool validLogin( const std::string &user, std::string &pass );


Wt::Signal<std::string> &DubsLogin::loginSuccessful()
{
  return m_loginSuccessfulSignal;
}//loginSuccessful()
