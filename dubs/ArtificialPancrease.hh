#if !defined(ARTIFICIAL_PANCREASE_HH)
#define ARTIFICIAL_PANCREASE_HH

//Started Thursday April 23 2009 -- Will Johnson
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"



const double kFailValue = -9999.9;

typedef std::vector<boost::posix_time::ptime> PTimeVec;
typedef std::pair<boost::posix_time::ptime, boost::posix_time::ptime> TimeRange;
typedef std::vector<TimeRange> TimeRangeVec;

//A typedef for functions that take only one argument
typedef boost::function< double(double time) > ForcingFunction;
typedef boost::function< double(const boost::posix_time::ptime &time) > PTimeForcingFunction;
// typedef boost::function< double(double time, double val) > ODEDerivFunction;


//A typedef needed to interface to the Runge-Kutta integrator
typedef boost::function< std::vector<double> (double x, std::vector<double> y) > RK_DerivFuntion;
typedef boost::function< std::vector<double> (const boost::posix_time::ptime &time, const std::vector<double> &yi) > RK_PTimeDFunc;


const boost::posix_time::ptime kGenericT0( boost::gregorian::date(1982, 
                                             boost::gregorian::Jul, 28), 
                                 boost::posix_time::time_duration( 0, 0, 0, 0));


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
