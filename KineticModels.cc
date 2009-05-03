#include <iostream>
#include "KineticModels.hh"
#include "RungeKuttaIntegrater.hh"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/foreach.hpp"
#include "boost/bind.hpp"
#include "boost/assign/list_of.hpp" //for 'list_of()'
#include "boost/assign/list_inserter.hpp"


using namespace std;
using namespace boost;
using namespace boost::posix_time;

//FastCarbAbsorbtion,
//A 1/2 hour absorbtion function, return carbs absorbed per minute
double FastCarbAbsorbtionFunc( double nCarbs, double nMinutes )
{
  static const ptime t0( gregorian::date(1972, gregorian::Jan, 1), time_duration( 0, 0, 0, 0) );
  static const double t_assent = 10.0;
  static const double t_dessent = 5.0;
  static const double v_max = 1.0;
  static const double k_gut_absoption = 1.0;
  static const double timeStep = 0.5; 
  
  static ConsentrationGraph  
  glucAbs = yatesGlucoseAbsorptionRate( t0, nCarbs, 
                                        t_assent, t_dessent, v_max, 
                                        k_gut_absoption, timeStep );
 
  return glucAbs.value( nMinutes );
}//FastCarbAbsorbtionFunc(...)


//MediumCarbAbsorbtion,
double MediumCarbAbsorbtionFunc( double nCarbs, double nMinutes )
{
  static const ptime t0( gregorian::date(1972, gregorian::Jan, 1), time_duration( 0, 0, 0, 0) );
  static const double t_assent = 45.0;
  static const double t_dessent = 6.0;
  static const double v_max = 0.8;
  static const double k_gut_absoption = 0.1;
  static const double timeStep = 1.0;
    
  static ConsentrationGraph  
  glucAbs = yatesGlucoseAbsorptionRate( t0, nCarbs, 
                                     t_assent, t_dessent, v_max, 
                                     k_gut_absoption, timeStep );
 
  return glucAbs.value( nMinutes );
}//MediumCarbAbsorbtionFunc(...)


double SlowCarbAbsorbtionFunc( double nCarbs, double nMinutes )
{
  static const ptime t0( gregorian::date(1972, gregorian::Jan, 1), time_duration( 0, 0, 0, 0) );
  static const double t_assent = 75.0;
  static const double t_dessent = 20.0;
  static const double v_max = 0.2;
  static const double k_gut_absoption = 0.03;
  static const double timeStep = 1.0;
   
  static ConsentrationGraph  
  glucAbs = yatesGlucoseAbsorptionRate( t0, nCarbs, 
                                        t_assent, t_dessent, v_max, 
                                        k_gut_absoption, timeStep );
 
  return glucAbs.value( nMinutes );
}//SlowCarbAbsorbtionFunc(...)




//The static pre-defined absorbtion functions
//NovologAbsorbtion,
double NovologConsentrationFunc( double unitsPerKilogram, double nMinutes )
{
  using namespace boost::assign;  //bring 'list_of()' into scope
  
  //Taken from handout that comes with the bottles of Humalog, 
  //  describes free serum insulin (in milli-Units per Liter) for
  //  a person injected with 0.15 Units of insulin per kilo-gram of body-wieght
  static const ptime t0( gregorian::date( 2002, gregorian::Jan, 10), time_duration(0,0,0,0) );
  
  //All the '-19.0' below is to makeup for what looks like the baseline insulin
  //  consentration in the chart.
  static const GraphElementSet data = list_of(GraphElement(0, 19.0-19.0))
               (15, 45.0-19.0) (30, 70.0-19.0) (40, 75.0-19.0) (45, 72.5-19.0)
               (60, 67.0-19.0) (75, 59.0-19.0) (90, 55.0-19.0) (105, 49.0-19.0)
               (120, 43.0-19.0) (135, 38.0-19.0) (150, 34.0-19.0) 
               (165, 30.0-19.0) (180, 26.0-19.0) (195, 22.5-19.0) 
               (210, 20.5-19.0) (225, 19.0-19.0);
               
  double consentration = 0.0;
  
  if( nMinutes < 0.0 || nMinutes > 225 ) return consentration;
  
  ConstGraphIter lb = data.lower_bound( GraphElement(nMinutes, 0.0) );
  ConstGraphIter ub = data.upper_bound( GraphElement(nMinutes, 0.0) );
  
  if( lb == ub ) //no exact match
  {
    assert( lb != data.begin() );
    assert( ub != data.end() );
    
    --lb;
    const double deltaTime = ub->m_minutes - lb->m_minutes;
    const double dV = ub->m_value - lb->m_value;
    const double fracTime = (nMinutes - lb->m_minutes) / deltaTime;
    
    consentration = lb->m_value + fracTime * dV;
  }else
  {
    consentration = lb->m_value;
  }//if( lb == ub ) / else
  
  consentration /= 0.15; //normailize consentration
  
  return unitsPerKilogram * consentration;
}//NovologConsentrationFunc(...)
    


ConsentrationGraph novologConsentrationGraph( 
                   boost::posix_time::ptime t0,
                   double unitsPerKilogram, double timeStep )
{
  ConsentrationGraph insConcen( t0, timeStep, InsulinGraph );
  
  double insulinConcentration = 0.0;
  for( double t=0.0; t<20.0 || insulinConcentration > 0.0; t += timeStep )
  {
    insulinConcentration = NovologConsentrationFunc( unitsPerKilogram, t );
    insConcen.insert( t, insulinConcentration );
  }//for( loop over time where insulin is active )
  
  return insConcen;
}//novologConsentrationGraph



double yatesGlucodeStomachToGutRate( double t, 
                   double glucose_equiv_carbs,
                   double t_assent,  //how long to get to max-stomach-absorption rate (minutes)
                   double t_dessent, //stomach-absorption rate tapper off time (minutes)
                   double v_max     //max stomach absorption rate (grams/minute)
                                   )
{
  if( t <= 0.0 ) return 0.0;
  
  const double assentArea = 0.5 * v_max * t_assent;
  const double dessentArea = 0.5 * v_max * t_dessent;
  const double max_area = glucose_equiv_carbs - dessentArea - assentArea;
  const double t_max = (glucose_equiv_carbs - assentArea - dessentArea) / v_max;
  const double t_total = t_assent + t_dessent + t_max;
  
  if( max_area >= 0.0 ) //trapazoid
  {
    if( t < t_assent )                return t * v_max / t_assent;
    else if( t < (t_max + t_assent) ) return v_max;
    else if( t < t_total )            return v_max * (1.0 - (t - t_max - t_assent) / t_dessent);
    else                              return 0.0;
  } else  //a triangle
  {
    const double tanAssent = v_max / t_assent;
    const double tanDescent = v_max / t_dessent;
    const double rizeTime = sqrt( 2 * glucose_equiv_carbs / (tanAssent*(1+(tanAssent/tanDescent))) );
    const double fallTime = rizeTime * tanAssent / tanDescent;
    const double maxHeight = rizeTime * tanAssent;
    
    if( t < rizeTime )                  return t * tanAssent;
    else if( t == rizeTime )            return 0.0;
    else if( t <= (rizeTime+fallTime) ) return maxHeight * ( 1.0 - (t - rizeTime) / fallTime);
    else return 0.0;
  }//if( nonDessentArea >= 0.0 )
    
  assert(0); //shouldn't have made it here
  return 0.0;
}//yatesGlucodeStomachToGutRate


double negativeForcingFunction( double t, ForcingFunction f )
{
  return -f(t);
}

std::vector<double> yatesCarbsEnterAndLeaveGutRate( double t, 
                                               std::vector<double> GutContents, 
                                               ForcingFunction stomachToGutRate,
                                               double k_gut_absoption )
{
  GutContents[0] = stomachToGutRate(t) - GutContents[0] * k_gut_absoption;
  return GutContents;
}//yatesGutToBloodAbsorbRate


//Implement Yates/Fletcher glucose absortption model
//  http://imammb.oxfordjournals.org/cgi/content/abstract/17/2/169
//  A 2-compartment model, 
//  1st-absoption from stomach to gut, paramaterized
//    as a trapeziodal function paramaterized by t_assent, t_dessent, v_max
//  2nd, absorption from gut to bloodstream; absorption is k_gut_absoption
//    * amount of glucose in gut
ConsentrationGraph yatesGlucoseAbsorptionRate( 
                   boost::posix_time::ptime t0,
                   double glucose_equiv_carbs,//Number of carbohydrates
                   double t_assent,  //how long to get to max-stomach-absorption rate (minutes)
                   double t_dessent, //stomach-absorption rate tapper off time (minutes)
                   double v_max,     //max stomach absorption rate (grams/minute)
                   double k_gut_absoption, //gut absorption constant
                   double timeStep  //timestep for numerical caculation
                                                   )
{  
  ConsentrationGraph gutAbs( t0, timeStep, GlucoseAbsorbtionRateGraph );
  
  //Define the function that says how fast the carbs enter the gut
  ForcingFunction stomachToGutRate = boost::bind( yatesGlucodeStomachToGutRate,
                                                  _1, glucose_equiv_carbs, 
                                                  t_assent, t_dessent, v_max );
  ForcingFunction negStomachToGutRate = boost::bind(negativeForcingFunction, 
                                                    _1, stomachToGutRate);
  //define function that says how fast carbs leave+enter the gut
  RK_DerivFuntion gutAbsRate = boost::bind( yatesCarbsEnterAndLeaveGutRate, 
                                                   _1, _2, stomachToGutRate, 
                                                   k_gut_absoption );
  const double epsilonGut = glucose_equiv_carbs / 10000.0;
  const double epsilonStomach = glucose_equiv_carbs / 1000.0;
    
  //Yes I know I should use a better integration method!!
  //  And I didn't pick variable name, the paper did
  double t;
  double G_stomach = glucose_equiv_carbs;
  vector<double> G_gut(1, 0.0);  //amount of carbs IN gut
  
  for( t = 0.0; (G_gut[0] > 0.0) || (G_stomach > 0.0); t += timeStep )
  {
    double G_in = k_gut_absoption * G_gut[0];
    gutAbs.insert( t, G_in );
   
    G_gut = rungeKutta4( t, G_gut, timeStep, gutAbsRate );
    G_stomach = rungeKutta4( t, G_stomach, timeStep, negStomachToGutRate );
    
    //Make sure we completely empty the stomach, which migh not otherwise
    //  hapen due to numerical integration not conserving amount of carbs
    if( (G_stomach < epsilonStomach) && (stomachToGutRate(t) == 0.0) )
    {
      G_gut[0] += G_stomach;
      G_stomach = 0.0;
    }//if( (t > 5.0) && (stomachToGutRate(t)==0.0) )
    
    //Just skip the last 0.01% of carbs in gut (it's a alow exp falloff)
    // also take care of numerical instabilities in the stomach
    if( G_stomach < epsilonStomach ) G_stomach = 0.0;
    if( (G_stomach==0.0) && (G_gut[0] < epsilonGut) ) G_gut[0] = 0.0;  
    
    // cout << "t=" << t << ", G_gut=" << G_gut[0]
         // << ", G_stomach=" << G_stomach <<  ", G_in=" << G_in << endl;
  }//for( while digesting food still )
  
  if( (--(gutAbs.end()))->m_value != 0.0 ) gutAbs.insert( t, 0.0 );
  // cout << "After " << t << " minutes, stomach has " << G_stomach << " carbs left, " 
       // << ", gut has " << G_gut << ", left; t_max was " << t_max << endl;
  
  return gutAbs;
}//ConsentrationGraph yatesGlucoseAbsorptionRate







double dG_dT( double time, 
              double X,    //This is 
              double G, 
              double G_basal, 
              double I_basal, 
              std::vector<double> G_parameter, 
              ForcingFunction D  //this is a function you supply
            )
{
  assert( G_parameter.size() == 2 );
  
  double dGdT = (-G_parameter[0]*G) - X*(G + G_basal) + G_parameter[1] * D(time);// + G_parameter[1]*I_basal;
    
  return dGdT;
}//dG_dT


double dX_dT( double time, 
              double X,
              double G,
              std::vector<double> X_parameters,
              ForcingFunction I   //this is a function you supply
            )
{
  G = G; //keep compiler from complaining
  assert( X_parameters.size() == 2 );
  //I is in Liters, while we want X in mU/dL
  
  return (-X_parameters[0] * X) + X_parameters[1]*I(time) / 10.0; //+ X_parameters[2] *(G - X_parameters[3]);
}//dX_dT


std::vector<double> dGdT_and_dXdT( double time, std::vector<double> G_and_X, 
                                   double G_basal, double I_basal, 
                                   std::vector<double> G_parameters,  //should be size==1
                                   std::vector<double> X_parameters,  //should be size==2
                                   ForcingFunction D,
                                   ForcingFunction I
                                 )
{
  vector<double> derivatives( 2, 0.0 );
  
  assert( G_and_X.size() == 2 );
  
  const double G = G_and_X[0];
  const double X = G_and_X[1];
  
  derivatives[0] = dG_dT( time, X, G, G_basal, I_basal, G_parameters, D );
  derivatives[1] = dX_dT( time, X, G, X_parameters, I );
  
  return derivatives;
}//dGdT_and_dXdT





