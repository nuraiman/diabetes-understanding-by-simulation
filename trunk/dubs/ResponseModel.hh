#if !defined(RESPONSE_MODEL_HH)
#define RESPONSE_MODEL_HH

#include <vector>
#include <utility> //for pair<,>
#include "boost/function.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/serialization/set.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/string.hpp>


#include "KineticModels.hh"
#include "ConsentrationGraph.hh"
#include "ArtificialPancrease.hh" //contains useful typedefs and constants


#include "Minuit2/FCNBase.h"
#include "TMVA/IFitterTarget.h"


//Eventually, when I add more models, I'll set up a inheritance hiegharchy
//  to improve organization

/*******************************************************************************
* To Do
*  Move all logic for finding start/stop times for predictions into seperate function
*  Generally clean up the  integrator functions
*  Convert to using a matrix for errors, instead/in-addtion-to vectors
*  add option to options_namespace for {default m_dt, 
*  work on the the chi2 def by throwing random numbers and looking at dist.
*  make the model more more sophisticated
*  add ability to find amount of insulin to take for a meal, maybe this should
*     be a function outside of this class
*  add ability to guess what went wrong in the previous 3 hours
*  add auto correlation function to find cgms delay
*  add function to find all possible time-ranges to make predictions in
*  add a debug setting that can be turned on/off/different-levels
*******************************************************************************/

class NLSimple;

typedef std::pair<PosixTime,double> FoodConsumption;
typedef std::pair<PosixTime,double> InsulinInjection;


//lets find the amount of insulin to be taken between startTime 
//  and startTime & endTime such that when X->2% of it's max, 
//  B.G. will be at basal
// InsulinInjection findInsulinCorrection( const NLSimple &model, 
                                        // const PosixTime &startTime,
                                        // const PosixTime &endTime);


//My non-linear simple model, it is a inspired by the Bergman model
class NLSimple
{
  public:
  
    //Parameters used to compute the diferential of Blood Glucose
    enum NLSimplePars
    {
      // dGdT = -BGMultiplier*G - X(G + G_basal) + CarbAbsorbMultiplier*CarbAbsorbRate
      BGMultiplier = 0,   
      CarbAbsorbMultiplier,
      
      // dXT = -XMultiplier*X + PlasmaInsulinMultiplier*I
      XMultiplier,
      PlasmaInsulinMultiplier,
      
      //
      NumNLSimplePars
    };//enum NLSimplePars
    
  
    //begin variable that matter to *this
    std::string m_description;                  //Useful for later checking
    
    TimeDuration m_cgmsDelay;                     //initially set to 15 minutes
    double m_basalInsulinConc;                    //units per hour
    double m_basalGlucoseConcentration;

    PosixTime      m_t0;
    TimeDuration   m_dt;
    TimeDuration   m_predictAhead; //how far predictions should be ahead of cgms
                                   //if set to less than 0, then optimization
                                   //routines use absolute prediction, not cgms
                                   //based prediction
    
    double              m_effectiveDof; //So Minuit2 can properly interpret errors
    std::vector<double> m_paramaters;           //size == NumNLSimplePars
    std::vector<double> m_paramaterErrorPlus;
    std::vector<double> m_paramaterErrorMinus;
    
    //If you add CGMS data from Isig, then below converts from CGMS reading
    //  to a corrected reading
    double m_currCgmsCorrFactor[2];
    
    ConsentrationGraph m_cgmsData;
    ConsentrationGraph m_freePlasmaInsulin;
    ConsentrationGraph m_glucoseAbsorbtionRate;
    
    ConsentrationGraph m_predictedInsulinX;
    ConsentrationGraph m_predictedBloodGlucose;
    
    
    PTimeVec m_startSteadyStateTimes;
    
    
    NLSimple( std::string fileName );
    NLSimple( const std::string &description, 
              double basalUnitsPerKiloPerhour,
              double basalGlucoseConcen = PersonConstants::kBasalGlucConc, 
              PosixTime t0 = kGenericT0 );
    const NLSimple &operator=( const NLSimple &rhs );
    
    ~NLSimple() {};
    
    double getOffset( const boost::posix_time::ptime &absoluteTime ) const;
    PosixTime getAbsoluteTime( double nOffsetMinutes ) const;
    

    
    //Functions for integrating the kinetic equations
    double dGdT( const PosixTime &time, double G, double X ) const;
    double dXdT( const PosixTime &time, double G, double X ) const;
    double dXdT_usingCgmsData( const PosixTime &time, double X ) const;
    
    std::vector<double> dGdT_and_dXdT( const PosixTime &time, 
                                       const std::vector<double> &G_and_X ) const;
    RK_PTimeDFunc getRKDerivFunc() const;
    
    static double getBasalInsulinConcentration( double unitesPerKiloPerhour );
    void addCgmsData( const ConsentrationGraph &newData, 
                        bool findNewSteadyState = false );
    void addCgmsData( PosixTime, double value );
    void addCgmsDataFromIsig( const ConsentrationGraph &isigData,
                              const ConsentrationGraph &calibrationData,
                              bool findNewSteadyState = false );
    void addBolusData( const ConsentrationGraph &newData, 
                       bool finNewSteadyStates = false );
    
    //uses default absorption rates
    void addConsumedGlucose( PosixTime time, double amount ); 
    
    //if you pass in carbs consumed, just uses default absorption rate
    //  passing in glucose absorption rate is preffered method
    void addGlucoseAbsorption( const ConsentrationGraph &newData ); 
    
    void resetPredictions();
    void setModelParameters( const std::vector<double> &newPar );
    void setModelParameterErrors( std::vector<double> &newParErrorLow, 
                                  std::vector<double> &newParErrorHigh );
    
    PosixTime findSteadyStateStartTime( PosixTime t_start, PosixTime t_end );
    void findSteadyStateBeginings( double nHoursNoInsulinForFirstSteadyState = 3.0 );
    
    
    ConsentrationGraph glucPredUsingCgms( int nMinutesPredict = -1,  //nMinutes ahead of cgms
                                                                     //if <=0, uses m_predictAhead
                                          PosixTime t_start = kGenericT0,
                                          PosixTime t_end   = kGenericT0 );
    
    //returns true  if all information is updated to time
    //  Only modifes cgmsData, X, and predictedGlucose graphs
    //  X and predictedGlucose will end at cgmsEndTime - m_cgmsDelay
    bool removeInfoAfter( const PosixTime &cgmsEndTime );
    
    void updateXUsingCgmsInfo( bool recomputeAll = true );
    
    double performModelGlucosePrediction( PosixTime t_start = kGenericT0,
                                          PosixTime t_end = kGenericT0,
                                          double bloodGlucose_initial = kFailValue,
                                          double bloodX_initial = kFailValue );
    
    double getModelChi2( double fracDerivChi2 = 0.0,
                         PosixTime t_start = kGenericT0,
                         PosixTime t_end = kGenericT0 );
    
    double getChi2ComparedToCgmsData( ConsentrationGraph &inputData,
                                      double fracDerivChi2 = 0.0,
                                      PosixTime t_start = kGenericT0,
                                      PosixTime t_end = kGenericT0 );
    
    //Below gives chi^2 based only on height differences of graphs
    double getBgValueChi2( const ConsentrationGraph &modelData,
                           const ConsentrationGraph &cgmsData,
                           PosixTime t_start, PosixTime t_end ) const;
    
    //Below gives chi^2 based on the differences in derivitaves of graphs
    double getDerivativeChi2( const ConsentrationGraph &modelDerivData,
                              const ConsentrationGraph &cgmsDerivData,
                              PosixTime t_start, PosixTime t_end ) const;
    //want to add a variable binning chi2
    
    double getFitDof() const;
    void setFitDof( double dof );
    
    
    //For model fiiting, specifying nMinutesPredict<=0.0 means don't use cgms 
    //  data tomake predictions
    double geneticallyOptimizeModel( double fracDerivChi2,
                                     TimeRangeVec timeRanges = TimeRangeVec(0) );
    
    double fitModelToDataViaMinuit2( double fracDerivChi2,
                                     TimeRangeVec timeRanges = TimeRangeVec(0) );
    
    DVec chi2DofStudy( double fracDerivChi2,
                             TimeRangeVec timeRanges = TimeRangeVec(0) ) const;
    
    void draw( bool pause = true, PosixTime t_start = kGenericT0,
               PosixTime t_end = kGenericT0 );
    
    bool saveToFile( std::string filename );
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version );
};//class NLSimple

BOOST_CLASS_VERSION(NLSimple, 0)


//An interace for NLSimple to Minuit2 and the Genetic Optimizer
class ModelTestFCN : public ROOT::Minuit2::FCNBase,  public TMVA::IFitterTarget
{
  public:
    //function forMinuit2
    virtual double operator()(const std::vector<double>& x) const;
    virtual double Up() const;
    virtual void SetErrorDef(double dof);
    
    //Function for TMVA fitters
    virtual Double_t EstimatorFunction( std::vector<Double_t>& parameters );
    
    ModelTestFCN( NLSimple *modelPtr, 
                  double fracDerivChi2,
                  std::vector<TimeRange> timeRanges );
  
  private:
    NLSimple *m_modelPtr;
    double    m_fracDerivChi2;
    
    std::vector<TimeRange> m_timeRanges;
    PosixTime m_tStart;
    PosixTime m_tEnd;
};//class ModelTestFCN

#endif //RESPONSE_MODEL_HH
