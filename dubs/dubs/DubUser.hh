
#ifndef DUBUSER_HH
#define DUBUSER_HH

#include <string>

#include <Wt/Dbo/Types>
#include <Wt/WGlobal>

class UsersModel;
class DubUser;
typedef Wt::Auth::Dbo::AuthInfo<DubUser> AuthInfo;


class DubUser
{
public:
  enum Role
  {
    Visitor = 0,
    FullUser = 1
  };//enum Role

  std::string name;
  Role        role;
  std::string currentFileName;
  typedef Wt::Dbo::collection<Wt::Dbo::ptr<UsersModel> > UsersModels;
  UsersModels models;

  template<class Action>
  void persist(Action& a)
  {
    Wt::Dbo::field(a, name,            "name" );
    Wt::Dbo::field(a, role,            "role" );
    Wt::Dbo::field(a, currentFileName, "currentFileName" );
    Wt::Dbo::hasMany(a, models, Wt::Dbo::ManyToOne, "user");
  }//presist function
};//class DubUser

DBO_EXTERN_TEMPLATES(DubUser);

#endif //DUBUSER_HH
