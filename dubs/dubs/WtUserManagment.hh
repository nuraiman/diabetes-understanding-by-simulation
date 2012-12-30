#ifndef WTUSERMANAGMENT_HH
#define WTUSERMANAGMENT_HH

#include "DubsConfig.hh"

#include <map>
#include <string>

#include <boost/thread.hpp>

#include <Wt/WString>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/WtSqlTraits>
#include <Wt/WContainerWidget>
#include <Wt/Dbo/backend/Sqlite3>

#include "dubs/DubUser.hh"

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
}//namespace Wt

class DubUserServer : public Wt::WObject
{
  //This is a simple class adapted from the SimpleChat example included
  //  with Wt, to create a server such that a user may only be logged
  //  in once at a time.  At a minimum each WApplication session should
  //  be connected to the userLogIn() signal so that if the same user logs
  //  in elsewhere it can quit

public:
  DubUserServer();
  bool login(const std::string& user, const std::string &sessionId);
  void logout(const std::string& user);
  Wt::Signal<std::string> &userLogIn() { return m_userLoggedIn; }
  Wt::Signal<std::string> &userLogOut() { return m_userLoggedOut; }

  const std::string sessionId( const std::string user ) const;

  //typedef std::set<Wt::WString> UserSet;
  typedef std::map<std::string,std::string> UserToSessionMap;
  UserToSessionMap users();

private:
  Wt::Signal<std::string>       m_userLoggedIn;
  Wt::Signal<std::string>       m_userLoggedOut;
  boost::recursive_mutex        m_mutex;

  UserToSessionMap              m_users;
};//DubUserServer class


class UsersModel;
class OptimizationChi2;
class ModelDisplayOptions;


namespace Wt
{
  namespace Dbo
  {
/*
    //Change the primary key of UsersModel to be a string in field name "fileName"
    template<>
    struct dbo_traits<UsersModel>
        : public dbo_default_traits
    {
      typedef std::string IdType;
      static IdType invalidId(){ return std::string(); }
      static const char *surrogateIdField() { return 0; }
    }; //struct dbo_traits<UsersModel>
*/

    //Emulate a OneToOne mapping of UsersModel to ModelDisplayOptions
    template<>
    struct dbo_traits<ModelDisplayOptions>
        : public dbo_default_traits
    {
      typedef ptr<UsersModel> IdType;
      static IdType invalidId(){ return ptr<UsersModel>(); }
      static const char *surrogateIdField() { return 0; }
    };//struct dbo_traits<ModelDisplayOptions>
  }//namespace Dbo
}//namespace Wt


class ModelDisplayOptions
{
public:
  Wt::WDateTime displayBegin;
  Wt::WDateTime displayEnd;

  Wt::Dbo::ptr<UsersModel> usermodel;

  template<class Action>
  void persist(Action& a)
  {
//    Wt::Dbo::belongsTo(a, usermodel, "usermodel");
    Wt::Dbo::id(a,    usermodel,     "usermodel", Wt::Dbo::OnDeleteCascade );  //Emulate a OneToOne mapping
    Wt::Dbo::field(a, displayBegin,  "displayBegin" );
    Wt::Dbo::field(a, displayEnd,    "displayEnd" );
  }//persist function
};//class ModelDisplayOptions


class UsersModel : public Wt::Dbo::Dbo<UsersModel>
{
  /*
    Problem: I want a OneToOne mapping between UsersModel and ModelDisplayOptions
             (so can modify ModelDisplayOptions w/o serializing NLSimple model),
             but Wt::Dbo doesnt provide a OneToOne mapping.
    Solution: Unresolved; using the dbo_traits... definition above we emulate a
              OneToOne mapping on the database side.
              I tried a few ways to emulate a OneToOne mapping on the c++ side,
              but wasnt happy with any of them yet.

  */
public:
  std::string fileName;
  Wt::WDateTime created;
  Wt::WDateTime modified;
  Wt::Dbo::ptr<DubUser> user;
  std::string serializedData;
  Wt::Dbo::collection< Wt::Dbo::ptr<OptimizationChi2> > chi2s;

  /*
  Wt::Dbo::ptr<ModelDisplayOptions> getDisplayOptions()
  {
    if( !session() )
    {
      if( displayOptions.size() )
        return displayOptions.front();
      return Wt::Dbo::ptr<ModelDisplayOptions>();
    }

    try
    {
      Wt::Dbo::ptr<ModelDisplayOptions> display_options
                                      = session()->find<ModelDisplayOptions>()
                                        .where("user_id = ?")
                                        .bind( self() )
                                        .resultValue();
      if( !display_options )
      {
        display_options = session()->add( new ModelDisplayOptions() );
        display_options->modify()->usermodel = self();
      }//if( !display_options )

      return display_options;
    }catch( Wt::Dbo::NoUniqueResultException &e )
    {
      assert( 0 );  //should never ever happen
    }
  }
  */

  Wt::Dbo::collection< Wt::Dbo::ptr<ModelDisplayOptions> > displayOptions;

public:
  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, user,       "user");
//    Wt::Dbo::id(a, fileName, "fileName"/*, 120*/ );
    Wt::Dbo::field(a, fileName,       "fileName" );
    Wt::Dbo::field(a, created,        "created" );
    Wt::Dbo::field(a, modified,       "modified" );
    Wt::Dbo::field(a, serializedData, "serializedData" );
    Wt::Dbo::hasMany(a, chi2s, Wt::Dbo::ManyToOne, "usermodel");

    // In fact, displayOptions is really constrained to hasOne() ...
    Wt::Dbo::hasMany(a, displayOptions, Wt::Dbo::ManyToOne, "usermodel");

    /*
    if( a.getsValue() )
    {
    }else if( a.setsValue() )
    {
    }
    */
  }//persist function
};//class UsersModel





class OptimizationChi2
{
public:
  int generation;
  double chi2;

  Wt::Dbo::ptr<UsersModel> usermodel;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::belongsTo(a, usermodel, "usermodel");
    Wt::Dbo::field(a, generation, "generation" );
    Wt::Dbo::field(a, chi2,  "chi2" );
  }//presist function
};//class UsersModel


DBO_EXTERN_TEMPLATES(UsersModel);
DBO_EXTERN_TEMPLATES(ModelDisplayOptions);
DBO_EXTERN_TEMPLATES(OptimizationChi2);

#endif // WTUSERMANAGMENT_HH
