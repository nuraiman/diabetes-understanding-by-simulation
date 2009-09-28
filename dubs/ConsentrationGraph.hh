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

#include <set>
#include "boost/date_time/posix_time/posix_time.hpp"

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/version.hpp>


#include "ArtificialPancrease.hh" //contains useful typedefs and constants


//forward declaration
class TGraph;


                               
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
  NumAbsorbtionFunctions,
};//enum AbsorbtionFunction


//  I want to look at other spline smoothings like
//http://www.physics.utah.edu/~detar/phys6720/handouts/cubic_spline/cubic_spline/node2.html
                         
enum SmoothingAlgo
{
  FourierSmoothing,      //Strict low-pass filter
  ButterworthSmoothing,  //order 4 low-pass Butterworth filter
  BSplineSmoothing,      //B-Splines, okay smoothing of data (not above link type)
                         //  but excelentfor taking derivative
  NoSmoothing,
};//enum SmoothingAlgo


class GraphElement
{
  public:
  
    double m_minutes;
    double m_value;
    
    GraphElement();
    GraphElement( double nMinutes, double value );
    
    //Serialize function has to be in the header since it's a template
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version );
    
    //Sort based off of 'm_minutes' element
    bool operator<( const GraphElement &lhs ) const;
};//class GraphElement

BOOST_CLASS_VERSION(GraphElement, 0)


typedef  double Filter_Coef[6][10]; //for the butterworth filter
typedef  double Memory_Coef[3][10]; //for the butterworth filter

typedef std::set<GraphElement>          GraphElementSet;
typedef GraphElementSet::const_iterator ConstGraphIter; //set iterators const always

typedef double(*AbsFuncPointer)( double, double );

//In retrospec, I should have just made ConsentrationGraph to 
// be based off of a map<ptime, double> object

//set<> is an stl sorted storage array 
class ConsentrationGraph : public std::set<GraphElement>
{
  private:
    //The time GraphElement.m_minutes is relative to
    PosixTime m_t0;
    
    //The default time step, in minutes
    //  The elements in the base GraphElementSet MAY have a smaller
    //  time delta, this is just the default
    double m_dt;
    
    //only applied to drawn objects, or model predictions not based at 0
    double m_yOffsetForDrawing;
    
    //The type of Conserntration Graph this is
    GraphType m_graphType;
    
  public:
    //Initializers
    ConsentrationGraph( const ConsentrationGraph &lhs );
    ConsentrationGraph( const std::string &savedFileName );
    ConsentrationGraph( PosixTime t0, double dt, GraphType graphType );
    
  private:
  //insert any funciton that uses a double to represent time here
  
  public:
    
    
    const ConsentrationGraph &operator=( const ConsentrationGraph &rhs );
    
    //Internal Methods
    double getOffset( const PosixTime &absoluteTime ) const;  //offset from m_t0 in minutes 
    static double getDt( const PosixTime &t0, const PosixTime &t1 );  //time diff in minutes 
    PosixTime getAbsoluteTime( double nOffsetMinutes ) const;
    bool containsTime( PosixTime absoluteTime ) const; //caution doesn't work like you think
    bool containsTime( double nMinutesOffset ) const;  //  returns true if the base set<> 
                                                       //  contains the exact time
    
    //If the base GraphElementSet doesn't have the exact time you want,
    //  value(...) will just use linear interpolation to find the value
    double value( double nOffsetminutes ) const;
    double value( PosixTime absTime ) const;
    double value( TimeDuration offset ) const;
    double valueUsingOffset( double nOffsetminutes ) const;  //just a synonym for value(double)
    
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
    unsigned int add( double amountPerVol, boost::posix_time::ptime beginTime, AbsFuncPointer absFunc );
    unsigned int add( double amountPerVol, double beginTimeOffset, 
                      AbsFuncPointer );
    
    //For use with a predifined dunction
    //  If NumAbsortionFunctions specified, defaults 
    //  to NovologAbsorbtion, or MediumCarbAbsorbtion,
    unsigned int add( double amountPerVol, boost::posix_time::ptime beginTime, 
                      AbsorbtionFunction absFunc = NumAbsorbtionFunctions);
    unsigned int add( double amountPerVol, double beginTimeOffset, 
                      AbsorbtionFunction absFunc = NumAbsorbtionFunctions);
    
    //get rid of the zero points that don't add anything
    unsigned int removeNonInfoAddingPoints();
    
    double getDt() const;
    GraphType getGraphType() const;
    std::string getGraphTypeStr() const;
    PosixTime getT0() const;
    PosixTime getStartTime() const;
    PosixTime getEndTime() const;
    
    
    //below removes data before t_start and after t_end
    void trim( const PosixTime &t_start = kGenericT0, 
               const PosixTime &t_end = kGenericT0 );
    
    //To add a single point use addNewDataPoint
    //  however, use of this function is mildly unsafe if you have previously
    //  used an add(...) function
    ConstGraphIter addNewDataPoint( double offset, double value );
    ConstGraphIter addNewDataPoint( PosixTime time, double value );
    
    //cals above
    ConstGraphIter insert( double offsetTime, double value );
    ConstGraphIter insert( const PosixTime &absoluteTime, double value );
    
    unsigned int addNewDataPoints( const ConsentrationGraph &newDataPoints );
    
    ConstGraphIter lower_bound( const PosixTime &time ) const;
    ConstGraphIter upper_bound( const PosixTime &time ) const;
    
    double getMostCommonDt() const;
    TimeDuration getMostCommonPosixDt() const;
    
    //You probably should not call the folowing 2 functions, 
    //  instead call 'getSmoothedGraph(...)'
    //For fastFourierSmoothing, lambda_min is wavelength of noise removed
    //  e.g. bumps/noise lasting less than 0.5*lambda_min are smoothed out
    //  tight now I do the bone head thing of just removing the frequencies
    //  above threshold, I think I should use a convoluton???
    void fastFourierSmooth( double lambda_min = 30.0, double time_window = -1 );
    
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
    
    //Forms a TGraph and draws on current active TPad (gPad)
    //  if pause is true, then gPad will be deleted upon File->exit
    TGraph* draw( std::string options = "", 
                  std::string title = "", 
                  bool pause = true,
                  int color = 0 ) const;
    //The x-axis will be number of minutes since 'kTGraphStartTime'
    TGraph *getTGraph( PosixTime t_start = kGenericT0,
                                           PosixTime t_end = kGenericT0  ) const;
    
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
    
    
    //Will leave serialization funtion in header
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version );
    
};//class ConsentrationGraph

BOOST_CLASS_VERSION(ConsentrationGraph, 0)


#endif //CONSENTRATION_GRAPH_HH

