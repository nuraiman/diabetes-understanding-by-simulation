#ifndef DubsApplication_h
#define DubsApplication_h

#include "DubsConfig.hh"

#include <string>

#include <Wt/WApplication>
#include <Wt/WEnvironment>

#include <boost/thread.hpp>
#include <boost/signals/connection.hpp>

#include "dubs/DubUser.hh"
#include "dubs/DubsSession.hh"
#include "dubs/WtUserManagment.hh"

class WtGui;

class DubsApplication : public Wt::WApplication
{
  //TODO: put a logout link in upper right hand corner
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

  bool isMobile() const;

protected:
  WtGui *m_gui;
  DubsSession m_dubsSession;

  DubUserServer &m_server;
  boost::recursive_mutex  m_mutex;
  boost::signals::connection m_logoutConnection;
};//class DubsApplication


#endif  //#ifndef DubsApplication_h
