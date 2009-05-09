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
    void serialize( Archive &ar, const unsigned int version )
    {
      unsigned int ver = version; //keep compiler from complaining
      ver = ver;
      
      ar & m_minutes;
      ar & m_value;
    }//serialize
    
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
    boost::posix_time::ptime m_t0;
    
    //The default time step, in minutes
    //  The elements in the base GraphElementSet MAY have a smaller
    //  time delta, this is just the default
    double m_dt;
    
    //only applied to drawn objects
    double m_yOffsetForDrawing;
    
    //The type of Conserntration Graph this is
    GraphType m_graphType;
    
  public:
    //Initializers
    ConsentrationGraph( const ConsentrationGraph &lhs );
    ConsentrationGraph( const std::string &savedFileName );
    ConsentrationGraph( boost::posix_time::ptime t0, double dt, GraphType graphType );
    
    const ConsentrationGraph &operator=( const ConsentrationGraph &rhs );
    
    //Internal Methods
    double getOffset( const boost::posix_time::ptime &absoluteTime ) const;  //offset from m_t0 in minutes 
    static double getDt( const boost::posix_time::ptime &t0, const boost::posix_time::ptime &t1 );  //time diff in minutes 
    boost::posix_time::ptime getAbsoluteTime( double nOffsetMinutes ) const;
    bool containsTime( boost::posix_time::ptime absoluteTime ) const;
    bool containsTime( double nMinutesOffset ) const;
     
    
    //If the base GraphElementSet doesn't have the exact time you want,
    //  value(...) will just use linear interpolation to find the value
    double value( double nOffsetminutes ) const;
    double value( boost::posix_time::ptime absTime ) const;
    double value( boost::posix_time::time_duration offset ) const;
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
    boost::posix_time::ptime getT0() const;
    boost::posix_time::ptime getEndTime() const;
    
    //To add a single point use addNewDataPoint
    //  however, use of this function is mildly unsafe if you have previously
    //  used an add(...) function
    ConstGraphIter addNewDataPoint( double offset, double value );
    ConstGraphIter addNewDataPoint( boost::posix_time::ptime time, double value );
    
    //cals above
    ConstGraphIter insert( double offsetTime, double value );
    ConstGraphIter insert( const boost::posix_time::ptime &absoluteTime, double value );
   
    double getMostCommonDt() const;
    
    
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
    ConsentrationGraph getTotal( double amountPerVol, boost::posix_time::ptime functionT0, 
                                 AbsFuncPointer ) const;
    ConsentrationGraph getTotal( double amountPerVol, boost::posix_time::ptime functionT0, 
                     AbsorbtionFunction absFunc = NumAbsorbtionFunctions) const;


    void setT0_dontChangeOffsetValues( boost::posix_time::ptime newT0 );
    
    void setYOffsetForDrawing( double yOffset );
    
    
    //Forms a TGraph and draws on current active TPad (gPad)
    TGraph* draw( std::string options = "", 
                  std::string title = "", 
                  bool pause = true,
                  int color = 0 ) const;
    TGraph *ConsentrationGraph::getTGraph() const;
    
    //Some funcitons to aid in drawing the graph
    static std::string getDate( boost::posix_time::ptime time );
    static std::string getDateForGraphTitle( boost::posix_time::ptime time );
    static std::string getTimeNoDate( boost::posix_time::ptime time );
    static std::string getTimeAndDate( boost::posix_time::ptime time );
    static boost::posix_time::ptime roundDownToNearest15Minutes( 
                                   boost::posix_time::ptime time, int slop = 15); //slop is number of minutes
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
    
    
    //Serialize function has to be in the header since it's a template
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version )
    {
      unsigned int ver = version; //keep compiler from complaining
      ver = ver;
      
      ar & m_t0;
      ar & m_dt;
      ar & m_yOffsetForDrawing;
      ar & m_graphType;
      ar & boost::serialization::base_object< std::set<GraphElement> >(*this);
      // ar & (*static_cast< std::set<GraphElement> *>(this));
    }//serialize
    
};//class ConsentrationGraph

BOOST_CLASS_VERSION(ConsentrationGraph, 0)


//We need to be able to serialize ptime, so we can save ConsentrationGraph.
namespace boost {
  namespace serialization {
    //serialize(...) has to write and read the time, so it's a bit awkward
    template<class Archive>
     void serialize(Archive & ar, boost::posix_time::ptime &time, const unsigned int version)
    {
      unsigned int vers = version; //to keep compiler from complaining
      vers = vers;
      
      std::string ts = boost::posix_time::to_simple_string(time);
      
      ar & ts;
      
      time = boost::posix_time::time_from_string(ts);
    }
  } // namespace serialization
} // namespace boost


#endif //CONSENTRATION_GRAPH_HH

