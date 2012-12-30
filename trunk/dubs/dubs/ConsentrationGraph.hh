#if !defined(CONSENTRATION_GRAPH_HH)
#define CONSENTRATION_GRAPH_HH

////////////////////////////////////////////////////////////////////////////////
//Started April 23 2009, -- Will Johnson                                      //
//  ConsentrationGraph is just a discrete-time-step function for keeping      //
//  track of blood-glucose consentrations, blood insulin consentrations, or   //
//  the absorbtion rate of glucose into blood stream.                         //
//  May also be used for other quantities, such as dg/dt, etc.                //
//  Storage format may be upgraded to a continues function in the future      //
////////////////////////////////////////////////////////////////////////////////

#include "DubsConfig.hh"

#include <set>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

// include headers that implement a archive in simple text format
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include "ArtificialPancrease.hh" //contains useful typedefs and constants


//forward declaration
#if(USE_CERNS_ROOT)
class TGraph;
#endif

//a struct so that we can sort by index
template<class T> struct index_compare_descend
{
  index_compare_descend(const T arr) : arr(arr) {} //pass the actual values you want sorted into here
  bool operator()(const size_t a, const size_t b) const
  {
    return arr[a] > arr[b];
  }
  const T arr;
};//struct index_compare


//a struct so that we can sort by index
template<class T> struct index_compare_assend
{
  index_compare_assend(const T arr) : arr(arr) {} //pass the actual values you want sorted into here
  bool operator()(const size_t a, const size_t b) const
  {
    return arr[a] < arr[b];
  }
  const T arr;
};//struct index_compare


//Some defines
enum GraphType
{
  InsulinGraph,  //free plasma insulin consentration
  BolusGraph,    //When bolus were given, a discreet graph
  GlucoseConsentrationGraph,
  GlucoseAbsorbtionRateGraph,
  GlucoseConsumptionGraph,
  BloodGlucoseConcenDeriv,  //the derivative of blood-glucose concentration
  CustomEvent,
  AlarmGraph,
  NumGraphType
};//enum GraphType


//Predefined absorbtion functions
//  These are mostly for debugging purposes while building the framework,
//  I hope to replace these with more flexible functionoids later
enum AbsorbtionFunction
{
  NovologAbsorbtion,
  FastCarbAbsorbtionRate,
  MediumCarbAbsorbtionRate,
  SlowCarbAbsorbtionRate,
  NumAbsorbtionFunctions
};//enum AbsorbtionFunction


//  I want to look at other spline smoothings like
//http://www.physics.utah.edu/~detar/phys6720/handouts/cubic_spline/cubic_spline/node2.html

enum SmoothingAlgo
{
  FourierSmoothing,      //Strict low-pass filter
  ButterworthSmoothing,  //order 4 low-pass Butterworth filter
  BSplineSmoothing,      //B-Splines, okay smoothing of data (not above link type)
                         //  but excelentfor taking derivative
  NoSmoothing
};//enum SmoothingAlgo


class GraphElement
{
  public:

    PosixTime m_time;
    double m_value;

    GraphElement();
    GraphElement( const PosixTime &time, double value );

    //Serialize function has to be in the header since it's a template
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version );

    //Sort based off of 'm_minutes' element ONLY!
    bool operator<( const GraphElement &lhs ) const;
//    bool operator>( const GraphElement &lhs ) const;
//    bool operator==( const GraphElement &lhs ) const;
};//class GraphElement

BOOST_CLASS_VERSION(GraphElement, 0)


TimeDuration toTimeDuration( double nMinutes );
double toNMinutes( const TimeDuration &timeDuration );


typedef  double Filter_Coef[6][10]; //for the butterworth filter
typedef  double Memory_Coef[3][10]; //for the butterworth filter

typedef std::vector<GraphElement>      GraphElementSet;
typedef GraphElementSet::iterator GraphIter; //
typedef GraphElementSet::const_iterator ConstGraphIter; //set iterators const always

typedef double(*AbsFuncPointer)( double, double );


void ff_xform_r2c( const std::vector<double> &input,
                   std::vector<double> &xformed_real,
                   std::vector<double> &xformed_imag );
void ff_xform_c2r( const std::vector<double> &in_real,
                   const std::vector<double> &in_img,
                   std::vector<double> &output );

//In retrospec, I should have just made ConsentrationGraph to
// be based off of a map<ptime, double> object

//set<> is an stl sorted storage array
class ConsentrationGraph : public GraphElementSet
{
  private:
    //The time GraphElement.m_minutes is relative to
    PosixTime m_t0;

    //The default time step, in minutes
    //  The elements in the base GraphElementSet MAY have a smaller
    //  time delta, this is just the default
    TimeDuration m_dt;

    //only applied to drawn objects, or model predictions not based at 0
    double m_yOffsetForDrawing;

    //The type of Conserntration Graph this is
    GraphType m_graphType;

  public:
    //Initializers
    ConsentrationGraph( const ConsentrationGraph &lhs );
    ConsentrationGraph( const std::string &savedFileName );
    ConsentrationGraph( PosixTime t0, double dt, GraphType graphType );
    ConsentrationGraph( PosixTime t0, TimeDuration dt, GraphType graphType );

  private:
  //insert any funciton that uses a double to represent time here

  public:


    const ConsentrationGraph &operator=( const ConsentrationGraph &rhs );

    //Internal Methods
    static double getDt( const PosixTime &t0, const PosixTime &t1 );  //time diff in minutes
    bool containsTime( PosixTime absoluteTime ) const; //caution doesn't work like you think

    //If the base GraphElementSet doesn't have the exact time you want,
    //  value(...) will just use linear interpolation to find the value
    double value( const PosixTime &time ) const;
    double valueUsingOffset( double nMinutesAfterTo ) const; //purely for compatability of some older code

    //To obtain the derivative or smoothed version of this graph
    //  note that these two functions are quite computationally inefficient
    //  (the copy this entire graph in memory a number of avoidable times)
    ConsentrationGraph getSmoothedGraph( double wavelength,
                                         SmoothingAlgo smoothType) const;
    ConsentrationGraph getDerivativeGraph( double wavelength,
                                           SmoothingAlgo smoothType ) const;


    //Add functions returns number of GraphElents added to GraphElementSet
    //  calls using posix_time::ptime, must reference a time after m_t0
    //For use with user defined function that gives blood consentration
    unsigned int add( double amountPerVol, const PosixTime &beginTime, AbsFuncPointer absFunc );

    //For use with a predifined dunction
    //  If NumAbsortionFunctions specified, defaults
    //  to NovologAbsorbtion, or MediumCarbAbsorbtion,
    unsigned int add( double amountPerVol, const PosixTime &beginTime,
                      AbsorbtionFunction absFunc = NumAbsorbtionFunctions);


    //get rid of the zero points that don't add anything
    unsigned int removeNonInfoAddingPoints();

    TimeDuration getDt() const;
    GraphType getGraphType() const;
    std::string getGraphTypeStr() const;
    PosixTime getT0() const;
    PosixTime getStartTime() const;
    PosixTime getEndTime() const;
    ConstGraphIter getLastElement() const;

    //below removes data before t_start and after t_end
    void trim( const PosixTime &t_start = kGenericT0,
               const PosixTime &t_end = kGenericT0,
               bool interpTrimmed = true );

    //To add a single point use addNewDataPoint
    //  however, use of this function is mildly unsafe if you have previously
    //  used an add(...) function
    GraphIter addNewDataPoint( const PosixTime &time, double value );
    //just calls addNewDataPoint
    GraphIter insert( const PosixTime &absoluteTime, double value );
    GraphIter insert( const GraphElement &element );


    unsigned int addNewDataPoints( const ConsentrationGraph &newDataPoints );


    GraphIter lower_bound( const PosixTime &time );
    GraphIter upper_bound( const PosixTime &time );
    ConstGraphIter lower_bound( const PosixTime &time ) const;
    ConstGraphIter upper_bound( const PosixTime &time ) const;

    GraphIter find( const PosixTime &time );
    ConstGraphIter find( const PosixTime &time ) const;
//    GraphIter find( const GraphElement &element );
//    ConstGraphIter find( const GraphElement &element ) const;

    TimeDuration getMostCommonDt() const;

    bool hasValueNear( const double &value, const double &epsilon = 0.000001 ) const;

    //You probably should not call the folowing 2 functions,
    //  instead call 'getSmoothedGraph(...)'
    //For fastFourierSmoothing, lambda_min is wavelength of noise removed
    //  e.g. bumps/noise lasting less than 0.5*lambda_min are smoothed out
    //  tight now I do the bone head thing of just removing the frequencies
    //  above threshold, I think I should use a convoluton???
    //Note: lambda_min is in minutes
    //If doMinCoeffInstead==true then frequencies with coefs below lambda_min
    //  (which must be between zero and one for this case) are removed
    //  eg. if lambda_min==0.1, then the smallest 90% of coeficients will be
    //  zeroed out
    //TODO: Make sure lambda_min isnt off by a factor of 2
    void fastFourierSmooth( double lambda_min, bool doMinCoeffInstead = false );

    //filterOrder gives order of the Butterworth filter (6 db/Octive, per order)
    //  max order 4 is supported
    void butterWorthFilter( double wavelength = 30, int filterOrder = 4 );

    double bSplineSmoothOrDeriv(  bool takeDerivative,  //true turns graph onto it's
                                                        //own derivative
                                                        //False just smoothes graph
                                  double knotDist = 30,//approx distance between knots
                                  int splineOrder = 6 ); //4 is cubic spline
    //below turns current graph into it's derivative,
    //  using finite numerical differentiation
    void differntiate( int nPoint = 3 );  //valid nPoints are 3,5,

    ConsentrationGraph getTotal( const ConsentrationGraph &rhs,
                                 const double weight = 1.0 ) const;
    ConsentrationGraph getTotal( double amountPerVol, PosixTime functionT0,
                                 AbsFuncPointer ) const;
    ConsentrationGraph getTotal( double amountPerVol, PosixTime functionT0,
                     AbsorbtionFunction absFunc = NumAbsorbtionFunctions) const;


    void setT0_dontChangeOffsetValues( PosixTime newT0 );

    void setYOffset( double yOffset );
    double getYOffset() const;

#if(USE_CERNS_ROOT)
    //Forms a TGraph and draws on current active TPad (gPad)
    //  if pause is true, then gPad will be deleted upon File->exit
    TGraph* draw( std::string options = "",
                  std::string title = "",
                  bool pause = true,
                  int color = 0 ) const;
    //The x-axis will be number of minutes since 'kTGraphStartTime'
    TGraph *getTGraph( PosixTime t_start = kGenericT0,
                                           PosixTime t_end = kGenericT0  ) const;
#endif  //#if(USE_CERNS_ROOT)
    void guiDisplay( bool pause = false );

    //Some funcitons to aid in drawing the graph
    static std::string getDate( PosixTime time );
    static std::string getDateForGraphTitle( PosixTime time );
    static std::string getTimeNoDate( PosixTime time );
    static std::string getTimeAndDate( PosixTime time );
    static PosixTime roundDownToNearest15Minutes( PosixTime time, int slop = 15); //slop is number of minutes
                                                                              // of rounding allowed


    //The static pre-defined absorbtion functions
    //To retrieve a pointer to one of the below
    AbsFuncPointer getFunctionPointer( AbsorbtionFunction absFunc ) const;

    bool saveToFile( std::string filename );
    static ConsentrationGraph loadFromFile( std::string filename );

    //next three functions implement the Butterworth filtering, adapted it from
    //  code found online (originally from Fortran), see cc file for reference
    static void butterworthInit( double Xdc, Filter_Coef C,
                                 int NSections, Memory_Coef D);
    static void calcButterworthCoef( double cuttOffFrequ, double samplingTime,
                                     int filterOrder, Filter_Coef C,
                                     int *NSections, double *groupDelay );
    static void butterworthFilter( double *Xs,double Xd, int NSections,
                                              Filter_Coef C, Memory_Coef D );
    static std::vector<double> getSavitzyGolayCoeffs(
                                         int nl, //number of coef. to left of current point
                                         int nr, //number of coef. to right of current point
                                         int m,  //order of smoothing polynomial
                                                 //  (also highest conserved moment)
                                         int ld //has to do with what derivative you want
                                                    );
    ConsentrationGraph
    getSavitzyGolaySmoothedGraph( int num_left, int num_right, int order );



    //Will leave serialization funtion in header
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version );

};//class ConsentrationGraph

BOOST_CLASS_VERSION(ConsentrationGraph, 0)


template<class Archive>
void GraphElement::serialize( Archive &ar, const unsigned int /*version*/ )
{
  ar & m_time & m_value;
}//serialize

template<class Archive>
void ConsentrationGraph::serialize( Archive &ar, const unsigned int /*version*/ )
{
  ar & m_t0;
  ar & m_dt;
  ar & m_yOffsetForDrawing;
  ar & m_graphType;
  ar & boost::serialization::base_object< GraphElementSet >(*this);
}//serialize



#endif //CONSENTRATION_GRAPH_HH

