//File/Class based heavily off Session.h file/class of 'auth1' example of Wt
#ifndef DubsSession_hh
#define DubsSession_hh

#include "DubsConfig.hh"

#include <string>

#include <Wt/Dbo/ptr>
#include <Wt/Auth/Login>
#include <Wt/Dbo/Session>
#include <Wt/Dbo/backend/Sqlite3>

#include "dubs/DubUser.hh"

namespace dbo = Wt::Dbo;

typedef Wt::Auth::Dbo::UserDatabase<AuthInfo> UserDatabase;

class DubsSession : public dbo::Session
{
public:
  static void configureAuth();

  DubsSession(const std::string& sqliteDb);
  ~DubsSession();

  dbo::ptr<DubUser> user();

  Wt::Auth::AbstractUserDatabase& users();
  Wt::Auth::Login& login() { return login_; }

  static const Wt::Auth::AuthService& auth();
  static const Wt::Auth::PasswordService& passwordAuth();
  static const std::vector<const Wt::Auth::OAuthService *>& oAuth();

private:
  dbo::backend::Sqlite3 connection_;
  UserDatabase *users_;
  Wt::Auth::Login login_;
};//class DubsSession

#endif // DubsSession_hh
