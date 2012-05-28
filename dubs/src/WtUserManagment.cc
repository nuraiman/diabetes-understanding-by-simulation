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
#include <Wt/SyncLock>
#include <Wt/Auth/AuthWidget>
#include <Wt/Auth/PasswordService>


#include "TH1.h"
#include "TH1F.h"
#include "TH2.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TRandom3.h"
#include "TSystem.h"


#include "WtUserManagment.hh"
#include "ArtificialPancrease.hh"
#include "dubs/DubUser.hh"

using namespace Wt;
using namespace std;



DBO_INSTANTIATE_TEMPLATES(UsersModel);
DBO_INSTANTIATE_TEMPLATES(OptimizationChi2);


DubUserServer::DubUserServer()
  : m_userLoggedIn(this),
    m_userLoggedOut(this)
{
}

bool DubUserServer::login(const std::string& user, const std::string &sessionId )
{
  SyncLock<boost::recursive_mutex::scoped_lock> lock(m_mutex);

  const bool wasLoggedIn = (m_users.find(user) != m_users.end());
  if( wasLoggedIn )
    cerr << "Forcing user " << user << " to logout" << endl;
  m_userLoggedIn.emit(user);
  m_users[user] = sessionId;
  return !wasLoggedIn;
}//...login( user )


const std::string DubUserServer::sessionId( const std::string user ) const
{
  UserToSessionMap::const_iterator iter = m_users.find(user);
  if( iter == m_users.end() )
    return "";
  return iter->second;
}//const string sessionId( const Wt::WString &user ) const


void DubUserServer::logout(const std::string& user)
{
  SyncLock<boost::recursive_mutex::scoped_lock> lock(m_mutex);

  UserToSessionMap::iterator i = m_users.find(user);

  if (i != m_users.end() )
  {
    cerr << "\nlogging out " << user << " sessionID=" << i->second << endl;
    m_users.erase(i);
    m_userLoggedOut.emit(user);
  }
}//logout( user )


DubUserServer::UserToSessionMap DubUserServer::users()
{
  SyncLock<boost::recursive_mutex::scoped_lock> lock(m_mutex);

  return m_users;
}//UserToSessionMap users()


