#ifndef DubsApplication_h
#define DubsApplication_h

#include <string>

#include <Wt/WApplication>
#include <Wt/WEnvironment>

#include <boost/thread.hpp>
#include <boost/signals/connection.hpp>

#include "dubs/DubUser.hh"
#include "dubs/DubsSession.hh"
#include "dubs/WtUserManagment.hh"



class DubsApplication : public Wt::WApplication
{
public:
  DubsApplication( const Wt::WEnvironment& env,
           DubUserServer &server,
           const std::string &databaseName );
  virtual ~DubsApplication();

  void showAppScreen();
  void showLoginScreen();

  void logout();
  void setupAfterLoginStatusChange();
  void checkLogout( const std::string &username );

protected:
  DubsSession m_dubsSession;
  Wt::Dbo::ptr<DubUser> m_userDbPtr;

  DubUserServer &m_server;
  boost::recursive_mutex  m_mutex;
  boost::signals::connection m_logoutConnection;
};//class DubsApplication


#endif  //#ifndef DubsApplication_h
