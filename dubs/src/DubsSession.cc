/*
This file is based almost enttirely off of Session.C file/class of 'auth1' example of Wt
which is Copyright (C) 2008 Emweb bvba, Kessel-Lo, Belgium.
*/

#include "dubs/DubsSession.hh"
#include "dubs/WtUserManagment.hh"

#include <boost/thread/mutex.hpp>

#include "Wt/Auth/AuthService"
#include "Wt/Auth/HashFunction"
#include "Wt/Auth/PasswordService"
#include "Wt/Auth/PasswordStrengthValidator"
#include "Wt/Auth/PasswordVerifier"
#include "Wt/Auth/GoogleService"
#include "Wt/Auth/FacebookService"
#include "Wt/Auth/Dbo/AuthInfo"
#include "Wt/Auth/Dbo/UserDatabase"

namespace {

  class MyOAuth : public std::vector<const Wt::Auth::OAuthService *>
  {
  public:
    ~MyOAuth()
    {
      for (unsigned i = 0; i < size(); ++i)
  delete (*this)[i];
    }
  };

  Wt::Auth::AuthService myAuthService;
  Wt::Auth::PasswordService myPasswordService(myAuthService);
  MyOAuth myOAuthServices;

}

void DubsSession::configureAuth()
{
  static boost::mutex mutex;
  static bool configured = false;

  boost::mutex::scoped_lock lock( mutex );

  if( configured )
    return;

  configured = true;

  myAuthService.setAuthTokensEnabled(true, "logincookie");
  myAuthService.setEmailVerificationEnabled(true);

  myAuthService.setIdentityPolicy( Wt::Auth::EmailAddressIdentity  );

  Wt::Auth::PasswordVerifier *verifier = new Wt::Auth::PasswordVerifier();
  verifier->addHashFunction(new Wt::Auth::BCryptHashFunction(7));
  myPasswordService.setVerifier(verifier);
  myPasswordService.setAttemptThrottlingEnabled(true);
  myPasswordService.setStrengthValidator
    (new Wt::Auth::PasswordStrengthValidator());

  if (Wt::Auth::GoogleService::configured())
    myOAuthServices.push_back(new Wt::Auth::GoogleService(myAuthService));

  if (Wt::Auth::FacebookService::configured())
    myOAuthServices.push_back(new Wt::Auth::FacebookService(myAuthService));
}

DubsSession::DubsSession(const std::string& sqliteDb)
  : connection_(sqliteDb)
{
  connection_.setProperty("show-queries", "true");

  setConnection(connection_);

  mapClass<AuthInfo>("auth_info");
  mapClass<AuthInfo::AuthIdentityType>("auth_identity");
  mapClass<AuthInfo::AuthTokenType>("auth_token");
  mapClass<DubUser>("DubUser");
  mapClass<UsersModel>("UsersModel");
  mapClass<OptimizationChi2>("OptimizationChi2");

  users_ = new UserDatabase( *this );

  dbo::Transaction transaction( *this );

  try {
    createTables();
    std::cerr << "DubsSession: Created database." << std::endl;

/*
    //Add a default guest/guest account
    Auth::User guestUser = users_->registerNew();
    guestUser.addIdentity(Auth::Identity::LoginName, "guest");
    myPasswordService.updatePassword(guestUser, "guest");
*/
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "DubsSession: Using existing database";
  }

  transaction.commit();
}

DubsSession::~DubsSession()
{
  delete users_;
}

Wt::Auth::AbstractUserDatabase& DubsSession::users()
{
  return *users_;
}

dbo::ptr<DubUser> DubsSession::user()
{
  if( login_.loggedIn() )
  {
    dbo::ptr<AuthInfo> authInfo = users_->find( login_.user() );
    dbo::ptr<DubUser> user = authInfo->user();

    if( !user )
    {
      dbo::Transaction transaction( *this );
      std::cerr << "First time user: " << login_.user().id() << std::endl;
      user = add( new DubUser() );
      user.modify()->name = login_.user().id();
      user.modify()->role = DubUser::Visitor;

      authInfo.modify()->setUser( user );
      transaction.commit();
    }//if (!user)

    return authInfo->user();
  }//if( login_.loggedIn() )

  return dbo::ptr<DubUser>();
}//user()

const Wt::Auth::AuthService& DubsSession::auth()
{
  return myAuthService;
}

const Wt::Auth::PasswordService& DubsSession::passwordAuth()
{
  return myPasswordService;
}

const std::vector<const Wt::Auth::OAuthService *>& DubsSession::oAuth()
{
  return myOAuthServices;
}
