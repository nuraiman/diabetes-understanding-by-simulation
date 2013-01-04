#if !defined(ARTIFICIAL_PANCREASE_HH)
#define ARTIFICIAL_PANCREASE_HH

#include "DubsConfig.hh"

//Started Thursday April 23 2009 -- Will Johnson
#include <string>
#include <vector>
#include <exception>

#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


const double kFailValue = -9999.9;

typedef std::vector<double>               DVec;
typedef boost::posix_time::ptime          PosixTime;
typedef boost::posix_time::time_duration  TimeDuration;
typedef std::vector<PosixTime>            PTimeVec;
typedef std::vector<TimeDuration>         PTimeDurationVec;
typedef boost::posix_time::time_period    TimeRange;
//typedef std::pair<PosixTime, PosixTime>   TimeRange;
typedef std::vector<TimeRange>            TimeRangeVec;

//A typedef for functions that take only one argument
typedef boost::function< double(double time) > ForcingFunction;
typedef boost::function< double(const PosixTime &time) > PTimeForcingFunction;
// typedef boost::function< double(double time, double val) > ODEDerivFunction;


//A typedef needed to interface to the Runge-Kutta integrator
typedef boost::function< DVec (double x, DVec y) > RK_DerivFuntion;
typedef boost::function< DVec (const PosixTime &time, const DVec &yi) > RK_PTimeDFunc;

inline bool lessThan( const boost::posix_time::time_period &l,
                     const boost::posix_time::time_period &r )
                     { return l.begin() < r.begin(); }


const PosixTime kGenericT0( boost::gregorian::date(1982,
                            boost::gregorian::Jul, 28),
                            TimeDuration( 0, 0, 0, 0) );
const TimeRange kInvalidTimeRange( kGenericT0, TimeDuration( 0, 0, 0, -1) );
#define EmptyTimeRangeVec TimeRangeVec(0,TimeRange(kGenericT0,kGenericT0))

const PosixTime kTGraphStartTime( boost::gregorian::date(2008,
                                  boost::gregorian::Jan, 1),
                                  TimeDuration( 0, 0, 0, 0) );

class CouldntOpenException: public std::exception
{
public:
  CouldntOpenException(){}
  virtual const char* what() const throw()
  { return "Couldn't open file"; }
};//class CouldntOpenException

//In order to determine at runtime where in the source file an error comes from,
//  we need the below three macros
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef SRC_LOCATION
#undef SRC_LOCATION
#endif
#define SRC_LOCATION ((boost::format("File %s: Function '%s': Line %s") \
                       % __FILE__  % BOOST_CURRENT_FUNCTION  % TOSTRING(__LINE__)).str())

//The below YEAR MONTH DAY macros are taken from
//http://bytes.com/topic/c/answers/215378-convert-__date__-unsigned-int
//  and I believe to be public domain code
#define YEAR ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 \
         + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))
#define MONTH (__DATE__ [2] == 'n' ? 0 \
         : __DATE__ [2] == 'b' ? 1 \
         : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3) \
         : __DATE__ [2] == 'y' ? 4 \
         : __DATE__ [2] == 'n' ? 5 \
         : __DATE__ [2] == 'l' ? 6 \
         : __DATE__ [2] == 'g' ? 7 \
         : __DATE__ [2] == 'p' ? 8 \
         : __DATE__ [2] == 't' ? 9 \
         : __DATE__ [2] == 'v' ? 10 : 11)
#define DAY ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 \
        + (__DATE__ [5] - '0'))

//The below is helpful to keep track of what day a particular executable
//  was compiled on
#ifdef COMPILE_DATE
#undef COMPILE_DATE
#endif
#define COMPILE_DATE ((boost::format("%d%02d%02d") % (YEAR) % ((MONTH) + 1) % (DAY)).str())

#endif //ARTIFICIAL_PANCREASE_HH
