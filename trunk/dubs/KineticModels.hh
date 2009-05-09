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



/*-------------> Now for models of how a human responds to inputs <-----------*/

//This is my modification oto the Non-Linear Type I Diabetes Bergman Model
//I use variable naming convention thats in the literature

//  G is blood glucose consentration, relative to basal (mg/dL)
//    G_basal is steady state blood glucose consentration, ~100 (mg/dL)
//  I is blood-plasma insulin consentration above basal (mU/L)
//    in this model it is a forcing function (pre-determined)
//    I_basal is steady state insulin consentration (mU/L)
//  X is Effective insulin  ( 1/min multiplied by constant 1*min*mg/dL )
//  D is rate of glucose change in blood-stream (mg/dL/min)
//    due to either carb-intake, or things like excersize, a forcing function.
//  G_parameter[0] multiplies G; g_parameter[1] unused//multiplies I_basal
//  X_parameter[0] multiplies X, X_parameter[1] multiplies I;  others unused now

double dG_dT( double time, 
              double X,    //This is 
              double G, 
              double G_basal, 
              double I_basal, 
              std::vector<double> G_parameter, //currently chould be of size 2
              ForcingFunction D  //this is a function you supply, created via:
                                 //ConsentrationGraph foodEaten(...)
                                  // ForcingFunction D = boost::bind( &ConsentrationGraph::value, foodEaten, _1 )
            );


double dX_dT( double time, 
              double X,
              double G,
              std::vector<double> X_parameters, //currently chould be of size 2
              ForcingFunction I   //Similar to D, but for insulin
            );


//Now we need a wrapper function for 'dG_dT' and 'dX_dT' for the 
//  RungeKutta integration.  We actually need a function with a signature
//  vector<double> f(double, vector<double>), but we can use boost::bind 
//  to achieve this for the below
std::vector<double> dGdT_and_dXdT( double time, std::vector<double> G_and_X, 
                                   double G_basal, double I_basal, 
                                   std::vector<double> G_parameters,  //should be size==1
                                   std::vector<double> X_parameters,  //should be size==2
                                   ForcingFunction D,
                                   ForcingFunction I
                                 );
  
  
#endif //KINETIC_MODEL_HH
  
