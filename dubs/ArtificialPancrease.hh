#if !defined(ARTIFICIAL_PANCREASE_HH)
#define ARTIFICIAL_PANCREASE_HH

//Started Thursday April 23 2009 -- Will Johnson
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"



const double kFailValue = -9999.9;

typedef std::vector<double>               DVec;
typedef boost::posix_time::ptime          PosixTime;
typedef boost::posix_time::time_duration  TimeDuration;
typedef std::vector<PosixTime>            PTimeVec;
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
                            TimeDuration( 0, 0, 0, 0));


namespace PersonConstants
{
  extern double kPersonsWeight; //kg, command line '--weight=78', or '-kg 78'
  extern double kBasalGlucConc; //
};//namespace PersonConstants


namespace ModelDefaults
{
  extern double kDefaultCgmsDelay; //minutes, command line '--cgmsdelay=15'
  extern double kCgmsIndivReadingUncert;
};//namespace ModelDefaults


#endif //ARTIFICIAL_PANCREASE_HH 
