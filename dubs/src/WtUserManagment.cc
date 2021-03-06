#include "DubsConfig.hh"

#include <set>
#include <cmath>
#include <math.h>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <Wt/WText>
#include <Wt/WTable>
#include <Wt/WLabel>
#include <Wt/WDialog>
#include <Wt/SyncLock>
#include <Wt/WLineEdit>
#include <Wt/WTabWidget>
#include <Wt/WTableView>
#include <Wt/WDatePicker>
#include <Wt/WPushButton>
#include <Wt/WEnvironment>
#include <Wt/WApplication>
#include <Wt/WBorderLayout>
#include <Wt/Dbo/QueryModel>
#include <Wt/Auth/AuthWidget>
#include <Wt/WContainerWidget>
#include <Wt/WRegExpValidator>
#include <Wt/WLengthValidator>
#include <Wt/WStandardItemModel>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Auth/PasswordService>

#if(USE_CERNS_ROOT)
#include "TH1.h"
#include "TH2.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TH1F.h"
#include "TSystem.h"
#include "TRandom3.h"
#endif  //#if(USE_CERNS_ROOT)

#include "dubs/DubUser.hh"
#include "dubs/WtUserManagment.hh"
#include "dubs/ArtificialPancrease.hh"

using namespace Wt;
using namespace std;

#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH


DBO_INSTANTIATE_TEMPLATES(UsersModel);
DBO_INSTANTIATE_TEMPLATES(OptimizationChi2);
DBO_INSTANTIATE_TEMPLATES(ModelDisplayOptions);


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


