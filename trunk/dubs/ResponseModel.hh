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

#include "NLSimpleGui.hh"

//Eventually, when I add more models, I'll set up a inheritance hiegharchy
//  to improve organization

/*******************************************************************************
* To Do
*  Move all logic for finding start/stop times for predictions into seperate function
*  Convert to using a matrix for errors, instead/in-addtion-to vectors
*  add option to options_namespace for {default m_dt, 
*  work on the the chi2 def by throwing random numbers and looking at dist.
*  make the model more more sophisticated
*    maybe a ANN like thing that depends on history
*  add ability to find amount of insulin to take for a meal, maybe this should
*     be a function outside of this class
*  add ability to guess what went wrong in the previous 3 hours
*  add auto correlation function to find cgms delay
*  add function to find all possible time-ranges to make predictions in
*  add a debug setting that can be turned on/off/different-levels
*  add excersize settings
*    --probably just a Multiple to normaly used paramaters
*  add a generic event (e.g. waking up) response -- and ability to fit to response
*  add some-sort of time varieing paramaters into the model
*
*  Make NLSimple be a 2-mode class
*    --Mode 1 is research
*    --Mode 2 is predict
*      --In this mode Inject and Consume functions will update predictions
*      --also update cgms readings will also update predictions
*        --so will need a automatic function to remove ending of X and pred
*    --XX To do this, I should just re-write predicttion methods (i hate them now)
*      --XX Also make m_predictAhead be time ahead of current time, instead ofcgms
*      --Still need to 'clean up' after these last 2 things though
*  Make a class (or function to NLSimple) that takes a NLSimple and performs
*     'real-time' analasys.
*  Add a belowBgBasalSigma, and aboveBgBasalSigma to NLSimple Class
*    --to be used in the 'Solver' that solves what correction needs to be taken
*  The 'Solver' should decide what correction should be taken (maybe make a correction class)
*      to describe what correction should be taken
*
*  Convert ConsentrationGraph class to use PosixTime instead of double for times
*    --I think this will save CPU time, as well as bugs
*  Add ability to add more CGMS/Meal/Insulin data to the model, via the gui
*  Add ability to do training in a set of time ranges
*    --inprinciple this is there - but add gui interface for it
*    --have a default range selector (so times when cgms isn't in use won't be used)
*******************************************************************************/

class TVirtualPad;
class NLSimple;

class TSpline3;
class EventDef;

enum EventDefType
{
  IndependantEffect,
  MultiplyInsulin,
  MultiplyCarbConsumed,
  NumEventDefTypes
};//enum EventDefType
    
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
    
    double                  m_effectiveDof; //So Minuit2 can properly interpret errors
    DVec                    m_paramaters;           //size == NumNLSimplePars
    DVec                    m_paramaterErrorPlus;
    DVec                    m_paramaterErrorMinus;
    
    
    typedef std::map<int, EventDef> EventDefMap;
    typedef EventDefMap::iterator EventDefIter;
    typedef std::pair<const int, EventDef> EventDefPair;
    EventDefMap m_customEventDefs;
    
    
    ConsentrationGraph m_cgmsData;
    ConsentrationGraph m_freePlasmaInsulin;
    ConsentrationGraph m_glucoseAbsorbtionRate;
    ConsentrationGraph m_mealData;
    ConsentrationGraph m_fingerMeterData;
    ConsentrationGraph m_customEvents;
    
    ConsentrationGraph m_predictedInsulinX;       //currently stored 10x what I use, bug waiting to happen, should be changed some time
    ConsentrationGraph m_predictedBloodGlucose;
    
    
    PTimeVec m_startSteadyStateTimes;
    
    NLSimpleGui *m_gui;  //This is only non-NULL when program is inside of a GUI
                         //  this pointer is used to communicate to NLSimpleGui
                         //  since root/cint sucks and I can't just use signals
                         //  and slots comm. for this class
    
    //Start constructors/member-functions
    NLSimple( std::string fileName );
    NLSimple( const std::string &description, 
              double basalUnitsPerKiloPerhour,
              double basalGlucoseConcen = PersonConstants::kBasalGlucConc, 
              PosixTime t0 = kGenericT0 );
    const NLSimple &operator=( const NLSimple &rhs );
    
    ~NLSimple() {};
    
    double getOffset( const PosixTime &absoluteTime ) const;
    PosixTime getAbsoluteTime( double nOffsetMinutes ) const;
    

    
    //Functions for integrating the kinetic equations
    double dGdT( const PosixTime &time, double G, double X ) const;
    double dXdT( const PosixTime &time, double G, double X ) const;
    double dXdT_usingCgmsData( const PosixTime &time, double X ) const;
    double customEventEffect( const PosixTime &time, 
                              const double &carbAbsRate, const double &X ) const;
    
    DVec dGdT_and_dXdT( const PosixTime &time, const DVec &G_and_X ) const;
    RK_PTimeDFunc getRKDerivFunc() const;
    
    static double getBasalInsulinConcentration( double unitesPerKiloPerhour );
    void addCgmsData( const ConsentrationGraph &newData, 
                        bool findNewSteadyState = false );
    void addCgmsData( PosixTime, double value );
    void addBolusData( const ConsentrationGraph &newData, 
                       bool finNewSteadyStates = false );
    
    //uses default absorption rates
    void addConsumedGlucose( PosixTime time, double amount ); 
    
    //if you pass in carbs consumed, just uses default absorption rate
    //  passing in glucose absorption rate is preffered method
    void addGlucoseAbsorption( const ConsentrationGraph &newData ); 
    
    void addFingerStickData( const PosixTime &time, double value );
    void addFingerStickData( const ConsentrationGraph &newData );
    
    bool defineCustomEvent( int recordType, std::string name, 
                            TimeDuration eventDuration, 
                            EventDefType eventType, 
                            DVec initialPars );
    bool defineCustomEvent( int recordType, std::string name,
                            TimeDuration eventDuration, 
                            EventDefType eventType,
                            unsigned int nDataPoints );
    bool undefineCustomEvent( int recordType );
    
    void addCustomEvent( const PosixTime &time, int eventType );
    void addCustomEvents( const ConsentrationGraph &newEvents );
    
    void resetPredictions();
    void setModelParameters( const std::vector<double> &newPar );
    void setModelParameterErrors( std::vector<double> &newParErrorLow, 
                                  std::vector<double> &newParErrorHigh );
    
    TimeDuration findCgmsDelayFromFingerStick() const;
    double findCgmsErrorFromFingerStick( const TimeDuration cgms_delay ) const;
    
    PosixTime findSteadyStateStartTime( PosixTime t_start, PosixTime t_end );
    void findSteadyStateBeginings( double nHoursNoInsulinForFirstSteadyState = 3.0 );
    
    //makeGlucosePredFromLastCgms(...) 
    //  updates predX if it needs to cmgsEndTime-m_cgmsDelay
    //  makes prediction starting at cmgsEndTime+m_dt, and ending at
    //  cmgsEndTime+m_predictAhead.
    //  The concentration graph you pass in will be cleared before use
    //  *Note* for this funciton predictions are made m_predictAhead time ahead
    //         of the latest cgms measurment, so really the predictions are
    //         m_predictAhead + m_cgmsDelay ahead of last known BG
    void makeGlucosePredFromLastCgms( ConsentrationGraph &predBg, 
                                      PosixTime simulateCgmsEndTime = kGenericT0 );
    
    //getGraphOfMaxTimePredictions(...)
    //  calls makeGlucosePredFromLastCgms(...) to make a graph showing what 
    //  the predictions are/were for m_predictAhead of cgms readings.
    //  The result will go from firstCgmsTime+m_predictAhead 
    //  to lastCgmsTime+m_predictAhead.  if firstCgmsTime not specified, will
    //  start at first steadyState X point, 
    //  LastCgmsTime not specified, end at last m_cgmsData point+m_predictAhead
    //  The concentration graph you pass in will be cleared before use
    //  If lastPointChi2Weight between 0.0 and 1.0 is specified, then a
    //  chi2 will be computed, were 0.0 gives every point of every prediction
    //  equal weight, and 1.0 only gives the last point any weight
    double getGraphOfMaxTimePredictions( ConsentrationGraph &predBg,
                                       PosixTime firstCgmsTime = kGenericT0,
                                       PosixTime lastCgmsTime = kGenericT0,
                                       double lastPointChi2Weight = kFailValue );
    
    
    
    //Dont't use glucPredUsingCgms or performModelGlucosePrediction,
    //  these have been surplanted by makeGlucosePredFromLastCgms(...) and 
    //  getGraphOfMaxTimePredictions(...)
    ConsentrationGraph glucPredUsingCgms( int nMinutesPredict = -1,  //nMinutes ahead of cgms
                                                                     //if <=0, uses m_predictAhead
                                          PosixTime t_start = kGenericT0,
                                          PosixTime t_end   = kGenericT0 );
    
    ConsentrationGraph performModelGlucosePrediction( 
                                          PosixTime t_start = kGenericT0,
                                          PosixTime t_end = kGenericT0,
                                          double bloodGlucose_initial = kFailValue,
                                          double bloodX_initial = kFailValue );
    
    //returns true  if all information is updated to time
    //  Only modifes cgmsData, X, and predictedGlucose graphs
    //  X and predictedGlucose will end at cgmsEndTime - m_cgmsDelay
    bool removeInfoAfter( const PosixTime &cgmsEndTime, bool removeCgms = true );
    
    void updateXUsingCgmsInfo( bool recomputeAll = true );
    
    double getModelChi2( double fracDerivChi2 = 0.0,
                         PosixTime t_start = kGenericT0,
                         PosixTime t_end = kGenericT0 );
    
    //useAssymetricErrors - set 'false' for determining accuracy, 'true' for optimizing insulin to be taken
    double getChi2ComparedToCgmsData( ConsentrationGraph &inputData,
                                      double fracDerivChi2 = 0.0,
                                      bool useAssymetricErrors = false,
                                      PosixTime t_start = kGenericT0,
                                      PosixTime t_end = kGenericT0 );
    
    //Below gives chi^2 based only on height differences of graphs
    //useAssymetricErrors - set 'false' for determining accuracy, 'true' for optimizing insulin to be taken
    double getBgValueChi2( const ConsentrationGraph &modelData,
                           const ConsentrationGraph &cgmsData,
                           bool useAssymetricErrors,
                           PosixTime t_start, PosixTime t_end ) const;
    
    //Below gives chi^2 based on the differences in derivitaves of graphs
    double getDerivativeChi2( const ConsentrationGraph &modelDerivData,
                              const ConsentrationGraph &cgmsDerivData,
                              PosixTime t_start, PosixTime t_end ) const;
    //want to add a variable binning chi2
    
    double getFitDof() const;
    void setFitDof( double dof );
    
    
    //For model fiting, specifying nMinutesPredict<=0.0 means don't use cgms 
    //  data tomake predictions, in which case endPredChi2Weight is predicted
    //  as what weight to give to the derivative based chi2
    double geneticallyOptimizeModel( double endPredChi2Weight,
                                     TimeRangeVec timeRanges = TimeRangeVec(0) );
    
    double fitModelToDataViaMinuit2( double endPredChi2Weight,
                                     TimeRangeVec timeRanges = TimeRangeVec(0) );
    
    DVec chi2DofStudy( double endPredChi2Weight,
                             TimeRangeVec timeRanges = TimeRangeVec(0) ) const;
    
    // timeRanges -- the time range of events your fitting
    // paramaterV -- contains answers and starting values
    // resultGV   -- the result of the fit, must be of the proper type of graph
    bool fitEvents( TimeRangeVec timeRanges, 
                    std::vector< std::vector<double> *> &paramaterV, 
                    std::vector<ConsentrationGraph *> resultGV );
    
    friend class NLSimpleGui;
    void runGui();
    void draw( bool pause = true, PosixTime t_start = kGenericT0,
               PosixTime t_end = kGenericT0 );
    
    static std::string convertToRootLatexString( double num, int nPrecision  );
    std::vector<std::string> getEquationDescription() const;
    
    bool saveToFile( std::string filename = "" );
    
    friend class boost::serialization::access;
  private:
    template<class Archive>
    void serialize( Archive &ar, const unsigned int version );
};//class NLSimple


//An interace for NLSimple to Minuit2 and the Genetic Optimizer
class ModelTestFCN : public ROOT::Minuit2::FCNBase, public TMVA::IFitterTarget
{
  public:
    //function forMinuit2
    virtual double operator()(const std::vector<double>& x) const;
    virtual double Up() const;
    virtual void SetErrorDef(double dof);
    
    //Function for TMVA fitters
    virtual Double_t EstimatorFunction( std::vector<Double_t>& parameters );
    
    //the function that does the actual work
    double testParamaters(const std::vector<double>& x, bool updateModel ) const;
    
    ModelTestFCN( NLSimple *modelPtr, 
                  double endPredChi2Weight,
                  std::vector<TimeRange> timeRanges );
    
    virtual ~ModelTestFCN(){}
  
  private:
    NLSimple *m_modelPtr;
    double    m_endPredChi2Weight;
    
    std::vector<TimeRange> m_timeRanges;
    PosixTime m_tStart;
    PosixTime m_tEnd;
};//class ModelTestFCN



//An interace for NLSimple to Minuit2 and the Genetic Optimizer
// To find the CGMS delay behind fingersticks
class CgmsFingerCorrFCN : public ROOT::Minuit2::FCNBase, public TMVA::IFitterTarget
{
  public:
    //function forMinuit2
    virtual double operator()(const std::vector<double>& x) const;
    virtual double Up() const;
    virtual void SetErrorDef(double dof);
    
    //Function for TMVA fitters
    virtual Double_t EstimatorFunction( std::vector<Double_t>& parameters );
    
    //the function that does the actual work
    double testParamaters(const std::vector<double>& x ) const;
    
    CgmsFingerCorrFCN( const ConsentrationGraph &cgmsData, 
                       const ConsentrationGraph &fingerstickData
                     );
    
    virtual ~CgmsFingerCorrFCN();
  
  private:
    const ConsentrationGraph &m_cgmsData;
    const ConsentrationGraph &m_fingerstickData;
};//class CgmsFingerCorrFCN


// namespace ROOT{ namespace Minuit2{ class MnMigrad;};};


//This class fits for events such as unrecorded meal or unrecorded injection
//  currently only GraphType's: 
//  BolusGraph and InsulinGraph (modeled with novologConsentrationGraph(...)), 
//  GlucoseAbsorbtionRateGraph, and GlucoseConsumptionGraph 
//  (modeled with yatesGlucoseAbsorptionRate(...)) are fit for.
//  This class to be used with ROOT::Minuit2::MnMigrad, or TMVA based fitters
//  As with everything else, this class is very un-optimized
class FitNLSimpleEvent: public ROOT::Minuit2::FCNBase, public TMVA::IFitterTarget
{
  //TO DO:1)When fitting multiple events of the same type, right now Minuit2 
  //        will treat the paramaters of each event seperately.  I should add
  //        a term to the chi2 to tie these paramaters together.  This will
  //        necessatate an accessor to get the mean and error of the paramaters
  //      2)Create a method to constrain magnitude of events within an uncert.
  
  public:
    FitNLSimpleEvent( const NLSimple *model );
    ~FitNLSimpleEvent(){};
    
    //Event type to be fitted for is determined via fitResult->getGraphType()
    //  *fitResult and *pars must remain to be valid objects 
    //  returns the place in m_fitForEvents this graph ocupies
    //  InsulinGraph and BolusGraph must have 1 paramater in vector (amount insulin)
    //  GlucoseAbsorbtionRateGraph and BloodGlucoseConcenDeriv
    //  must have 1 (amount ingected) or 5 paramaters (the Yates model paramters)
    unsigned int addEventToFitFor( ConsentrationGraph *fitResult,
                                   PosixTime *startTime,
                                   const TimeDuration &timeUncert,
                                   std::vector<double> *pars );
    
    //function forMinuit2
    //below parameters are ordered by m_fitParamters 
    // (m_fitParamters[0][0], m_fitParamters[0][1]..., t0,m_fitParamters[1][0] ),
    // t0 is number minutes after m_fitForEvents[i]->m_t0
    virtual double operator()(const std::vector<double>& parameters) const;
    virtual double Up() const;
    virtual void SetErrorDef(double dof);
    
    //Function for TMVA fitters
    virtual Double_t EstimatorFunction( std::vector<Double_t>& parameters );
    
  private:
    const NLSimple *m_model; //model passed in to the constructor
  
    //Each graph in m_fitForEvents gets it's own set of paramaters
    //  the paramaters passed in through addEventToFitFor(...) are used
    //  for storage as well as modified with intermediate and final result
    //  below are mutable so operator() can stay const as required by Minuit2
    //  (I could have programmed this class better so this wasn't necassary)
    mutable std::vector<std::vector<double> *>  m_fitParamters; 
    
    mutable std::vector<PosixTime *>            m_startTimes;
    mutable std::vector<TimeDuration>           m_startTimeUncerts;
    mutable std::vector<ConsentrationGraph *>   m_fitForEvents;
    
    NLSimple getModelForNewFit() const; //fills in model returned with current guesses
    
    //below calls updateFitForEvents() and then add these to the model passed in
    //  also updates m_fitForEvents from m_fitParamters
    //  should be called from within getModelForNewFit() only.
    void updateModelWithCurrentGuesses( NLSimple &model ) const; 
};//class FitNLSimpleEvent


//fit a smooth curve to a finite number of points
class EventDef
{
  friend class NLSimple;
  friend class boost::serialization::access;
  
  private:
    std::string m_name;
    mutable TSpline3 *m_spline;
    
    double *m_times;
    double *m_values;
    unsigned int m_nPoints;
    TimeDuration m_duration;
    EventDefType m_eventDefType;
    
    void buildSpline() const;
    
  public:
    EventDef();
    EventDef( const EventDef &rhs );
    const EventDef &operator=( const EventDef &rhs );
    EventDef( std::string name, TimeDuration eventLength, 
              EventDefType defType, unsigned int nPoints);
    ~EventDef();
    
    unsigned int getNPoints() const;
    double getPar( unsigned int parNum ) const;
    void setPar( unsigned int point, double value );
    void setPar( const std::vector<double> &par );
    
    double eval( const TimeDuration &dur ) const;
    
    void setName( const std::string name );
    const std::string &getName() const;
    const TimeDuration &getDuration() const;
    const EventDefType &getEventDefType() const;
    
    void draw() const;
    
  private:
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const;
    template<class Archive>
    void load(Archive & ar, const unsigned int version);
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};//EventDef



void drawClarkeErrorGrid( TVirtualPad *pad,
                          const ConsentrationGraph &cmgsGraph, 
                          const ConsentrationGraph &meterGraph,
                          const TimeDuration &cmgsDelay,
                          bool isCgmsVsMeter );

std::vector<TObject *> getClarkeErrorGridObjs( const ConsentrationGraph &cmgsGraph, 
                                               const ConsentrationGraph &meterGraph,
                                               const TimeDuration &cmgsDelay,
                                               bool isCgmsVsMeter );



BOOST_CLASS_VERSION(NLSimple, 0)
BOOST_CLASS_VERSION(EventDef, 0)


#endif //RESPONSE_MODEL_HH
