#if !defined(ARTIFICIAL_PANCREASE_HH)
#define ARTIFICIAL_PANCREASE_HH

//Started Thursday April 23 2009 -- Will Johnson
#include "boost/function.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"


const double kFailValue = -9999.9;


//A typedef for functions that take only one argument
typedef boost::function< double(double time) > ForcingFunction;

// typedef boost::function< double(double time, double val) > ODEDerivFunction;


//A typedef needed to interface to the Runge-Kutta integrator
typedef boost::function< std::vector<double> (double x, std::vector<double> y) > RK_DerivFuntion;


const boost::posix_time::ptime kGenericT0( boost::gregorian::date(1982, 
                                             boost::gregorian::Jul, 28), 
                                 boost::posix_time::time_duration( 0, 0, 0, 0));


namespace PersonConstants
{
  const double kPersonsWeight = 78.0; //kg
};//namespace PersonConstants

#endif //ARTIFICIAL_PANCREASE_HH 
