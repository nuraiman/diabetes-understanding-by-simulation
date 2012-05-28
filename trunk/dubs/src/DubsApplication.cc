
#include <string>
#include <iostream>

#include <Wt/WString>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/Auth/AuthWidget>
#include <Wt/Auth/PasswordService>


#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/signals/connection.hpp>

#include "dubs/DubUser.hh"
#include "dubs/DubsSession.hh"
#include "dubs/WtUserManagment.hh"
#include "dubs/DubsApplication.hh"

using namespace Wt;
using namespace std;


DubsApplication::DubsApplication( const Wt::WEnvironment& env,
                                  DubUserServer &server,
                                  const string &databaseName )
  : WApplication( env ),
    m_dubsSession( databaseName ),
    m_userDbPtr(),
    m_server( server ),
    m_mutex(),
    m_logoutConnection()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );

  enableUpdates( true );
  setTitle( "Diabetes Understanding By Simulation (dubs)" );

  //If we have deployed this app as a fastcgi, then we need to modify URL
  //  of the local_resource
  string urlStr = "local_resources/dubs_style.css";
  if( boost::algorithm::contains( url(), "dubs.app" )
      || boost::algorithm::contains( url(), "dubs.wt" ) )
    urlStr = "dubs/exec/" + urlStr;

  useStyleSheet( urlStr );

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
}//DubsApplication


void DubsApplication::setupAfterLoginStatusChange()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );
  const Wt::Auth::LoginState loginStatus = m_dubsSession.login().state();

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
  root()->clear();
}//void showAppScreen()


void DubsApplication::showLoginScreen()
{
  boost::recursive_mutex::scoped_lock lock( m_mutex );
  root()->clear();

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
  }//if( username.narrow() == m_userDbPtr->name )
}//void checkLogout( Wt::WString username )


