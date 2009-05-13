#if !defined(KINETIC_MODEL_HH)
#define KINETIC_MODEL_HH


#include <vector>
#include "boost/function.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include "ArtificialPancrease.hh" //contains useful typedefs and constants

#include "ConsentrationGraph.hh"

//I'll put this stuff in a namepace later


//First some pre-defined models for prototyping/debugging
//Free plasma insulin consentration for Novolog, returns milli-Units per Liter
double NovologConsentrationFunc( double unitsPerKilogram, double nMinutes );

//Rate of Glucose entering bloodstream, returns grams
double FastCarbAbsorbtionFunc( double nCarbs, double nMinutes );
double MediumCarbAbsorbtionFunc( double nCarbs, double nMinutes );
double SlowCarbAbsorbtionFunc( double nCarbs, double nMinutes );


ConsentrationGraph novologConsentrationGraph( 
                   boost::posix_time::ptime t0,
                   double unitsPerKilogram, double timeStep );



//Implement Yates/Fletcher glucose absortption model
//  http://imammb.oxfordjournals.org/cgi/content/abstract/17/2/169
//  A 2-compartment model, 
//  1st-absoption from stomach to gut, paramaterized
//    as a trapeziodal function paramaterized by t_assent, t_dessent, v_max
//  2nd, absorption from gut to bloodstream; absorption is k_gut_absoption
//    * amount of glucose in gut
//right now just using simple numerical integration
ConsentrationGraph yatesGlucoseAbsorptionRate( 
                   boost::posix_time::ptime t0,
                   double glucose_equiv_carbs,//Number of carbohydrates
                   double t_assent,  //how long to get to max-stomach-absorption rate (minutes)
                   double t_dessent, //stomach-absorption rate tapper off time (minutes)
                   double v_max,     //max stomach absorption rate (grams/minute)
                   double k_gut_absoption, //gut absorption constant
                   double timeStep  //timestep for numerical caculation
                                          );


//Called by above to get rateofstomach empting into gut
double yatesGlucodeStomachToGutRate( double t, 
                   double glucose_equiv_carbs,
                   double t_assent,  //how long to get to max-stomach-absorption rate (minutes)
                   double t_dessent, //stomach-absorption rate tapper off time (minutes)
                   double v_max     //max stomach absorption rate (grams/minute)
                                   ); 

//A convience function called by yatesGlucodeStomachToGutRate
std::vector<double> 
yatesCarbsEnterAndLeaveGutRate( double t, 
                           std::vector<double> GutContents, 
                           ForcingFunction stomachToGutRate,
                           double k_gut_absoption );

  
  
#endif //KINETIC_MODEL_HH
  
