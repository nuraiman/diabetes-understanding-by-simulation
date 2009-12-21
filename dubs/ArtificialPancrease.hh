#if !defined(ARTIFICIAL_PANCREASE_HH)
#define ARTIFICIAL_PANCREASE_HH

//Started Thursday April 23 2009 -- Will Johnson
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/foreach.hpp"

//To install ROOT and QT4, just use the INSTALL_QTROOT.sh script at:
//  http://root.bnl.gov/QtRoot/How2Install4Unix.html
//  currently code is not dependant on QT, but I would like to use itin near future

const double kFailValue = -9999.9;

class TApplication;
extern TApplication *gTheApp; //I think I could get rid of this and just us gApplication

//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

typedef std::vector<double>               DVec;
typedef boost::posix_time::ptime          PosixTime;
typedef boost::posix_time::time_duration  TimeDuration;
typedef std::vector<PosixTime>            PTimeVec;
typedef std::vector<TimeDuration>         PTimeDurationVec;
typedef std::pair<PosixTime, PosixTime>   TimeRange;
typedef std::vector<TimeRange>            TimeRangeVec;

//A typedef for functions that take only one argument
typedef boost::function< double(double time) > ForcingFunction;
typedef boost::function< double(const PosixTime &time) > PTimeForcingFunction;
// typedef boost::function< double(double time, double val) > ODEDerivFunction;


//A typedef needed to interface to the Runge-Kutta integrator
typedef boost::function< DVec (double x, DVec y) > RK_DerivFuntion;
typedef boost::function< DVec (const PosixTime &time, const DVec &yi) > RK_PTimeDFunc;


const PosixTime kGenericT0( boost::gregorian::date(1982, 
                            boost::gregorian::Jul, 28), 
                            TimeDuration( 0, 0, 0, 0) );

const PosixTime kTGraphStartTime( boost::gregorian::date(2008, 
                                  boost::gregorian::Jan, 1), 
                                  TimeDuration( 0, 0, 0, 0) );

namespace DefaultInputs
{
  extern std::string ns_defaultModelFileName;
};//

//Below are variables whos values may be set via the command line - or gui panel.
//  Values shown are the default values if not specified
//  These variables will soon become a class in ProgramOptions so we can 
//  save them to disk easier when a model is saved
namespace PersonConstants
{
  extern double kPersonsWeight;          //kg, command line '--weight=78', or '-kg 78'
  extern double kBasalGlucConc;          //
};//namespace PersonConstants


namespace ModelDefaults
{
  extern double kCgmsIndivReadingUncert;
  
  extern TimeDuration kDefaultCgmsDelay; //minutes, ('--cgmsdelay=15')
  extern TimeDuration kPredictAhead;     //default how far to predict ahead of cgms
  extern TimeDuration kIntegrationDt;    //Integration tiemstep in minutes ('--dt=1.0')
  
  extern double kLastPredictionWeight;   //for calc. chi2 of model ('--last_pred_weight=0.25')
  
  extern double kTargetBG;               //target blood glucose, ('--target=100')
  extern double kBGLowSigma;             //uncert on low BG, ('--lowsigma=10')
  extern double kBGHighSigma;            //uncert on high BG, ('--lowsigma=20')
  
  //Genetic minimization paramaters
  extern int kGenPopSize;                //(--genetic_pop_size=100)
  extern int kGenConvergNsteps;          //(--genetic_conv_steps=10)
  extern int kGenNStepMutate;            //(--genetic_nstep_track=6)
  extern int kGenNStepImprove;           //(--genetic_ngen_improve=3)
  extern double kGenSigmaMult;           //(--genetic_mutate_sigma=0.5)
  extern double kGenConvergCriteria;     //(--genetic_convergence_chi2=1.0)
  //When the number of improvments within the last kGenNStepMutate generations is:
  //  a) smaller than kGenNStepImprove, then divide current sigma by kGenSigmaMult
  //  b) equal, do nothing
  //  c) larger than kGenNStepImprove, then multiple current sigma by kGenSigmaMult
  //
  //If convergence hasn't improvedmore than kGenConvergCriteria in the last
  //  kGenConvergNsteps, then consider minimization complete
};//namespace ModelDefaults


#endif //ARTIFICIAL_PANCREASE_HH 