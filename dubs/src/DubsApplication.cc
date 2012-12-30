#include "DubsConfig.hh"

#include <string>
#include <iostream>

#include <Wt/WString>
#include <Wt/WGridLayout>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/Auth/AuthWidget>
#include <Wt/Auth/PasswordService>

#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/signals/connection.hpp>

#include "dubs/WtGui.hh"
#include "dubs/DubUser.hh"
#include "dubs/DubsSession.hh"
#include "dubs/WtUserManagment.hh"
#include "dubs/DubsApplication.hh"

using namespace Wt;
using namespace std;

//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH



DubsApplication::DubsApplication( const Wt::WEnvironment& env,
                                  DubUserServer &server,
                                  const string &databaseName )
  : WApplication( env ),
    m_gui( NULL ),
    m_dubsSession( databaseName ),
    m_server( server ),
    m_mutex(),
    m_logoutConnection()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );


  try
  {
    cout << "\n\n\n\n\nenv.getCookie()=" << flush << env.getCookie("dubslogin") << endl;
  }catch(...)
  {
    cout << "....failed\n\n\n" << endl;
  }

  enableUpdates( true );
  setTitle( "Diabetes Understanding By Simulation (dubs)" );

  //If we have deployed this app as a fastcgi, then we need to modify URL
  //  of the local_resource
  string urlStr = "local_resources/dubs_style.css";
  if( (boost::algorithm::contains( url(), "dubs.app" )
      || boost::algorithm::contains( url(), "dubs.wt" ))
      && env.hostName().find( "localhost" )==string::npos )
    urlStr = "dubs/exec/" + urlStr;

  useStyleSheet( urlStr );

  if( isMobile() )
  {
    styleSheet().addRule( "input[type=\"text\"]", "font-size:0.95em;" );
    styleSheet().addRule( "button[type=\"button\"]", "font-size:0.9em;" );
  }//if( isMobile() )


  DubsSession::configureAuth();
  m_dubsSession.login().changed().connect( this, &DubsApplication::setupAfterLoginStatusChange );

  //Everytime a new user logs in m_server.userLogIn() signal is emitted, so
  //  that if the same user is already logged in, then all other instances
  //  of there session will be logged out
  m_logoutConnection.disconnect();
  m_logoutConnection = m_server.userLogIn().connect( this, &DubsApplication::checkLogout );

  setupAfterLoginStatusChange();
}//DubsApplication( const Wt::WEnvironment& env, DubUserServer &server )


DubsApplication::~DubsApplication()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );
  cout << "DubsApplication: terminating session" << endl;

  m_logoutConnection.disconnect();
  if( m_dubsSession.user() )
    m_server.logout( m_dubsSession.user()->name );
}//DubsApplication


void DubsApplication::setupAfterLoginStatusChange()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );
  Wt::Auth::LoginState loginStatus = m_dubsSession.login().state();

  /*
  const Wt::Auth::AuthService &auth = DubsSession::auth();

  Wt::WEnvironment::CookieMap cookies = environment().cookies();
  cerr << "\n\n\nThere are " << cookies.size() << " cookies\n\n" << endl;
  foreach( const Wt::WEnvironment::CookieMap::value_type &a, cookies )
    cerr << "\t" << a.first << " - " << a.second << endl;
  cerr << endl << endl;

  switch( loginStatus )
  {
    case Wt::Auth::LoggedOut:
    case Wt::Auth::DisabledLogin:
      if( auth.authTokensEnabled() )
      {
        cerr << "\n\n\n\nauth.authTokensEnabled() - and not logged in" << endl;
        Auth::AbstractUserDatabase &users = m_dubsSession.users();

        const string cookieName = auth.authTokenCookieName();

        try
        {
          const string authToken = environment().getCookie( cookieName );
          const Auth::AuthTokenResult authRes = auth.processAuthToken( authToken, users );
          cerr << "Got cookie " << cookieName << endl;

          if( authRes.result() == Auth::AuthTokenResult::Valid )
          {
            cerr << "Auth results were valid" << endl;
            const Auth::User &user = authRes.user();
            setCookie( cookieName, authRes.newToken(), auth.authTokenValidity() );
            m_dubsSession.login().login( user, Auth::StrongLogin );
            loginStatus = m_dubsSession.login().state();
          }//if( authRes.result() == Auth::AuthTokenResult::Valid )
          else
            cerr << "Auth results were NOT-valid" << endl;
        }catch(...)
        {
          cerr << "There was no cookie named " << cookieName << endl;
        }
      }//if( auth.authTokensEnabled() )
    break;

    case Wt::Auth::WeakLogin:
    case Wt::Auth::StrongLogin:
    if( auth.authTokensEnabled() )
    {
      //XXX - right now I'm having
      cerr << "\n\n\n\nauth.authTokensEnabled() - and logged in - will set cookie" << endl;
      const Auth::User &user  = m_dubsSession.login().user();
      const string cookieName = auth.authTokenCookieName();
      const string authToken  = auth.createAuthToken( user );
//      setCookie( cookieName, authToken, auth.authTokenValidity() );
      setCookie( cookieName, authToken, 5000000, "localhost:8080", "" );
      Wt::WEnvironment::CookieMap cookies = environment().cookies();
      cerr << "\tthere are now " << cookies.size() << " cookies\n\n" << endl;
      cerr << "Setting cookie " << cookieName << " to " << authToken << endl;
    }//if( auth.authTokensEnabled() )
    break;
  }//switch( loginStatus )
*/


  switch( loginStatus )
  {
    case Wt::Auth::LoggedOut:
    case Wt::Auth::DisabledLogin:
      showLoginScreen();
    break;

    case Wt::Auth::WeakLogin:
    case Wt::Auth::StrongLogin:
      showAppScreen();
    break;
  }//switch( loginStatus )

}//void setupAfterLoginStatusChange()


void DubsApplication::showAppScreen()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );

  const Dbo::ptr<DubUser> user = m_dubsSession.user();

  string username;

  {
    Dbo::Transaction transaction( m_dubsSession );
    username = user->name;
    transaction.commit();
  }


  if( !user )
  {
    cerr << SRC_LOCATION <<"\n\tI shouldnt ever be here" << endl;
    showLoginScreen();
    return;
  }//if( !m_dubsSession.user() )

  if( m_server.sessionId(username) != sessionId() )
    m_server.login( username, sessionId() );

  root()->clear();
  WGridLayout *layout = new WGridLayout();
  root()->setLayout( layout );

  m_gui = new WtGui( user, this );
  layout->addWidget( m_gui, 0, 0, 1, 1 );
  layout->setColumnStretch( 0, 5 );
  layout->setRowStretch( 0, 5 );
}//void showAppScreen()


void DubsApplication::showLoginScreen()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );

//  if( m_gui )
//    delete m_gui;

  root()->clear();
  m_gui = NULL;

  Wt::Auth::AuthWidget *authWidget
        = new Wt::Auth::AuthWidget( DubsSession::auth(),
                                    m_dubsSession.users(),
                                    m_dubsSession.login() );

  authWidget->model()->addPasswordAuth( &DubsSession::passwordAuth() );
  authWidget->model()->addOAuth( DubsSession::oAuth() );
  authWidget->setRegistrationEnabled( true );

  authWidget->processEnvironment();

  root()->addWidget( authWidget );
}//void WtGui::showLoginScreen()



void DubsApplication::logout()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );

  m_dubsSession.login().logout();
  setupAfterLoginStatusChange();
}//void logout()


void DubsApplication::checkLogout( const std::string &username )
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );

  if( m_dubsSession.user() && (username == m_dubsSession.user()->name) )
  {
    {
      WApplication::UpdateLock lock( this );
//      redirect( "static_pages/forced_logout.html" );
      logout();
      triggerUpdate();
    }
//    quit();
  }//if( username is trying to login again )

}//void checkLogout( Wt::WString username )


bool DubsApplication::isMobile() const
{
  const WEnvironment &env = environment();
  const bool isMob = (env.agentIsMobileWebKit()
                         || env.agentIsIEMobile()
                         || env.userAgent().find("Opera Mobi") != std::string::npos
                        );
  return isMob;
}//bool isMobile() const

