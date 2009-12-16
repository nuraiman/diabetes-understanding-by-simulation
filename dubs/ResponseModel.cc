//
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <stdio.h>
#include <math.h>  //contains M_PI
#include <stdlib.h>
#include <fstream>
#include <algorithm> //min/max_element
#include <float.h> // for DBL_MAX

#include "TH1F.h"
#include "TH2D.h"
#include "TLine.h"
#include "TText.h"
#include "TLegend.h"
#include "TSpline.h"
#include "TGraph.h"
#include "TGAxis.h"
#include "TCanvas.h"
#include "TRandom3.h"
#include "TGraph2d.h"
#include "TLegendEntry.h"
#include "TApplication.h"


//Roots Minuit2 includes
#include "Minuit2/FCNBase.h"
#include "Minuit2/FunctionMinimum.h"
#include "Minuit2/MnMigrad.h"
#include "Minuit2/MnMinos.h"
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/MnUserParameterState.h"
#include "Minuit2/MnPrint.h"
#include "Minuit2/SimplexMinimizer.h"

//Roots TMVA includes
#include "TMVA/IFitterTarget.h"
#include "TMVA/GeneticAlgorithm.h"


#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/time_serialize.hpp"
#include "boost/date_time/gregorian/greg_serialize.hpp"
#include "boost/foreach.hpp"
#include "boost/bind.hpp"
#include "boost/ref.hpp"
#include "boost/assign/list_of.hpp" //for 'list_of()'
#include "boost/assign/list_inserter.hpp"
#include <boost/serialization/set.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include "boost/format.hpp"

#include "ResponseModel.hh"
#include "KineticModels.hh"
#include "CgmsDataImport.hh"
#include "ArtificialPancrease.hh"
#include "RungeKuttaIntegrater.hh"


using namespace std;
using namespace boost;
using namespace boost::posix_time;


extern TApplication *gTheApp;


/*
NLSimple::NLSimple( const NLSimple &rhs ) :
     m_description( rhs.m_description ), 
     m_cgmsDelay( rhs.m_cgmsDelay ),
     m_basalInsulinConc( rhs.m_basalInsulinConc ),
     m_basalGlucoseConcentration( rhs.m_basalGlucoseConcentration ), 
     m_t0( rhs.m_t0 ),
     m_dt( ModelDefaults::kIntegrationDt ),
     m_predictAhead( ModelDefaults::kPredictAhead ), 
     m_effectiveDof( 1.0 ),
     m_paramaters( rhs.m_paramaters ),
     m_paramaterErrorPlus( rhs.m_paramaterErrorPlus ), 
     m_paramaterErrorMinus( rhs.m_paramaterErrorMinus ),
     m_cgmsData( rhs.m_cgmsData ),
     m_freePlasmaInsulin( rhs.m_freePlasmaInsulin ),
     m_glucoseAbsorbtionRate( rhs.m_glucoseAbsorbtionRate ),
     m_predictedInsulinX( rhs.m_predictedInsulinX ),
     m_predictedBloodGlucose( rhs.m_predictedBloodGlucose ),
     m_startSteadyStateTimes( rhs.m_startSteadyStateTimes ), m_gui(NULL)
{
}//NLSimple( const NLSimple &rhs )
*/

NLSimple::NLSimple( const string &description, double basalUnitsPerKiloPerhour,
                    double basalGlucoseConcen, boost::posix_time::ptime t0 ) :
m_description(description), m_cgmsDelay( ModelDefaults::kDefaultCgmsDelay ),
     m_basalInsulinConc( getBasalInsulinConcentration(basalUnitsPerKiloPerhour) ),
     m_basalGlucoseConcentration( basalGlucoseConcen ), 
     m_t0( t0 ),
     m_dt( ModelDefaults::kIntegrationDt ),
     m_predictAhead( ModelDefaults::kPredictAhead ),
     m_effectiveDof(1.0),
     m_paramaters(NumNLSimplePars, kFailValue),
     m_paramaterErrorPlus(0), m_paramaterErrorMinus(0),
     m_cgmsData(t0, 5.0, GlucoseConsentrationGraph),
     m_freePlasmaInsulin(t0, 5.0, InsulinGraph),
     m_glucoseAbsorbtionRate(t0, 5.0, GlucoseAbsorbtionRateGraph),
     m_mealData(t0, 5.0, GlucoseConsumptionGraph),
     m_fingerMeterData(t0, 5.0, GlucoseConsentrationGraph),
     m_customEvents(t0, 5.0, CustomEvent),
     m_predictedInsulinX(t0, 5.0, InsulinGraph),
     m_predictedBloodGlucose(t0, 5.0, GlucoseConsentrationGraph),
     m_startSteadyStateTimes(0),
     m_gui(NULL)
{
}//NLSimple construnctor




NLSimple::NLSimple( std::string fileName ) :
     m_description(""), m_cgmsDelay( ModelDefaults::kDefaultCgmsDelay ),
     m_basalInsulinConc( kFailValue ),
     m_basalGlucoseConcentration( kFailValue ), 
     m_t0( kGenericT0 ),
     m_dt( ModelDefaults::kIntegrationDt ),
     m_predictAhead( ModelDefaults::kPredictAhead ),
     m_effectiveDof(1.0),
     m_paramaters(NumNLSimplePars, kFailValue),
     m_paramaterErrorPlus(0), m_paramaterErrorMinus(0),
     m_cgmsData(kGenericT0, 5.0, GlucoseConsentrationGraph),
     m_freePlasmaInsulin(kGenericT0, 5.0, InsulinGraph),
     m_glucoseAbsorbtionRate(kGenericT0, 5.0, GlucoseAbsorbtionRateGraph),
     m_mealData(kGenericT0, 5.0, GlucoseConsumptionGraph),
     m_fingerMeterData(kGenericT0, 5.0, GlucoseConsentrationGraph),
     m_customEvents(kGenericT0, 5.0, CustomEvent),
     m_predictedInsulinX(kGenericT0, 5.0, InsulinGraph),
     m_predictedBloodGlucose(kGenericT0, 5.0, GlucoseConsentrationGraph),
     m_startSteadyStateTimes(0), 
     m_gui(NULL)
{   
  if( fileName == "" )
  {
    fileName = NLSimpleGui::getFileName( true );
    
    if( fileName.empty() ) 
    {
      cout << "NLSimple::Warning, model will be default" << endl;
      return;
    }//if( fileName.empty() ) 
  }//if( fileName == "" )
  
  
  unsigned int beginExtension = fileName.find_last_of( "." );
  string extention = fileName.substr( beginExtension );
  if( extention != ".dubm" ) fileName += ".dubm";
  
  
  std::ifstream ifs( fileName.c_str() );
  if( !ifs.is_open() )
  {
    cout << "Couldn't open file " << fileName << " for reading" << endl;
    exit(1);
  }//if( !ofs.is_open() )
  
  boost::archive::text_iarchive ia(ifs);
  
  ia >> *this;  // restore from the archive  
}//NLSimple::NLSimple





//if you are updatin this, make sure you updarte the serialize function!!!
const NLSimple &NLSimple::operator=( const NLSimple &rhs )
{
  if( &rhs == this ) return *this;
  
  m_description                = rhs.m_description;
    
  m_t0                         = rhs.m_t0;
  m_dt                         = rhs.m_dt;
  m_predictAhead               = rhs.m_predictAhead;
  
  m_cgmsDelay                  = rhs.m_cgmsDelay;
  m_basalInsulinConc           = rhs.m_basalInsulinConc;
  m_basalGlucoseConcentration  = rhs.m_basalGlucoseConcentration;
  
  m_effectiveDof               = rhs.m_effectiveDof;
  
  m_paramaters                 = rhs.m_paramaters;
  m_paramaterErrorPlus         = rhs.m_paramaterErrorPlus;
  m_paramaterErrorMinus        = rhs.m_paramaterErrorMinus;
  m_customEventDefs            = rhs.m_customEventDefs;
  
  m_cgmsData                   = rhs.m_cgmsData;
  m_freePlasmaInsulin          = rhs.m_freePlasmaInsulin;
  m_glucoseAbsorbtionRate      = rhs.m_glucoseAbsorbtionRate;
  m_fingerMeterData            = rhs.m_fingerMeterData;
  m_customEvents               = rhs.m_customEvents;
  m_mealData                   = rhs.m_mealData;
  
  m_predictedInsulinX          = rhs.m_predictedInsulinX;
  m_predictedBloodGlucose      = rhs.m_predictedBloodGlucose;
 
  m_startSteadyStateTimes      = rhs.m_startSteadyStateTimes;
   
  m_gui = NULL; 
  
  return *this;
}//operator=



double NLSimple::getOffset( const boost::posix_time::ptime &absoluteTime ) const
{
  return ConsentrationGraph::getDt( m_t0, absoluteTime );
}//getOffset


ptime NLSimple::getAbsoluteTime( double nOffsetMinutes ) const
{
  return m_t0 + toTimeDuration(nOffsetMinutes);
}//ptime getAbsoluteTime( double nOffsetMinutes ) const;


double NLSimple::customEventEffect( const PosixTime &time, 
                                    const double &carbAbsRate, const double &X ) const
{
  double dGdT = 0.0;
  
  foreach( const EventDefPair &et, m_customEventDefs )
  {
    double mult = 1.0;
    switch(et.second.m_eventDefType)
    {
      case IndependantEffect:    mult = 1.0;         break;
      case MultiplyInsulin:      mult = X;           break;
      case MultiplyCarbConsumed: mult = carbAbsRate; break;
      case NumEventDefTypes: assert(0);
    };//switch(eventType)
    
    ConstGraphIter lb = m_customEvents.lower_bound(time - et.second.m_duration);
    ConstGraphIter ub = m_customEvents.upper_bound(time);
    
    for( ConstGraphIter iter = lb; iter != ub; ++iter )
    {
      if( static_cast<int>(iter->m_value) == et.first )
      {
        dGdT += mult * et.second.eval( time - iter->m_time );
      }//if( corect custom event type )
    }//for( loop over possible custom events )
  }//foreach( defined custom event type )
  
  return dGdT;
}//double NLSimple::customEventEffect( const PosixTime &time,



//Functions for integrating the kinetic equations
double NLSimple::dGdT( const ptime &time, double G, double X ) const
{
  assert( m_paramaters.size() == NumNLSimplePars );
  
  const double dCarAbsorbDt = m_glucoseAbsorbtionRate.value( time );
  
  double dGdT = (-m_paramaters[BGMultiplier]*G) 
                - X*(G + m_basalGlucoseConcentration) 
                + m_paramaters[CarbAbsorbMultiplier] * dCarAbsorbDt;
                // + G_parameter[1]*I_basal;
                + customEventEffect( time, dCarAbsorbDt, X );
                
  return dGdT;
}//dGdT




double NLSimple::dXdT( const ptime &time, double G, double X ) const
{
  G = G;//keep compiler from complaining
  assert( m_paramaters.size() == NumNLSimplePars );
    
  //The 10 is for convertin U/L, to U/dL
  const double insulin = (m_freePlasmaInsulin.value( time ) / 10.0);
  
  double dXdT = -X * m_paramaters[XMultiplier]
                + insulin * m_paramaters[PlasmaInsulinMultiplier];
               //+ X_parameters[2] *(G - X_parameters[3]);
  
  return dXdT; 
}//dXdT


double NLSimple::dXdT_usingCgmsData( const ptime &time, double X ) const
{
  // const ptime absCgmsPTime = time + m_cgmsDelay;
  
  //The 10 is for convertin U/L, to U/dL
  const double insulin= (m_freePlasmaInsulin.value( time ) / 10.0);
  
  double dXdT = -X * m_paramaters[XMultiplier]
                + insulin * m_paramaters[PlasmaInsulinMultiplier];
               //+ X_parameters[2] *(G - X_parameters[3]);
               ;
  return dXdT; 
}//double dXdT_usingCgmsData( double time, double X ) const

    


vector<double> NLSimple::dGdT_and_dXdT( const boost::posix_time::ptime &time, 
                                        const std::vector<double> &G_and_X ) const
{
  assert( G_and_X.size() == 2 );

  const double G = G_and_X[0];
  const double X = G_and_X[1];
  
  vector<double> rVal(2);
  rVal[0] = dGdT(time, G, X );
  rVal[1] = dXdT(time, G, X );
  
  return rVal;
}//dGdT_and_dXdT


RK_PTimeDFunc NLSimple::getRKDerivFunc() const
{
  return bind( &NLSimple::dGdT_and_dXdT, boost::cref(*this), _1, _2 );
}//getRKDerivFunc()

bool NLSimple::undefineCustomEvent( int recordType )
{
  EventDefIter iter = m_customEventDefs.find(recordType);
  
  if( iter == m_customEventDefs.end() ) return false; 
    
  m_customEventDefs.erase(iter);
  return true;
}//bool NLSimple::undefineCustomEvent( int recordType )


bool NLSimple::defineCustomEvent( int recordType, std::string name, 
                                  TimeDuration eventDuration, 
                                  EventDefType et,
                                  unsigned int nDataPoints )
{
  //make sure we don't already have a record
  if( m_customEventDefs.find(recordType) != m_customEventDefs.end() )
  {
    EventDef &ev = m_customEventDefs[recordType];
    
    if( (ev.getNPoints() == nDataPoints) && (ev.getEventDefType() == et) )
    {
      ev.setName( name );
      return false;
    }//if( just changing name )
  }//if( already have a recond of this type )
  // m_customEventDefs.insert( make_pair(recordType, EventDef() ) );
  
  m_customEventDefs[recordType] = EventDef( name, eventDuration, et, nDataPoints);
  
  return true;
}//void NLSimple::defineCustomEvent( int recordType, std::string name, int nDataPoints )



bool NLSimple::defineCustomEvent( int recordType, std::string name,
                                  TimeDuration eventDuration, 
                                  EventDefType et, 
                                  std::vector<double> initialPars )
{
  bool added = defineCustomEvent( recordType, name, eventDuration, et, initialPars.size() );
  
  m_customEventDefs[recordType].setPar(initialPars);
  
  return added;
}//void NLSimple::defineCustomEvent( int recordType, std::string name, std::vector<double> initialPars )



double NLSimple::getBasalInsulinConcentration( double unitsPerKiloPerhour )
{
  if( unitsPerKiloPerhour <= 0.0 )
  {
    cout << "Warning: you have selected a basil insulin rate of "
         << unitsPerKiloPerhour << "Units Per Kilo Per hour"
         << ". I am making basal insulin concentration equal 0.0" << endl;
    return 0.0;
  }//if( unitsPerKiloPerhour <= 0.0 )
  
  ConsentrationGraph basalConcen( kGenericT0, 5.0, InsulinGraph );
  
  const double uPer5Min = unitsPerKiloPerhour / 12.0;
  // const double nMinuteDoBasal = 15.0 * 60.0; //15 hours
  const PosixTime endTime = kGenericT0 + minutes( 15 * 60 );
  
  for( PosixTime time = kGenericT0; time <= endTime; time += minutes(5) )
  {
    basalConcen.add( uPer5Min, time, NovologAbsorbtion );
  }//for( loop over time making a basal rate )
  
  unsigned int nPoints = 0;
  double basalConc = 0.0;
  
  const PosixTime startSteadyState = endTime - minutes(5*60);
    
  for( PosixTime time = startSteadyState; time <= endTime; time += minutes(5.0) )
  {
    ++nPoints;
    basalConc += basalConcen.value( time );
  }//for( loop over time making a basal rate )
  
  basalConc /= nPoints;
  
  cout << "void NLSimple::setBasalInsulinAmount( " << unitsPerKiloPerhour << ")"
       << " says basal insulin concentration is " << basalConc 
       << "milli-Units per Liter" << endl;
       
  return basalConc;
}//setBasalInsulinAmount


void NLSimple::addBolusData( const ConsentrationGraph &newData,
                             bool finNewSteadyStates  )
{
  if( newData.getGraphType() == InsulinGraph )
  {
    m_freePlasmaInsulin = m_freePlasmaInsulin.getTotal( newData );
  }else if( newData.getGraphType() == BolusGraph )
  {
    ConsentrationGraph
    insulinConc = CgmsDataImport::bolusGraphToInsulinGraph( newData, 
                                             PersonConstants::kPersonsWeight );
    m_freePlasmaInsulin = m_freePlasmaInsulin.getTotal( insulinConc );
  }else 
  {
    cout << "void NLSimple::addBolusData( ConsentrationGraph graph ): "
         << " graph must be of type InsulinGraph or BolusGraph" << endl;
         
    exit(1);
  }//if( check which type of data was passed in ) /else
    
  if(finNewSteadyStates) findSteadyStateBeginings();
}//addBolusData


void NLSimple::addCgmsData( const ConsentrationGraph &newData, 
                            bool finNewSteadyState )
{
  m_cgmsData = m_cgmsData.getTotal( newData );
  
  if(finNewSteadyState) findSteadyStateBeginings();
}//void NLSimple::addCgmsData( const ConsentrationGraph &newData )


void NLSimple::addCgmsData( boost::posix_time::ptime time, double value )
{
  m_cgmsData.addNewDataPoint( time, value );
}//void NLSimple::addCgmsData( boost::posix_time::ptime, double value )






void NLSimple::addConsumedGlucose( ptime time, double amount )
{
  ConsentrationGraph consumpRate( time, 5.0, GlucoseConsumptionGraph);
  consumpRate.insert( time, amount );
  addGlucoseAbsorption( consumpRate );
}//addConsumedGlucose



void NLSimple::addFingerStickData( const PosixTime &time, double value )
{
  ConsentrationGraph newData(m_t0, 5.0, GlucoseConsentrationGraph);
  newData.insert( time, value );
  addFingerStickData( newData );
}//void NLSimple::addFingerStickData( PosixTime, double value )


void NLSimple::addFingerStickData( const ConsentrationGraph &newData )
{
  foreach( const GraphElement &el, newData )
  {
    m_fingerMeterData.addNewDataPoint( el.m_time, el.m_value );
  }//foreach(....)
}//void NLSimple::addFingerStickData( const ConsentrationGraph &newData )



void NLSimple::addCustomEvent( const PosixTime &time, int eventType )
{
  ConsentrationGraph newData(m_t0, 5.0, CustomEvent);
  newData.insert( time, eventType );
  addCustomEvents( newData );
}//void NLSimple::addCustomEvent( const PosixTime &time, int eventType )



void NLSimple::addCustomEvents( const ConsentrationGraph &newEvents )
{
  foreach( const GraphElement &el, newEvents )
  {
    m_customEvents.addNewDataPoint( el.m_time, el.m_value );
  }//foreach(....)
}//void NLSimple::addCustomEvents( const ConsentrationGraph &newEvents )



void NLSimple::addGlucoseAbsorption( const ConsentrationGraph &newData )
{
  if( newData.getGraphType() == GlucoseAbsorbtionRateGraph )
  {
    m_glucoseAbsorbtionRate = m_glucoseAbsorbtionRate.getTotal( newData );
  }else if( newData.getGraphType() == GlucoseConsumptionGraph )
  {
    foreach( const GraphElement &el, newData )
    {
      m_mealData.addNewDataPoint( el.m_time, el.m_value );
    }//foreach(....)
    
    ConsentrationGraph
    absorbRate = 
    CgmsDataImport::carbConsumptionToSimpleCarbAbsorbtionGraph( newData );
    m_glucoseAbsorbtionRate = m_glucoseAbsorbtionRate.getTotal( absorbRate );
  }else
  {
    cout << "void NLSimple::addGlucoseAbsorption( ConsentrationGraph graph ): "
         << " graph must be of type GlucoseAbsorbtionRateGraph or "
         << " GlucoseConsumptionGraph" << endl;
    exit(1);
  }//if( check which type of data was passed in ) /else
}//addConsumedGlucose




double CgmsFingerCorrFCN::operator()(const std::vector<double>& x) const
{
  return testParamaters(x);
}//operator()

Double_t CgmsFingerCorrFCN::EstimatorFunction( std::vector<Double_t>& parameters )
{
  return testParamaters(parameters);
}//EstimatorFunction(...)

double CgmsFingerCorrFCN::testParamaters(const std::vector<double>& x ) const
{
  assert( x.size() == 1 );
  if( m_fingerstickData.empty() || m_cgmsData.empty() ) return 99999.9;
    
  const double fracUncert = 0.10; //this is for value of cgms
  //should we have an uncertainty based on the rate of change of the cgms?
  
  double chi2 = 0.0;
  const TimeDuration delay = toTimeDuration( x[0] );

  foreach( const GraphElement &el, m_fingerstickData )
  {
    const PosixTime &time     = el.m_time;
    const double fingerValue = el.m_value;
    const double cgmsValue   = m_cgmsData.value(time + delay);
    
    const ConstGraphIter nextTimeIter = m_cgmsData.lower_bound(time + delay);
    ConstGraphIter previousTimeIter   = nextTimeIter;
    --previousTimeIter;
    
    if( nextTimeIter == m_cgmsData.end() || nextTimeIter == m_cgmsData.begin() ) continue;
    
    const PosixTime nextTime = nextTimeIter->m_time;
    const PosixTime prevTime = previousTimeIter->m_time;
    
    TimeDuration prevDur = time + delay - prevTime ;
    TimeDuration nextDur = nextTime - time - delay;
    if( nextDur.is_negative() ) nextDur = -nextDur;
    if( prevDur.is_negative() ) prevDur = -prevDur;
    
    const TimeDuration maxTimeToCgms = max( nextDur, prevDur );
    
    if( maxTimeToCgms < TimeDuration(0, 16, 0, 0) )
    {
      const double uncert2 = pow( fracUncert * fingerValue, 2 );
      assert( uncert2 != 0.0 );
      chi2 += pow( fingerValue - cgmsValue, 2) / uncert2;
    }//if( maxTimeToCgms > TimeDuration(0, 16, 0, 0) )
  }//foreach( fingerstick data point )
   
   return chi2;
}//testParamaters(...)


double CgmsFingerCorrFCN::Up() const {return 1.0;}
void CgmsFingerCorrFCN::SetErrorDef(double dof){ dof = dof; }



    
CgmsFingerCorrFCN::CgmsFingerCorrFCN( const ConsentrationGraph &cgmsData, 
                                      const ConsentrationGraph &fingerstickData ) : 
                    m_cgmsData(cgmsData), m_fingerstickData(fingerstickData)
{
}//CgmsFingerCorrFCN::CgmsFingerCorrFCN(...)
    
CgmsFingerCorrFCN::~CgmsFingerCorrFCN(){}
  

//We want to use something like an auto correlation function to determine
//  delay between CGMS and fingerstick data
TimeDuration NLSimple::findCgmsDelayFromFingerStick() const
{
  // return toTimeDuration(15.0);
  using namespace ROOT::Minuit2;
  CgmsFingerCorrFCN chi2Fcn( m_cgmsData,  m_fingerMeterData );
  
  MnUserParameters upar;
  upar.Add( "CGMS_delay"   , 15.0, 1.0 );
  upar.SetLimits( "CGMS_delay", 0.0, 30.0 );
  MnMigrad migrad( chi2Fcn, upar );
  // migrad.SetPrecision(0.001);
  
  FunctionMinimum min = migrad();
  
  if( !min.IsValid() ) 
  {
    cout << "First attempt to find CGMS delay failed, will retry with strategy = 2." << endl;
    MnMigrad migrad(chi2Fcn, upar, 2);
    min = migrad();
  }//if( !min.IsValid() )
  
  const double delayD = min.UserState().Value(0);
  
  cout << "Found a cgms delay of " << delayD << endl;
  
  return toTimeDuration(delayD);
}//TimeDuration NLSimple::findCgmsDelayFromFingerStick() const



double NLSimple::findCgmsErrorFromFingerStick( const TimeDuration cgms_delay ) const
{
  //We'll make an atempt to find an assymeteric error, but only return the symetrized error
  double fracDeviation2(0.0);
  int numValidPointsUp(0), numValidPointsDown(0);
  double fractionalErrUp(0.0), fractionalErrDown(0.0); 
  
  
  foreach( const GraphElement &el, m_fingerMeterData )
  {
    const PosixTime &time    = el.m_time;
    const double fingerValue = el.m_value;
    const double cgmsValue   = m_cgmsData.value(time + cgms_delay);
    
    const ConstGraphIter nextTimeIter = m_cgmsData.lower_bound(time + cgms_delay);
    ConstGraphIter previousTimeIter   = nextTimeIter;
    --previousTimeIter;
    
    if( nextTimeIter == m_cgmsData.end() || nextTimeIter == m_cgmsData.begin() ) continue;
    const PosixTime &nextTime = nextTimeIter->m_time;
    const PosixTime &prevTime = previousTimeIter->m_time;
    
    //check to make sure there is cgms readings near the fingerstick readings 
    TimeDuration prevDur = time + cgms_delay - prevTime ;
    TimeDuration nextDur = nextTime - time - cgms_delay;
    if( nextDur.is_negative() ) nextDur = -nextDur;
    if( prevDur.is_negative() ) prevDur = -prevDur;
    const TimeDuration maxTimeToCgms = max( nextDur, prevDur );
    
    if( maxTimeToCgms < TimeDuration(0, 16, 0, 0) )
    {
      // cout << "(" << fingerValue << ", " << cgmsValue << ")" << endl;
      fracDeviation2 += pow(fingerValue - cgmsValue, 2) / cgmsValue / cgmsValue;
      if( (fingerValue - cgmsValue) > 0.0 )
      {
        ++numValidPointsUp;
        fractionalErrUp += abs(fingerValue - cgmsValue) / cgmsValue;
      }else
      {
        ++numValidPointsDown;
        fractionalErrDown += abs(fingerValue - cgmsValue) / cgmsValue;
      }//
    }//if( maxTimeToCgms > TimeDuration(0, 16, 0, 0) )
    // else cout << "maxTimeToCgms=" << maxTimeToCgms << endl;
  }//foreach( fingerstick data point )
  
  if( !fractionalErrDown && !numValidPointsUp ) return kFailValue;
  
  const double upError   = numValidPointsUp  / numValidPointsUp;
  const double downError = fractionalErrDown / numValidPointsDown;
  const double symetricError = (numValidPointsUp+fractionalErrDown) / (numValidPointsUp+numValidPointsDown);
  fracDeviation2 /= ((numValidPointsUp+numValidPointsDown) * (numValidPointsUp+numValidPointsDown));
  
  cout << "The CGMS-fingerstick error is: +" << upError << ", -" << downError 
       << ", symmetrized error +-" << symetricError << ", standard deviation +-" 
       << sqrt(fracDeviation2) << endl; 
  
  return sqrt(fracDeviation2); //symetricError;
}//TimeDuration NLSimple::findCgmsErrorFromFingerStick( const TimeDuration cgms_delay ) const



void NLSimple::resetPredictions()
{
  m_predictedInsulinX.clear();
  m_predictedBloodGlucose.clear();
}//resetPredictions()


void NLSimple::setModelParameters( const std::vector<double> &newPar )
{
  //number of para
  size_t nParExp = NumNLSimplePars;
  foreach( const EventDefPair &et, m_customEventDefs )
  {
    nParExp += et.second.getNPoints();
  }//foreach( custom event def )
  
  assert( newPar.size() == nParExp || newPar.empty() );
  
  resetPredictions();
  
  size_t parNum = 0;
  for( ; parNum < NumNLSimplePars; ++parNum ) m_paramaters[parNum] = newPar[parNum];
  
  foreach( EventDefPair &et, m_customEventDefs )
  {
    const size_t nPoints = et.second.getNPoints();
    
    for( size_t pointN = 0; pointN < nPoints; ++pointN, ++parNum )
    {
      et.second.setPar( pointN, newPar[parNum] );
    }//for
  }//foreach( custom event def )
  
}//setModelParameters


void NLSimple::setModelParameterErrors( std::vector<double> &newParErrorLow, 
                                        std::vector<double> &newParErrorHigh )
{
  assert( newParErrorLow.size() == NumNLSimplePars );
  assert( newParErrorHigh.size() == NumNLSimplePars );
  
  m_paramaterErrorPlus = newParErrorLow;
  m_paramaterErrorMinus = newParErrorHigh;
}//setModelParameterErrors




void NLSimple::findSteadyStateBeginings( double nHoursNoInsulin )
{
  using namespace gregorian;
  
  m_startSteadyStateTimes.clear();
  
   
  const ptime cgmsStartTime = m_cgmsData.getStartTime();
  const ptime insulinStartTime = m_freePlasmaInsulin.getStartTime();
  
  const time_duration noInsulinDur = getAbsoluteTime( 60.0 * nHoursNoInsulin ) 
                                     - getAbsoluteTime(0);
                                  
  const ptime cgmsEndTime =  m_cgmsData.getEndTime() - m_cgmsDelay;
  const ptime insulinEndTime = m_freePlasmaInsulin.getEndTime();
  
  const ptime endTime   = std::min(cgmsEndTime, insulinEndTime);
  const ptime startTime = std::max(cgmsStartTime, insulinStartTime);
  // const double endRelTime = getOffset(endTime);
  // const double startRelTime = getOffset(startTime);
 
  const time_duration dt(0, 1, 0, 0);
  
  double X_max = 0.0;
  double X = 0.0;
  vector<double> xValue;
  
  //If we haven't set model parameters yet, then will use nHoursNoInsulin 
  //  to find steady state.
  bool justUseInsulinInfo = ( m_paramaters.empty() || (m_paramaters[0] == kFailValue) );
  
  if( justUseInsulinInfo )
  {
    PosixTime lastInj = kGenericT0;
    bool inSteadyState = false;
    
    if( cgmsStartTime < (insulinStartTime - noInsulinDur) ) 
    {
      m_startSteadyStateTimes.push_back( startTime );
      inSteadyState = true;
    }//if( start time is a steady state )
  
    foreach( const GraphElement &el, m_freePlasmaInsulin )
    {
      const ptime &time = el.m_time;
      
      if( time < startTime || time > endTime ) continue;
      
      if( el.m_value > 0.0 )
      {
        if( inSteadyState ) lastInj = time;
        
        inSteadyState = false;
      }else
      {
        if( !inSteadyState && ((time - lastInj) > noInsulinDur) )
        {
          m_startSteadyStateTimes.push_back( time );
          cout << "Found Ins. dep. only start of steady state time at " << time << endl;
          inSteadyState = true;
        }//
      }//if non-zero insulin value
    }//
    
    return;
  }//if( justUseInsulinInfo )
  else
  {
    cout << "There are paramaters" << endl;
    foreach( double d, m_paramaters ) cout << "  " << d;
    cout <<endl;
  }
  
  for( ptime time = startTime; time < endTime; time += dt )
  {
    PTimeForcingFunction derivFunc 
    = boost::bind( &NLSimple::dXdT_usingCgmsData, boost::cref(*this), _1, X );
    
    X = rungeKutta4( time, X, dt, derivFunc );
    
    X_max = std::max( X, X_max );
    xValue.push_back(X);
  }//for( loop over valid time ranges )
  
  
  //Let anything less than 0.1% of X max be essentially zero
  X_max /= 1000.0;
   
  bool isZeroX = false;
  if( cgmsStartTime < (insulinStartTime - hours(nHoursNoInsulin)) ) 
  {
    m_startSteadyStateTimes.push_back( startTime );
    isZeroX = true;
  }//if( start time is a steady state )
 
  
  for( size_t mins = 0; mins < xValue.size(); ++mins )
  {
    if( isZeroX )
    {
      if( xValue[mins] > X_max ) isZeroX = false;
    }else
    {
      if( xValue[mins] < X_max )
      {
        isZeroX = true;
        const ptime time = startTime + toTimeDuration( (double)mins);
        m_startSteadyStateTimes.push_back( time );
        cout << "Found model dependant start of steady state at " << time << endl;
      }//if( has become zero )
    }//if( isZeroX ) / else
  }//for( loop over X's we have found )
  
}//findSteadyStateBeginings


ptime NLSimple::findSteadyStateStartTime( ptime t_start, ptime t_end )
{
  if( m_startSteadyStateTimes.empty() ) findSteadyStateBeginings();
  
  if( m_startSteadyStateTimes.empty() )
  {
    cout << "double NLSimple::findSteadyStateStartTime( t0, tEnd ):"
         << " I was unable to find any approproate steady state start times,"
         << endl;
    exit(1);
  }//if( m_startSteadyStateTimes.empty() )
  
  
  if( t_start == kGenericT0 && t_end == kGenericT0) return m_startSteadyStateTimes[0];

  
  foreach( const ptime &t, m_startSteadyStateTimes )
  {
    bool okayTime = true;
    
    if( t_start != kGenericT0 ) if( t < t_start ) okayTime = false;
    if( t_end   != kGenericT0 ) if( t > t_end ) okayTime = false;

    if( okayTime ) return t;
  }//foreach(...)
    
  cout << "double NLSimple::findSteadyStateTime( t0, tEnd ):"
       << " I was unable to find any approproate steady state start times,"
       << " between " << t_start << " and " << t_end << endl;
  exit(1);
  
  return kGenericT0;
}//findSteadyStateTime



//makeGlucosePredFromLastCgms(...) 
//  updates predX if it needs to cmgsEndTime-m_cgmsDelay
//  makes prediction starting at cmgsEndTime+m_dt, and ending at
//  cmgsEndTime+m_predictAhead.
//  The concentration graph you pass in will be cleared before use
//  *Note* for this funciton predictions are made m_predictAhead time ahead
//         of the latest cgms measurment, so really the predictions are
//         m_predictAhead + m_cgmsDelay ahead of last known BG
void NLSimple::makeGlucosePredFromLastCgms( ConsentrationGraph &predBg, 
                                            PosixTime simulateCgmsEndTime )
{
  predBg.clear();
  updateXUsingCgmsInfo(false);
  
  assert( simulateCgmsEndTime <= m_cgmsData.getEndTime() );
  const ptime cgmsEndTime = (simulateCgmsEndTime==kGenericT0)
                            ? m_cgmsData.getEndTime()
                            : simulateCgmsEndTime;
  assert( cgmsEndTime <= m_cgmsData.getEndTime() );
  assert( cgmsEndTime >= m_cgmsData.getStartTime() );
  assert( m_predictedInsulinX.getEndTime() >= (cgmsEndTime-m_cgmsDelay) );
  
  const ptime predStartTime = cgmsEndTime - m_cgmsDelay;
  const ptime predEndTime = cgmsEndTime + m_predictAhead - m_dt;
  
  DVec gAndX(2);
  if(  m_cgmsData.value(cgmsEndTime) < 10.0 )
  {
    cout << "makeGlucosePredFromLastCgms(...):m_cgmsData.value(" << cgmsEndTime 
         << ")=" << m_cgmsData.value(cgmsEndTime) << "; simulateCgmsEndTime="
         << simulateCgmsEndTime << " cgmsEndTime=" << cgmsEndTime 
         << " and m_cgmsData.getEndTime()=" << m_cgmsData.getEndTime() << endl;
    exit(-1);
  }//
  assert( m_cgmsData.value(cgmsEndTime) > 10.0 );
  gAndX[0] = m_cgmsData.value(cgmsEndTime) - m_basalGlucoseConcentration;
  gAndX[1] = m_predictedInsulinX.value(predStartTime) / 10.0;
  RK_PTimeDFunc func = bind( &NLSimple::dGdT_and_dXdT, boost::cref(*this), _1, _2 );
  
  //for some really bad paramater choices we migh get screwy values
  if( isnan(gAndX[1]) || isinf(gAndX[1]) || isinf(-gAndX[1]) )
  {
    cout << "makeGlucosePredFromLastCgms(): Warning found X value of " 
         << gAndX[1] << ", will not compute Blood Glucose Predictions" << endl;
    predBg.clear();
    return;
  }//something screwy
    
  if( m_dt > m_predictAhead )
  {
    cout << "The integration timestep of " << m_dt << " is larger than the"
         << " predict ahead time of " << m_predictAhead << endl;
    exit(1);
  }//if( m_dt > m_predictAhead )
  
  
  ptime time;
  for( time = predStartTime; time <= predEndTime; time += m_dt )
  {
    gAndX = rungeKutta4( time, gAndX, m_dt, func );
    const double actualGlucoseValue = gAndX[0]+m_basalGlucoseConcentration;
    predBg.insert( time+m_dt, actualGlucoseValue );
    // cout << "makeGlucosePredFromLastCgms(...): filled prediction attime t="
         // << time+m_dt << " with value " << actualGlucoseValue << endl;
  }//for( make predictions )
  
  if( time != (cgmsEndTime + m_predictAhead) )
  {
    assert( time < (cgmsEndTime + m_predictAhead) );
    TimeDuration dt = cgmsEndTime + m_predictAhead - time;
    cout << "makeGlucosePredFromLastCgms(...): Filling in one last point, using dt=" 
          << dt << endl;
    gAndX = rungeKutta4( time, gAndX, dt, func );
    const double actualGlucoseValue = gAndX[0]+m_basalGlucoseConcentration;
    predBg.insert( time+m_dt, actualGlucoseValue );
    assert( (time+dt) == (cgmsEndTime + m_predictAhead) );
    
    if( predBg.getEndTime() != (cgmsEndTime + m_predictAhead) )
    {
      cout << "predBg.getEndTime() != (cgmsEndTime + m_predictAhead)" << endl
           << predBg.getEndTime() << " != (" << cgmsEndTime << " + "
           << m_predictAhead << ")" << endl
           << predBg.getEndTime() << " != " << cgmsEndTime + m_predictAhead 
           << endl
           << "Just inserted " << actualGlucoseValue << " at " 
           << time << " + " << m_dt << " = " << time+m_dt << endl;
      exit(1);
    }//
    
    assert( predBg.getEndTime() == (cgmsEndTime + m_predictAhead) );
  }//if( need one more step )
  
  return;
}//makeGlucosePredFromLastCgms(...)



//getGraphOfMaxTimePredictions(...)
//  calls makeGlucosePredFromLastCgms(...) to make a graph showing what 
//  the predictions are/were for m_predictAhead of cgms readings,
//  The concentration graph you pass in will be
double NLSimple::getGraphOfMaxTimePredictions( ConsentrationGraph &predBg,
                                               PosixTime firstCgmsTime,
                                               PosixTime lastCgmsTime,
                                               double lastPointChi2Weight )
{
  // predBg.clear();
  updateXUsingCgmsInfo(false);
  double chi2 = 0.0;
  const ptime knownBgEndTime = m_cgmsData.getEndTime() - m_cgmsDelay;
  const bool calcChi2 = (lastPointChi2Weight>=0.0 && lastPointChi2Weight<=1.0);
  
  if( firstCgmsTime == kGenericT0 )
  {
    if( m_predictedInsulinX.empty() ) firstCgmsTime = m_cgmsData.getStartTime();
    else                    firstCgmsTime = m_predictedInsulinX.getStartTime();
    
    // cout << "getGraphOfMaxTimePredictions(...): First value available for X"
         // << " is at time " << firstCgmsTime << endl;
  }//if( firstCgmsTime == kGenericT0 )
  
  if( lastCgmsTime == kGenericT0 ) lastCgmsTime = m_cgmsData.getEndTime();
  
  if( m_cgmsData.getEndTime() < lastCgmsTime )
  {
    cout << "NLSimple::getGraphOfMaxTimePredictions(...) you want me to use"
         << " cgms time through " << lastCgmsTime << " but I only have cgms time"
         << " through " << m_cgmsData.getEndTime() << endl;
    exit(-1);
  }//if( m_cgmsData.getEndTime() < lastCgmsTime )
  
  if( firstCgmsTime < m_cgmsData.getStartTime() )
  {
    cout << "NLSimple::getGraphOfMaxTimePredictions(...) you want me to use"
         << " cgms time from " << firstCgmsTime << " but I only have cgms from "
         << m_cgmsData.getStartTime() << endl;
    exit(-1);
  }//if( m_cgmsData.getEndTime() < lastCgmsTime )
  
  assert( firstCgmsTime <= lastCgmsTime );
  
  if( !predBg.empty() ) cout << "Warning, untested trimming" << endl;
  predBg.trim( kGenericT0, firstCgmsTime-TimeDuration(0,0,0,1) );
  
  ConstGraphIter startIter = m_cgmsData.lower_bound(firstCgmsTime);
  ConstGraphIter endIter = m_cgmsData.upper_bound(lastCgmsTime);
  
  for( ConstGraphIter cgmsIter = startIter; cgmsIter != endIter; ++cgmsIter )
  {
    const ptime cgmsTime = cgmsIter->m_time;
    // cout << "On time cgmsTime=" << cgmsTime << endl;
    
    ConsentrationGraph currPred( m_t0, toNMinutes(m_dt), GlucoseConsentrationGraph );
    
    makeGlucosePredFromLastCgms( currPred, cgmsTime );
    
    if( currPred.empty() )
    {
      cout << "getGraphOfMaxTimePredictions(...): recieved a blank answer."
           << " I refuse to continue, returning blank result.  You" 
           << " should have just recieved a warning about X" << endl;
      predBg.clear();
      return DBL_MAX;
    }//if( currPred.empty() )
    
    const ptime predEndTime = currPred.getEndTime();
    if( predEndTime != (cgmsTime+m_predictAhead) )
    {
      cout << "Problem, currPred.getEndTime()=" << predEndTime
           << " while (cgmsTime+m_predictAhead)=" << (cgmsTime+m_predictAhead)
           << endl;
      exit(-1); //assert(0) takes forever on my mac
    }
    // cout << "getGraphOfMaxTimePredictions(..): Filling in point "
         // << currPred.value(predEndTime) << " for time " << predEndTime << endl;

    predBg.insert( predEndTime, currPred.value(predEndTime) );
    
    if( calcChi2 )
    {
      const double fracDerivChi2 = 0.0;
      const ptime t0 = currPred.getStartTime();
      const ptime t1 = currPred.getEndTime();  //note the very last time could be double-counted
      if( t1 <= knownBgEndTime )
      {
        chi2 += getChi2ComparedToCgmsData( currPred, fracDerivChi2, false, t0, t1 );
      }//if( we can get the chi2 )
    }//if( calcChi2 )
  }//for( loop over cgms points )
  
  assert( predBg.getEndTime() <= (lastCgmsTime+m_predictAhead) );
  
  if( predBg.getEndTime() < (lastCgmsTime+m_predictAhead) )
  {
    // cout << "getGraphOfMaxTimePredictions(..): Filling in one last point," 
         // << " from " << predBg.getEndTime() << " to " 
         // << lastCgmsTime+m_predictAhead << endl;
    
    ConsentrationGraph currPred( m_t0, toNMinutes(m_dt), GlucoseConsentrationGraph );
    makeGlucosePredFromLastCgms( currPred, lastCgmsTime );
    const ptime predEndTime = currPred.getEndTime();
    predBg.insert( predEndTime, currPred.value(predEndTime) );
    assert( predEndTime == (lastCgmsTime+m_predictAhead) );
    //we can't calc chi2, so we won't
  }//if( fill in one last point )
  
  if( calcChi2 )
  {
    // cout << "chi2 of indiv pred is " << chi2 << endl;
    
    chi2 *= (1.0 - lastPointChi2Weight);
    
    if( lastPointChi2Weight != 0.0 )
    {
      const ptime t0 = predBg.getStartTime();
      const ptime t1 = min( knownBgEndTime, predBg.getEndTime() );
      double c = getChi2ComparedToCgmsData( predBg, 0.0, false, t0, t1 );
      // cout << "chi2 of end-points of pred is " << c << endl;
      chi2 += (lastPointChi2Weight * c );
    }//
    // cout << "altogether the chi2 is " << chi2 << endl;
  }//if( calcChi2 )
  
  return chi2;
}//getGraphOfMaxTimePredictions(...)






ConsentrationGraph NLSimple::glucPredUsingCgms( int nMinutesPredict,  //nMinutes ahead of cgms
                                                ptime t_start, ptime t_end )
{
  cout << "Warning, I don't think NLSimple::glucPredUsingCgms(...) actually"
       << " works corectly, use at your own risk" << endl;
  using namespace boost::posix_time;
  const ptime startTime = findSteadyStateStartTime( t_start, t_end );
  const ptime endTime = (t_end != kGenericT0) 
                        ? t_end : m_cgmsData.getEndTime() - m_cgmsDelay;
  
  // const double dt = toNMinutes(m_dt);
  const time_duration durationPredAhead = (nMinutesPredict>0) 
                                          ? time_duration(0,nMinutesPredict, 0, 0) 
                                          : m_predictAhead;
                                    
  ptime startPredGraph = startTime + durationPredAhead + m_cgmsDelay;
  // ConsentrationGraph predBgGraph = ( startPredGraph, dt, GlucoseConsentrationGraph );
  ConsentrationGraph predBgGraph = m_predictedBloodGlucose;
  predBgGraph.clear();
  
  updateXUsingCgmsInfo();
  if( m_predictedInsulinX.empty() )
  {
    cout << "glucPredUsingCgms(...): warning, could not compute X, will not make"
         << " glucose prediction" << endl;
    return predBgGraph;
  }//
  
  assert( !m_predictedInsulinX.empty() );
  const ptime lastXTime = (--m_predictedInsulinX.end())->m_time;
  
  const ptime xStartTime = m_predictedInsulinX.begin()->m_time;
  if(  xStartTime > startTime ) cout << startTime << " <= " << xStartTime << endl;
  assert( xStartTime <= startTime );
  
  // cout << "EndtIMte=" << endTime << " xStart=" << xStartTime << " xEnd=" << lastXTime
       // << " startTime=" << startTime << endl;
  
  TimeDuration cgmsDt(0,-5,0,0);//a negative value so we know we haven't fond
  PosixTime previousCgmsTime = kGenericT0;
  vector<double> postKnownXAndG(2, kFailValue);
  RK_PTimeDFunc derivFunc = getRKDerivFunc();
  
  //Lets only make predictions starting from cgms values
  const ConstGraphIter lb = m_cgmsData.lower_bound(startTime + m_cgmsDelay);
  const ConstGraphIter ub = m_cgmsData.upper_bound(endTime + m_cgmsDelay);
  
  bool cgmsCoverSim = (( ub != m_cgmsData.end() ) 
                           && ( ub->m_time > (endTime + m_cgmsDelay)) );
  
  
  ptime time = startTime + m_cgmsDelay;  
  for( ConstGraphIter cgmsIter = lb; time < endTime; )
  {
    if(cgmsIter != ub || cgmsCoverSim ) 
    {
      previousCgmsTime = time;
      time = cgmsIter->m_time - m_cgmsDelay;
      if( cgmsIter == ub ) time = endTime;
    }//if(cgmsIter != ub)
    
    
    if( ((cgmsIter == ub) || (time > endTime)) && !cgmsCoverSim )  
    {
      assert( cgmsIter != m_cgmsData.begin() );
      
      if( (time >= endTime) && cgmsDt.is_negative() )
      {
        assert( time>endTime );
        assert( cgmsDt.is_negative() );
        
        cgmsDt = endTime - previousCgmsTime;
        cout << "glucPredUsingCgms(...): filing in one last X point using dt="
             << cgmsDt << " previous CGMNS time=" << previousCgmsTime
             << " this cgmsTime is " << time
             << " and endTime=" << endTime << endl;
        time = previousCgmsTime;  //the 'time += cgmsDt;' statment 
                                  //  will update time to endTime
      }//if( time > endTime )
      
      //getMostCommonPosixDt() is an expensive call, so only do it once and
      //  only if we have to
      if( cgmsDt.is_negative() ) 
      {
        cgmsDt = m_cgmsData.getMostCommonDt();
        
        cout << "glucPredUsingCgms(...): starting to simulate cgms data from  "
             << time << " to " << endTime << ", will use dt=" << cgmsDt << endl;
        
        assert( !cgmsDt.is_negative() );
        // assert( previousCgmsTime == kGenericT0 );
      }//if( cgmsDt is uninitialezed yet )
      
      time += cgmsDt;
    }//if( (cgmsIter == ub) OR (time > endTime))
        
    const ptime predEndTime = time + durationPredAhead - m_cgmsDelay;
    
    vector<double> predBgAndX(2);
    predBgAndX[0] = m_cgmsData.value( previousCgmsTime ) - m_basalGlucoseConcentration;
    predBgAndX[1] = m_predictedInsulinX.value( previousCgmsTime - m_cgmsDelay );
    
    assert( (previousCgmsTime - m_cgmsDelay) <= lastXTime  ); 
    assert( predBgAndX[0] > (1.0-m_basalGlucoseConcentration) );
    
    if( time > lastXTime )
    {
      if( postKnownXAndG[0] == kFailValue )
      {
        cout << "Warning, glucPredUsingCgms(...): is making a prediction beyond"
             << " last time I can compute X using cgms data. Doing this " 
             << " for " << lastXTime << " through " << endTime << endl;
        ConstGraphIter prevIter = cgmsIter;
        --prevIter;
        
        postKnownXAndG[0] = prevIter->m_value - m_basalGlucoseConcentration;
        postKnownXAndG[1] = m_predictedInsulinX.value( previousCgmsTime - m_cgmsDelay );
        cout << "Starting with " << time-m_dt << "--g=" << postKnownXAndG[0]
             << " and x="<< postKnownXAndG[1] << endl;        
      }//if( first time we have excedded lastXTime )
      
      // below is the same as calling 'getRKDerivFunc()' but I want to leave
      //  explicit since I might make dX and dG have some hysteresis in the future
      //  which would make the below invalid
      RK_PTimeDFunc func = bind( &NLSimple::dGdT_and_dXdT, boost::cref(*this), _1, _2 );
      
      for( ; previousCgmsTime < (time-m_dt); previousCgmsTime += m_dt )
      {
        postKnownXAndG = rungeKutta4( previousCgmsTime, postKnownXAndG, m_dt, func );
      }//for
      
      if( previousCgmsTime != time )
      {
        assert( previousCgmsTime < time );
        const time_duration lastDt = time - previousCgmsTime;
        //next cout is because this section of code is untested
        cout << "if( previousCgmsTime != time ) so using dt=" << lastDt 
             << " to update X and G to time " << time << endl;
        postKnownXAndG = rungeKutta4( previousCgmsTime, postKnownXAndG, lastDt, func );
      }//if( need one more step to get to time )
      
      predBgAndX = postKnownXAndG;
      
      cout << "      " << time << ": g=" << postKnownXAndG[0]
           << " x="<< postKnownXAndG[1] << endl;
    }//if( time > lastXTime )
    
    // bool isDubugTime = (time > time_from_string("2009-Apr-01 07:00:00") 
                        // && time < time_from_string("2009-Apr-01 11:00:00"));
    // if( isDubugTime )
      // cout << "At " << time << " starting with g=" 
           // << predBgAndX[0] + m_basalGlucoseConcentration 
           // << " and X=" << predBgAndX[1]
           // << " actualCgms=" << m_cgmsData.value(time);
    
    ptime predTime;
    for( predTime = time; predTime < predEndTime; predTime += m_dt )
    {
       predBgAndX = rungeKutta4( predTime, predBgAndX, m_dt, derivFunc );     
    }//for( loop over prediction time )
    
    predBgGraph.insert( predTime+durationPredAhead, predBgAndX[0] + m_basalGlucoseConcentration );
    
    // if( isDubugTime ) 
      // cout << " and inserted " << predBgAndX[0] + m_basalGlucoseConcentration
           // << " at time " << predTime << endl;
    
    if(cgmsIter != ub) ++cgmsIter;
  }//for( loop over time )
  
  
  return predBgGraph;
}//glucPredUsingCgms




void NLSimple::updateXUsingCgmsInfo( bool recomputeAll )
{ 
  if( m_paramaters.size() != NumNLSimplePars )
  {
    cout << "void NLSimple::updateXUsingCgmsInfo( bool recomputeAll ): You must"
         << " have " << NumNLSimplePars << " in m_paramaters. You have " 
         << m_paramaters.size() << endl;
    exit(1);
  }//if( m_paramaters.size() != NumNLSimplePars )
  
  if( recomputeAll ) m_predictedInsulinX.clear();
  
   const ptime endTime = m_cgmsData.getEndTime() - m_cgmsDelay;
   
  if( !m_predictedInsulinX.empty() )
  {
    if( m_predictedInsulinX.getEndTime() >= endTime ) return;
  }//
  
  const ptime steadyTime = findSteadyStateStartTime( kGenericT0, kGenericT0 );
  const ptime lastXTime = !m_predictedInsulinX.empty() ? 
                           (--m_predictedInsulinX.end())->m_time : 
                           kGenericT0;
  const ptime startTime = m_predictedInsulinX.empty() ? steadyTime : lastXTime;
 
  
  double prevX = kFailValue;  //purely as a double checky
  
  // if( startTime < endTime )
    // cout << "Updating X using CGMS from " << startTime << " to " << endTime << endl;
  
  //get the first point if currently empty
  if( m_predictedInsulinX.empty() ) m_predictedInsulinX.insert( startTime, 0.0 ); 
  
  for( ptime time = startTime; time <= endTime; time += m_dt )
  {
    double cgmsX = m_predictedInsulinX.value(time) / 10.0;
    
    //for some really bad paramater choices we migh get screwy values
    if( isnan(cgmsX) || isinf(cgmsX) || isinf(-cgmsX) )
    {
      cout << "updateXUsingCgmsInfo(): Warning found X value of " << cgmsX
           << ", will not compute X" << endl;
      m_predictedInsulinX.clear();
      return;
    }//something screwy
      
        
    //Just to checlk nothing is going wrong with Concentration graph
    if( prevX != kFailValue ) 
    {
      if( !isnan(cgmsX) && !isinf(cgmsX) && !isnan(prevX) && !isinf(prevX) 
          && fabs(cgmsX - prevX) > fabs(prevX/10000.0) ) 
      {
        cout << "updateXUsingCgmsInfo:: " << cgmsX << " != " << prevX << endl;
        assert( fabs(cgmsX - prevX) < fabs(prevX/10000.0) );
      }//
    }//if( prevX != kFailValue ) 
    
    
    PTimeForcingFunction derivFunc 
    = boost::bind( &NLSimple::dXdT_usingCgmsData, boost::cref(*this), _1, cgmsX );
    
    double newCgmsX = rungeKutta4( time, cgmsX, m_dt, derivFunc );
    prevX = newCgmsX;

    //the 10.0 below is to go from U/L to U/DL
    m_predictedInsulinX.insert( time + m_dt, 10.0 * newCgmsX );
    
    // if( time > time_from_string("2009-Apr-01 07:00:00") 
      // && time < time_from_string("2009-Apr-01 11:00:00") ) 
    // cout << "insert into X at time " << time+m_dt << " a value " << 10.0*newCgmsX << endl;
  }//for( loop over valid time ranges )
}//void updateXUsingCgmsInfo();




ConsentrationGraph NLSimple::performModelGlucosePrediction( boost::posix_time::ptime t_start,
                                                boost::posix_time::ptime t_end,
                                                double bloodGlucose_initial,
                                                double bloodX_initial )
{    
  cout << "Warning, performModelGlucosePrediction(....) needs some work, use"
       << " with caution" << endl;
  boost::posix_time::ptime endTime   = t_end;
  boost::posix_time::ptime startTime = findSteadyStateStartTime( t_start, t_end );
  
  //if x and cgms data start at t_start, then be are all good, else we nee
  
  ConsentrationGraph pred = m_predictedBloodGlucose;
  pred.clear();
  
  if( bloodGlucose_initial > 0.0 && t_start==kGenericT0 )
  {
    cout << "double NLSimple::performModelGlucosePrediction(...):"
         << " you can specify an initial blood glucose, but no time" << endl;
    exit(1);
  }//if( bloodGlucose_initial < 30 )
    
    
  if( bloodX_initial != kFailValue ) startTime = t_start;
  else bloodX_initial = 0.0;
    
    
  bloodGlucose_initial = m_cgmsData.value( startTime + m_cgmsDelay );
  if( bloodGlucose_initial < 1 )
  {
    cout << "double NLSimple::performModelGlucosePrediction(...):"
         << " I only have cgms data through " << m_cgmsData.getEndTime() 
         << " and the initial time is "  << startTime << endl;
    exit(1);
  }//if( bloodGlucose_initial < 30 )

    
  //if no end time will go till max of m_freePlasmaInsulin + 1 hour,
  //  or m_glucoseAbsorbtionRate + 1 hour
  if( endTime == kGenericT0 )
  {
    ptime insulinLastTime = m_freePlasmaInsulin.getEndTime() + hours(1);
    ptime glucoseLastTime = m_glucoseAbsorbtionRate.getEndTime() + hours(1);
    
    endTime = std::max( insulinLastTime, glucoseLastTime );
  }//if( endTime== kGenericT0 )
  
  if( !m_predictedInsulinX.empty() )
  {
    bool needsClearing = false;
    const ptime xStart = m_predictedInsulinX.getStartTime();
    const ptime xEnd = m_predictedInsulinX.getEndTime();
    
    if( xEnd > startTime && xEnd < endTime )     needsClearing = true;
    if( xStart > startTime && xStart < endTime ) needsClearing = true;
    if( xEnd > endTime && xStart < startTime )   needsClearing = true;
      
    if( needsClearing ) 
    {
      cout << "performModelGlucosePrediction(...):Warning, the current X "
           << " prediction runs from " << xStart
           << " to " << xEnd << " which interferes"
           << "with the time you want to predict (" << startTime << " to "
           << endTime << "), am resetting m_predictedInsulinX" << endl;
      m_predictedInsulinX.clear();
    }//if( needsClearing )
  }//if( !m_predictedInsulinX.empty() )
  
  if( !m_predictedBloodGlucose.empty() )
  {
    bool needsClearing = false;
    const ptime bgStart = m_predictedBloodGlucose.getStartTime();
    const ptime bgEnd = m_predictedBloodGlucose.getEndTime();
    
    if( bgEnd > startTime && bgEnd < endTime )     needsClearing = true;
    if( bgStart > startTime && bgStart < endTime ) needsClearing = true;
    if( bgEnd > endTime && bgStart < startTime )   needsClearing = true;
      
    if( needsClearing ) 
    {
      cout << "performModelGlucosePrediction(...):Warning, the current X "
           << " prediction runs from " << bgStart
           << " to " << bgEnd << " which interferes"
           << "with the time you want to predict (" << startTime << " to "
           << endTime << "), am results can not be added to"
           << " m_predictedBloodGlucose" << endl;
      // m_predictedBloodGlucose.clear();
    }//if( needsClearing )
  }//if( !m_predictedBloodGlucose.empty() )
  
  
  vector<double> gAndX(2);
  gAndX[0] = bloodGlucose_initial - m_basalGlucoseConcentration;
  gAndX[1] = bloodX_initial;
  RK_PTimeDFunc derivFunc = getRKDerivFunc();
  
  // cout << "About to start integrating between " << to_simple_string(startTime) 
       // << " and  " << to_simple_string(endTime) 
       // << ", With G_0=" << gAndX[0] << " and X_0=" << gAndX[1] << endl;
  
  for( ptime time = startTime; time <= endTime; time += m_dt )
  {     
     gAndX = rungeKutta4( time, gAndX, m_dt, derivFunc );
     
     pred.insert( time, gAndX[0] + m_basalGlucoseConcentration );
     
     //10.0 is to make up for unit differences, X should be mU/L, not mU/dL like
     //  the differential equations require
     m_predictedInsulinX.insert( time, 10.0 * gAndX[1] );
  }//for( loop over time )
  
  
  return pred; //getChi2ComparedToCgmsData( m_predictedBloodGlucose, startTime, endTime );
}//performModelGlucosePrediction



double NLSimple::getModelChi2( double fracDerivChi2,
                               boost::posix_time::ptime t_start,
                               boost::posix_time::ptime t_end )
{
  return getChi2ComparedToCgmsData( m_predictedBloodGlucose, fracDerivChi2, false, t_start, t_end );
}//getModelChi2



double NLSimple::getChi2ComparedToCgmsData( ConsentrationGraph &inputData,
                                            double fracDerivChi2,
                                            bool useAssymetricErrors,
                                            boost::posix_time::ptime t_start,
                                            boost::posix_time::ptime t_end) 
{
  double chi2 = 0.0;
  
  // const double origInputYOffset = inputData.getYOffset();
  // if( inputData.getYOffset() == 0.0 ) inputData.setYOffset(m_basalInsulinConc);
  
  if( fracDerivChi2 != 0.0 )
  {
    if( useAssymetricErrors ) 
    {
      cout << "You can-not use assymetric errors when using the derivative to calc chi2" << endl;
      exit(-1);
    }//if( useAssymetricErrors ) 
    
    ConsentrationGraph modelDerivData = inputData.getDerivativeGraph(60.0, BSplineSmoothing);
    ConsentrationGraph cgmsDerivData = m_cgmsData.getDerivativeGraph(60.0, BSplineSmoothing);
    
    double derivChi2 = getDerivativeChi2( modelDerivData, cgmsDerivData, t_start, t_end );
    
    chi2 += fracDerivChi2 * derivChi2;
  }//if( fracDerivChi2 != 0.0 )
  
  if( fracDerivChi2 != 1.0 )
  {
    double magChi2 = getBgValueChi2( inputData, m_cgmsData, useAssymetricErrors, t_start, t_end );
    chi2 += (1.0 - fracDerivChi2) * magChi2;
  }//if( fracDerivChi2 != 1.0 )
  
  // inputData.setYOffset(origInputYOffset);
  
  return chi2;
}//getChi2ComparedToCgmsData(...)


double NLSimple::getBgValueChi2( const ConsentrationGraph &modelData,
                                 const ConsentrationGraph &cgmsData,
                                 bool useAssymetricErrors,
                                 boost::posix_time::ptime t_start,
                                 boost::posix_time::ptime t_end ) const
{
  using namespace boost::gregorian;
  const double fracUncert = ModelDefaults::kCgmsIndivReadingUncert;
  
  const ptime effCgmsStart = cgmsData.getStartTime() - m_cgmsDelay;
  const ptime effCgmsEnd = cgmsData.getEndTime() - m_cgmsDelay;
  
  if( t_start == kGenericT0 ) 
    t_start = max( modelData.getStartTime(), effCgmsStart);
  
  if( t_end == kGenericT0 ) 
    t_end = min( modelData.getEndTime(), effCgmsEnd);
  
  
  if( t_start < effCgmsStart || t_end > effCgmsEnd )
  {
    cout << "NLSimple::getBgValueChi2(...): You spefied to start at "
         << t_start << " but cgms data starts at " << cgmsData.getStartTime()
         << ", you wnated end time " << t_end << " cgms data ends at "
         << cgmsData.getEndTime();
    exit(1);
  }//if( bad range of times )
  
  if( t_start < modelData.getStartTime() || t_end > modelData.getEndTime() )
  {
    cout << "NLSimple::getBgValueChi2(...): Warning, model data doesn't extend"
         << " for full time of " << t_start << " to " << t_end 
         << ", Model extend from " << modelData.getStartTime()
         << " to " << modelData.getEndTime()
         << " am assuming basal glucose consentrations for other times" << endl;
  }//if( model data doesnt' extend everywhere )
  
  // cout << "About to find magnitude based chi2 between " << t_start << " and "
       // << t_end << endl;
   
  int nPoints = 0;
  double chi2 = 0.0;
  
  const ptime modelEndTime = modelData.getEndTime();
  const ptime modelBeginTime = modelData.getStartTime();
  ConstGraphIter start = cgmsData.lower_bound( t_start + m_cgmsDelay );
  ConstGraphIter end = cgmsData.upper_bound( t_end + m_cgmsDelay );
  
  for( ConstGraphIter iter = start; iter != end; ++iter )
  {
    const ptime time = iter->m_time - m_cgmsDelay;
    const double cgmsValue  = iter->m_value;
    if( cgmsValue < 0.0 ) continue;
    if( time < modelBeginTime ) continue;
    if( time > modelEndTime ) break;
    double modelValue =  modelData.value(time);// //modelData.getYOffset()
    const double uncert = fracUncert*cgmsValue;
    
    // if( modelValue == 0.0 ) modelValue = m_basalGlucoseConcentration;
    // cout << time << ": modelValue=" << modelValue << ", cgmsValue=" <<cgmsValue 
          // << ", uncert=" << uncert << endl;
    ++nPoints;
    if( useAssymetricErrors ) 
    {
      const double d = modelValue - cgmsValue;
      if( d > 0.0 ) chi2 += pow( d / ModelDefaults::kBGHighSigma, 2 );
      else          chi2 += pow( d / ModelDefaults::kBGLowSigma, 2 );
    } else          chi2 += pow( (modelValue - cgmsValue) / uncert, 2 );
  }//for( loop over cgms data points )
  
  if( !nPoints && (t_start < t_end) )
  {
    cout << "getBgValueChi2(...): Warning asked to get chi2 for 0 points (" 
         << t_start << " to " << t_end << ")" << endl;
  }//if( !nPoints )
  
  //make sure we don't a divide by zero problem
  if( !nPoints ) return 0.0;
    
  // cout  << "Returning "  << chi2 / nPoints << endl;
  return chi2 / nPoints;
}//getBgValueChi2




//Below gives chi^2 based on the differences in derivitaves of graphs
double NLSimple::getDerivativeChi2( const ConsentrationGraph &modelDerivData,
                                    const ConsentrationGraph &cgmsDerivData,
                                    boost::posix_time::ptime t_start,
                                    boost::posix_time::ptime t_end ) const
{
  assert( modelDerivData.getGraphType() == cgmsDerivData.getGraphType() );
  using namespace boost::gregorian;

  const ptime effCgmsStart = cgmsDerivData.getStartTime() - m_cgmsDelay;
  const ptime effCgmsEnd = cgmsDerivData.getEndTime() - m_cgmsDelay;
  
  if( t_start == kGenericT0 ) 
    t_start = max( modelDerivData.getStartTime(), effCgmsStart);
  
  if( t_end == kGenericT0 ) 
    t_end = min( modelDerivData.getEndTime(), effCgmsEnd);

  
  if( modelDerivData.getGraphType() != BloodGlucoseConcenDeriv )
  {
    cout << "NLSimple::getDerivativeChi2(...): You passed in graphs of type "
         << modelDerivData.getGraphTypeStr() << " I need type"
         << "BloodGlucoseConcenDeriv" << endl;
    exit(1);
  }//if( wrong graph type )
    
  
  if( t_start < effCgmsStart || t_end > effCgmsEnd )
  {
    cout << "NLSimple::getDerivativeChi2(...): You spefied to start at "
         << t_start << " but cgms data starts at " << cgmsDerivData.getStartTime()
         << ", you wnated end time " << t_end << " cgms data ends at "
         << cgmsDerivData.getEndTime() << endl;
    exit(1);
  }//if( bad range of times )
  
  if( t_start < modelDerivData.getStartTime() || t_end > modelDerivData.getEndTime() )
  {
    cout << "NLSimple::getDerivativeChi2(...): Warning, model data doesn't extend"
         << " for full time of " << t_start << " to " << t_end 
         << ", Model extends from " << modelDerivData.getStartTime()
         << " to " << modelDerivData.getEndTime()
         << " am assuming basal glucose consentrations for other times" << endl;
  }//if( model data doesnt' extend everywhere )

  // cout << "About to find slope based chi2 between " << t_start << " and "
       // << t_end << endl;
       
  // const double fracUncert = ModelDefaults::kCgmsIndivReadingUncert;
  
  int nPoints = 0;
  double chi2 = 0.0;
  
  ConstGraphIter start = cgmsDerivData.lower_bound(t_start + m_cgmsDelay);
  ConstGraphIter end = cgmsDerivData.upper_bound(t_end + m_cgmsDelay);
  
  for( ConstGraphIter iter = start; iter != end; ++iter )
  {
    const ptime time = iter->m_time - m_cgmsDelay;
    const double cgmsValue  = iter->m_value;
    double modelValue = modelDerivData.value(time);
    // const double uncert = fracUncert*cgmsValue;
    if( modelValue == 0.0 ) modelValue = m_basalGlucoseConcentration;
    
    ++nPoints;
    chi2 += pow( (modelValue - cgmsValue), 2 );
  }//for( loop over cgms data points )
  
  return chi2 / nPoints;
}//getDerivativeChi2
    


double NLSimple::geneticallyOptimizeModel( double endPredChi2Weight,
                                           vector<TimeRange> timeRanges )
{
  using namespace TMVA;
  
  m_paramaters.clear();
  resetPredictions();
  findSteadyStateBeginings(3);
  
  Int_t fPopSize      = ModelDefaults::kGenPopSize;
  Int_t fNsteps       = ModelDefaults::kGenConvergNsteps;
  Int_t fSC_steps     = ModelDefaults::kGenNStepMutate;
  Int_t fSC_rate      = ModelDefaults::kGenNStepImprove;
  Double_t fSC_factor = ModelDefaults::kGenSigmaMult;
  Double_t fConvCrit  = ModelDefaults::kGenConvergCriteria;
  //When the number of improvments within the last fSC_steps
  // a) smaller than fSC_rate, then divide present sigma by fSC_factor
  // b) equal, do nothing
  // c) larger than fSC_rate, then multiply the present sigma by fSC_factor
  //
  //If convergence hasn't improvedmore than fConvCrit in the last
  //  fNsteps, then consider minimization complete
  
  ModelTestFCN fittnesFunc( this, endPredChi2Weight, timeRanges );
  fittnesFunc.SetErrorDef(10.0);
  
  size_t nParExp = (size_t)NumNLSimplePars;
  foreach( const EventDefPair &et, m_customEventDefs )
  {
    nParExp += et.second.getNPoints();
  }//foreach( custom event def )
  
  vector<TMVA::Interval*> ranges(nParExp);
  
  //Havw a switch statment to get a warning if we change NLSimplePars
  for( NLSimplePars par = NLSimplePars(0); par < NumNLSimplePars; par = NLSimplePars(par+1) )
  {
    switch(par)
    {
      case BGMultiplier:            ranges[par] = new Interval( -0.01, 0.1 );  break;
      case CarbAbsorbMultiplier:    ranges[par] = new Interval( 0.1, 10.0 );   break;
      case XMultiplier:             ranges[par] = new Interval( 0.0, 0.1 );    break;
      case PlasmaInsulinMultiplier: ranges[par] = new Interval( 0.0, 0.0015 ); break;
    };//
  }//for( loop over NLSimplePars )
    
  size_t parNum = (size_t)NumNLSimplePars;
  
  foreach( const EventDefPair &et, m_customEventDefs )
  {
    const size_t nPoints = et.second.getNPoints();
    for( size_t pointN = 0; pointN < nPoints; ++pointN, ++parNum )
    {
      cout << "Adding paramater " << pointN << " for Custom Event named " << et.second.getName()
           << " for genetic minization" << endl;
      ranges[parNum] = new Interval( -5.0, 5.0 );
    }//for
  }//foreach( custom event def )
  
  
  GeneticAlgorithm ga( fittnesFunc, fPopSize, ranges );
  
  
  int generation = 0;
  do{
    generation++;
    ga.Init();              // prepares the new generation and does evolution
    ga.CalculateFitness();  // assess the quality of the individuals
    
    //ga.GetGeneticPopulation().Print(0);
    std::cout << "---Generation " << generation << " yeilded---" << std::endl;
    vector<Double_t> currentPars = ga.GetGeneticPopulation().GetGenes(0)->GetFactors();
    cout << "Parameters: ";
    foreach( Double_t p, currentPars ) cout << p << "  ";
    cout << endl;
    double currFitness = fittnesFunc.testParamaters( currentPars, (m_gui != NULL) );
    cout << "Current fitness is " << currFitness << " fitness" << endl << endl << endl;
    
    if( m_gui ) 
    {
      m_gui->drawEquations();
      m_gui->drawModel();
      
      //The NLSimpleGui will delete the below pave text on it's next draw
      TPaveText *pt = new TPaveText(0.15, 0.8, 0.28, 0.89, "NDC");
      pt->SetBorderSize(0);
      pt->SetTextAlign(12);
      stringstream ss;
      ss << "#chi^{2}=" << currFitness;
      pt->AddText( ss.str().c_str() );
      pt->Draw();
      gPad->Update();
    }//if( m_gui )  
      
    fConvCrit = 0.008 * currFitness;
    
    // reduce the population size to the initially defined one
    ga.GetGeneticPopulation().TrimPopulation(); 
    
    //Here is where we could change the different factors
    ga.SpreadControl( fSC_steps, fSC_rate, fSC_factor );
  }while ( !ga.HasConverged( fNsteps, fConvCrit ) );
  // converged if: fitness-improvement < CONVCRIT within the last CONVSTEPS loops

  GeneticGenes* genes = ga.GetGeneticPopulation().GetGenes( 0 );
  std::vector<Double_t> gvec;
  gvec = genes->GetFactors();

  cout << "Final paramaters are: ";
  foreach( Double_t d, gvec ) cout << d << "  ";
  cout << endl;
  
  setModelParameters(gvec);
  if( m_gui ) 
  {
    m_gui->drawEquations();
    m_gui->drawModel();
    m_gui->refreshClarkAnalysis();
    m_gui->updateCustomEventTab();
  }//if( m_gui ) 
  
  double chi2 = fittnesFunc.testParamaters( gvec, true );
  
  
  return chi2;
}//geneticallyOptimizeModel

//returns true  if all information is updated to time
bool NLSimple::removeInfoAfter( const boost::posix_time::ptime &cgmsEndTime, 
                                bool removeCgms )
{
  //I think maybe 
  const ptime time = cgmsEndTime - m_cgmsDelay; ;  
  ConstGraphIter lastCgmsPoint = m_cgmsData.lower_bound( cgmsEndTime );
  ConstGraphIter lastXPoint    = m_predictedInsulinX.lower_bound( time );
  ConstGraphIter lastPBGPoint  = m_predictedBloodGlucose.lower_bound( time );
    
  unsigned int nUpdated = 0;
  if( removeCgms && lastCgmsPoint != m_cgmsData.end() )
  {
    ++nUpdated;
    const double val = m_cgmsData.value(time);
    m_cgmsData.erase( lastCgmsPoint, m_cgmsData.end() );
    m_cgmsData.insert( time, val );
  }//if( lastCgmsPoint != m_cgmsData.end() )
  
  if( lastXPoint != m_predictedInsulinX.end() )
  {
    ++nUpdated;
    const double val = m_predictedInsulinX.value(time);
    m_predictedInsulinX.erase( lastXPoint, m_predictedInsulinX.end() );
    m_predictedInsulinX.insert( time, val );
  }//if( lastXPoint != m_predictedInsulinX.end() )
  
  if( lastPBGPoint != m_predictedBloodGlucose.end() )
  {
    ++nUpdated;
    const double val = m_predictedBloodGlucose.value(time);
    m_predictedBloodGlucose.erase(lastPBGPoint, m_predictedBloodGlucose.end() );
    m_predictedBloodGlucose.insert( time, val );
  }//if( lastPBGPoint != m_predictedBloodGlucose.end() )
  
  
  if(removeCgms) return (nUpdated == 3);
  return (nUpdated == 2);
}//removeInfoAfter
    
    
    
double NLSimple::fitModelToDataViaMinuit2( double endPredChi2Weight,
                                           vector<TimeRange> timeRanges ) 
{  
  using namespace ROOT::Minuit2;
  
  vector<double> startingVal( NumNLSimplePars );
  startingVal[BGMultiplier]            = 0.0;
  startingVal[CarbAbsorbMultiplier]    = 30.0 / 9.0;
  startingVal[XMultiplier]             = 0.040;
  startingVal[PlasmaInsulinMultiplier] = 0.00015;
      
  //
  if( m_paramaters.size() && m_paramaters[0] != kFailValue ) 
  {
    assert( m_paramaters.size() == NumNLSimplePars );
    
    startingVal = m_paramaters;
    cout << "fitModelToDataViaMinuit2(...): using the existing model parameters"
         << " as starting values:";
    foreach( double d, m_paramaters ) cout << " " << d;
    cout << endl;
  }//if( use existing parameters )
  
  
  m_paramaters.clear();
  resetPredictions();
  findSteadyStateBeginings(3);
  
  ModelTestFCN modelFCN( this, endPredChi2Weight, timeRanges );
  modelFCN.SetErrorDef(10.0);
  
  MnUserParameters upar;
 
  //Have a switch statment to get a warning if we change NLSimplePars
  for( NLSimplePars par = NLSimplePars(0); par < NumNLSimplePars; par = NLSimplePars(par+1) )
  {
    switch(par)
    {
      case BGMultiplier:            
        upar.Add( "BGMult"   , startingVal[BGMultiplier], 0.1 );
        upar.SetLimits( "BGMult", -0.01, 0.1);
      break; 
      
      case CarbAbsorbMultiplier:
        upar.Add( "CarbMult" , startingVal[CarbAbsorbMultiplier], 0.1 );
        upar.SetLimits( "CarbMult", 0.1, 10);
      break;
  
      case XMultiplier:
        upar.Add( "XMult"    , startingVal[XMultiplier], 0.1 );
        upar.SetLimits( "XMult", 0.0, 0.1);
      break;
      
      case PlasmaInsulinMultiplier: 
        upar.Add( "InsulMult", startingVal[PlasmaInsulinMultiplier], 0.1 );
        upar.SetLimits( "InsulMult", 0.0, 0.001);
      break;
      assert(0);
    };//swtich(par)
  }//for( loop over NLSimplePars )
    
  size_t parNum = (size_t)NumNLSimplePars;
  
  foreach( const EventDefPair &et, m_customEventDefs )
  {
    const size_t nPoints = et.second.getNPoints();
    for( size_t pointN = 0; pointN < nPoints; ++pointN, ++parNum )
    {
      ostringstream name;
      name << et.second.getName() << "_" << pointN;
      
      cout << "Adding paramater named " << name.str() << " to minuit optimization" << endl;
      
      upar.Add( name.str(), et.second.getPar(pointN), 0.1 );
      upar.SetLimits( name.str(), -5.0, 5.0 );
    }//for
  }//foreach( custom event def )
  
  
  MnMigrad migrad(modelFCN, upar);
  
  cout<<"start migrad "<< endl;
  FunctionMinimum min = migrad();
  
  if(!min.IsValid()) 
  {
    //try with higher strategy
    std::cout<<"FM is invalid, try with strategy = 2."<<std::endl;
    MnMigrad migrad(modelFCN, upar, 2);
    min = migrad();
  }
  
  cout << "minimum: " << min << endl;
  // MinimumParameters paramaters = min.Parameters();
  
  m_paramaters.clear();
  
  for( NLSimplePars par = BGMultiplier; 
       par < NumNLSimplePars; 
       par = NLSimplePars( par + 1 ) )
  {
    m_paramaters.push_back( min.UserState().Value(par) );
    // cout << "Par " << par << " is " << min.UserState().Value(par) << endl;
  }
  
  m_predictedBloodGlucose.clear();
  
  double chi2 = 0.0;
  
  sort( timeRanges.begin(), timeRanges.end() );
  foreach( const TimeRange &tr, timeRanges )
  {
    ptime tStart = findSteadyStateStartTime( tr.first, tr.second );
    ConsentrationGraph predGluc(m_t0, 1.0, GlucoseConsentrationGraph);    
    
    if( !m_predictAhead.is_negative() )
    { 
      // predGluc = m_modelPtr->glucPredUsingCgms( -1, tStart, tr.second );
      chi2 += getGraphOfMaxTimePredictions( predGluc, tr.first, 
                                            tr.second, endPredChi2Weight );
    } else
    {
      predGluc = performModelGlucosePrediction( tStart, tr.second );
      // ptime chi2Start = ( predTime.is_negative() ) ?  tStart :  tStart + predTime + cgmsDelay;
      chi2 += getChi2ComparedToCgmsData( predGluc, endPredChi2Weight, false,
                                                     predGluc.getStartTime(), 
                                                     predGluc.getEndTime() );
    }//if( usePredictedChi2 ) else 

    if( m_predictedBloodGlucose.getEndTime() > predGluc.getStartTime() )
    {
      //m_cgmsDelay below is arbitrary
      m_predictedBloodGlucose.trim( kGenericT0, predGluc.getStartTime() - m_cgmsDelay );
    }//if( need to avoid colisions )
      
    m_predictedBloodGlucose = m_predictedBloodGlucose.getTotal(predGluc);
  }//foeach(...)  
  
  return chi2;
}//fitModelToData



bool NLSimple::fitEvents( TimeRangeVec timeRanges, 
                    std::vector< std::vector<double> *> &paramaterV, 
                    std::vector<ConsentrationGraph *> resultGV )
{
  
  FitNLSimpleEvent modelFcn( this );
  
  
  
}//bool NLSimple::fitEvents(...)






//okay what were going to do is assume every parameter has a 20% error
//  we will perform 100 Pseudo-Experiments foreach parameter, where the
//  paramater is varied within a gaussian
DVec NLSimple::chi2DofStudy( double endPredChi2Weight,
                                            TimeRangeVec timeRanges ) const
{
  cout  << "In chi2DofStudy(...) predicting ahead " << m_predictAhead << endl;
  NLSimple selfCopy = *this;
  selfCopy.resetPredictions();
  selfCopy.m_paramaters.clear();
  selfCopy.findSteadyStateBeginings();
  selfCopy.m_paramaters = m_paramaters;
  cout << "done finding steady states" << endl;
  
  DVec dof(NumNLSimplePars, 0.2);
  ModelTestFCN testFunc( &selfCopy, endPredChi2Weight, timeRanges ); 
  
  if( timeRanges.empty() ) 
    timeRanges.push_back( TimeRange(kGenericT0,kGenericT0) );
 
  int nCgmsPoints = 0;
  foreach( const TimeRange &tr, timeRanges )
  {
    const ptime startTime = selfCopy.findSteadyStateStartTime( tr.first, tr.second ) + m_cgmsDelay;
    const ptime endTime = (tr.second != kGenericT0) ? tr.second : m_cgmsData.getEndTime();
    
    ConstGraphIter lb = selfCopy.m_cgmsData.lower_bound( startTime );
    ConstGraphIter ub = selfCopy.m_cgmsData.upper_bound( endTime );
    
    for( ; lb != ub; ++lb ) ++nCgmsPoints;
    
    // cout << "About to do initial predicuion for " 
         // << startTime << " to " << endTime << endl;         
    ConsentrationGraph &cg = selfCopy.m_predictedBloodGlucose;
    cg = cg.getTotal( selfCopy.glucPredUsingCgms( -1, tr.first, tr.second ) );
  }//foreach( timeRange )
  
  //okay, now just to make things easy on us
  // ConsentrationGraph &cgmsGraph = selfCopy.m_cgmsData;
  // set<GraphElement> &predSet = selfCopy.m_predictedBloodGlucose;
  // 
  // cgmsGraph.clear();
  // //I think htis should be the case
  // assert( cgmsGraph.getT0() == selfCopy.m_predictedBloodGlucose.getT0() ); 
  // // 
  // cout << "About to copy prediction to cgms graph" << endl;
  // foreach( const GraphElement &ge, predSet )
  // {
    // ptime time = ge.m_time;
    // time += m_cgmsDelay;  
    // //instead of thiscould have called cgmsGraph.setT0_dontChangeOffsetValues(...)
    // 
    // cgmsGraph.insert( time, ge.m_value );
  // }//foreach( prediction set )
  
  // cout << "About to draw" << endl;
  // new TCanvas("CgmsIsNowPred", "CGMS Data(black) Replaced By Pred(Red)");
  // selfCopy.m_predictedBloodGlucose.draw( "", "", false, 2 );
  // cgmsGraph.draw( "l", "", true, 1 );
  
  selfCopy.m_predictedBloodGlucose.clear();
  
  const double nomChi2 = testFunc(m_paramaters);
  cout << "The nominal chi2/nCgmsPoints=" << nomChi2/nCgmsPoints << endl;
  
  // cout << "chi2DofStudy(...): There are " << nCgmsPoints 
       // << " cgms points used for fit" << endl;
  assert( m_paramaters.size() == NumNLSimplePars );
  
  TH1F *chi2Hists[NumNLSimplePars] = {NULL};
  
  TRandom3 rand;
  for( size_t parNum = 0; parNum<NumNLSimplePars; ++parNum)
  {
    DVec chi2s;
    DVec pars = m_paramaters;
    
    for( int i = 0; i < 300; ++i )
    {
      const double nomPar = m_paramaters[parNum];
      pars[parNum] = rand.Gaus( nomPar, 0.2*nomPar );
      
      double chi2 = testFunc(pars);
      chi2s.push_back( (chi2 - nomChi2)/nCgmsPoints );
    }//for( do the PE's )
    
    double min = *min_element(chi2s.begin(), chi2s.end());
    double max = *max_element(chi2s.begin(), chi2s.end());
    
    chi2Hists[parNum] = new TH1F( "parTh1", Form("chi2Hist par %i", parNum), 25, min, max);
    
    foreach( double d, chi2s ) chi2Hists[parNum]->Fill( d );
    
    new TCanvas(Form("Canvas_par_%i", parNum), Form("Canvas_par_%i", parNum));
    chi2Hists[parNum]->Draw();
  }//for( loop over paramaters )
  
  gTheApp->Run(kTRUE);
  
  return dof;
}//chi2DofStudy


void NLSimple::draw( bool pause,
                     boost::posix_time::ptime t_start,
                     boost::posix_time::ptime t_end  ) 
{
  assert( gTheApp );
  TGraph *cgmsBG  = m_cgmsData.getTGraph( t_start, t_end );
  TGraph *predBG  = m_predictedBloodGlucose.getTGraph( t_start, t_end );
  TGraph *glucAbs = m_glucoseAbsorbtionRate.getTGraph( t_start, t_end );
  TGraph *insConc = m_freePlasmaInsulin.getTGraph( t_start, t_end );
  TGraph *xPred   = m_predictedInsulinX.getTGraph( t_start, t_end );
  
  double maxHeight = std::max( cgmsBG->GetMaximum(), predBG->GetMaximum() );
  double minHeight = std::min( cgmsBG->GetMinimum(), predBG->GetMinimum() );
  
  bool usePredForAxis = true;
  
  if( predBG->GetN() < 4 )
  {
    usePredForAxis = false;
    maxHeight = cgmsBG->GetMaximum();
    minHeight = cgmsBG->GetMinimum();
  }//if( we don;thave a predictions )
  
  //When We use SetPoint(...) it erases the  histogram with nice labels
  TH1 *axisHisto = usePredForAxis ? (TH1 *)predBG->GetHistogram()->Clone()
                                  : (TH1 *)cgmsBG->GetHistogram()->Clone();
  
  const double glucMax = glucAbs->GetMaximum();
  const double glucMin = glucAbs->GetMinimum();
  
  const double graphRange = maxHeight - minHeight;
  const double glucScale = 0.5 * graphRange / (glucMax - glucMin);
  const double glucOffset = (minHeight - glucMin) - 0.2*graphRange;
  
  //Scale glucose absorbtion rate to be viewable
  for( int i=0; i<glucAbs->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    glucAbs->GetPoint( i, x, y );
    glucAbs->SetPoint( i, x, glucScale * (y-glucMin) + glucOffset);
  }//for( loop over glucAbs points )
  
  
  const double insulinMax = insConc->GetMaximum();
  const double insulinMin = insConc->GetMinimum();
  const double insulinScale = 0.25 * graphRange / (insulinMax - insulinMin);
  const double insulinOffset = (minHeight - insulinMin) - 0.2*graphRange;
  
  //Scale insulin concentration to be viewable
  for( int i=0; i<insConc->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    insConc->GetPoint( i, x, y );
    insConc->SetPoint( i, x, insulinScale * (y-insulinMin) + insulinOffset);
  }//for( loop over glucAbs points )
  
    
  const double xMax = xPred->GetMaximum();
  const double xMin = xPred->GetMinimum();
  const double xScale = 0.25 * graphRange / (xMax - xMin);
  const double xOffset = (minHeight - xMin) - 0.2*graphRange;
  
  for( int i=0; i<xPred->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    xPred->GetPoint( i, x, y );
    xPred->SetPoint( i, x, xScale * (y-xMin) + xOffset);
  }//for( loop over glucAbs points )
  
  
  //One problem is that calling cgmsBG->SetPoint(...) erases all the labels
  const double cgms_minutes_delay = toNMinutes(m_cgmsDelay);  
  for( int i=0; i<cgmsBG->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    cgmsBG->GetPoint( i, x, y );
    cgmsBG->SetPoint( i, x - cgms_minutes_delay, y);
  }//for( loop over glucAbs points )
  

  //Before setting the cgmsBG's histogram, we'll avoid memmorry leaks (maybe)
  TH1 *uslessHistPtr = cgmsBG->GetHistogram();
  cgmsBG->SetHistogram(NULL);
  delete uslessHistPtr;
  cgmsBG->SetHistogram(axisHisto);
  
  
  // minHeight -= 0.2 * abs(minHeight);
  minHeight -= 0.25 * graphRange; 
  maxHeight += 0.23 * abs(maxHeight);

  cgmsBG->SetMinimum( minHeight );
  predBG->SetMinimum( minHeight );
  
  cgmsBG->SetMaximum( maxHeight );
  predBG->SetMaximum( maxHeight );
  
  cgmsBG->SetLineColor(1);    //black
  predBG->SetLineColor(2);    //red
  glucAbs->SetLineColor(4);   //blue
  insConc->SetLineColor(28);  //brown
  xPred->SetLineColor(28);    //brown
  
  cgmsBG->SetLineWidth(2);
  predBG->SetLineWidth(2);
  glucAbs->SetLineWidth(2);
  insConc->SetLineWidth(1);
  xPred->SetLineWidth(1);
  
  insConc->SetLineStyle( 5 );
  
  predBG->SetTitle( "" );
  
  if( !gPad )  new TCanvas();
  
  cgmsBG->Draw( "Al" );
  if( usePredForAxis )  predBG->Draw( "l" );  
  if( glucAbs->GetN() ) glucAbs->Draw( "l" );
  if( xPred->GetN() )   xPred->Draw( "l" );
  if( insConc->GetN() ) insConc->Draw( "l" );
  
  // TAxis yaxis = cgmsBG->GetYaxis();
  
  
  // TGaxis *axis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(),
                            // gPad->GetUxmax(), gPad->GetUymax(), 
                            // glucNewMin, maxHeight,510,"+L");
  // axis->SetLineColor( 4 );
  // axis->SetLabelColor( 4 );
  // axis->SetTitleColor( 4 );
  // axis->SetTitle( "Glucose Absorption Rate (mg/dL/min)" );
  // axis->Draw();
  
  TLegend *leg = new TLegend( 0.65, 0.6, 0.95, 0.90);
  leg->SetBorderSize(0);
  leg->AddEntry( cgmsBG, "time-corrected CGMS data", "l" );
  if( usePredForAxis )  leg->AddEntry( predBG, "Predicted Blood-Glucose", "l" );
  if( xPred->GetN() )   leg->AddEntry( xPred, "Effective Insulin", "l" );
  if( insConc->GetN() ) leg->AddEntry( insConc, "Predicted Free-Plasma Insulin", "l" );
  if( glucAbs->GetN() ) leg->AddEntry( glucAbs, "Rate Of Glucose Absorbtion", "l" );
  leg->Draw();
  
  //If we don't draw a graph, we loose all references to the object
  //  so we must delete it now, or it becomes a memmorry leak
  if( !usePredForAxis )  { delete predBG;  predBG  = NULL; }  
  if( !glucAbs->GetN() ) { delete glucAbs; glucAbs = NULL; }
  if( !xPred->GetN() )   { delete xPred;   xPred   = NULL; }
  if( !insConc->GetN() ) { delete insConc; insConc = NULL; }
  
  if( pause )
  {
    gTheApp->Run(kTRUE);
    if( gPad )  { delete gPad; gPad = NULL; }
    if( cgmsBG )  delete cgmsBG;
    if( predBG )  delete predBG;
    if( glucAbs ) delete glucAbs;
    if( insConc ) delete insConc;
    if( leg )     delete leg;
    // delete axis;
  }//if( pause )
  
}//draw()

void NLSimple::runGui()
{
  assert( !m_gui );
  
  m_gui = new NLSimpleGui( this );
  gApplication->Run(kTRUE);
  
  delete m_gui;
  m_gui = NULL;
}//runGui


std::string NLSimple::convertToRootLatexString( double num, int nPrecision )
{
  if( fabs(num) >= 0.01 && fabs(num) < 999.9 ) 
  {
    ostringstream answer;
    answer << setiosflags(ios::fixed) << setprecision(nPrecision) << num;
    return answer.str();
  }//if( num >= 0.01 && num < 999.9 ) 
  
  int nExp = 0;
  if( fabs(num) < 0.01 )
  {
    while( fabs(num) < 1.0 )
    {
      --nExp;
      num *= 10.0;
    }//while(...)
  }else
  {
    while( fabs(num) > 10.0 )
    {
      ++nExp;
      num /= 10.0;
    }//while(...)
  }//if( num < 10 )

  // return Format( "%5.4dx10^{%i}", num, nExp );
  // return (format("%|+5|x10^{%d}") % num % nExp).str();
  ostringstream answer;
  answer << setiosflags(ios::fixed) << setprecision(nPrecision)
         << num << "x10^{" << nExp << "}";
  return answer.str();
}//std::string convertToRootLatexString( const double num )


std::vector<std::string> NLSimple::getEquationDescription() const
{
  vector<string> descrip(2);
  
  if( m_paramaters.size() != NumNLSimplePars ) return descrip;
  
  
  descrip[1] = string("#frac{dG}{dt} = -") 
               + convertToRootLatexString( m_paramaters[BGMultiplier], 4 )
               + " G - X(G - " + convertToRootLatexString( m_basalGlucoseConcentration, 0 )
               + ") + " + convertToRootLatexString( m_paramaters[CarbAbsorbMultiplier], 4 )
               + " Carb(t)";
  
  descrip[0] = string("#frac{dX}{dt} = -") 
               + convertToRootLatexString( m_paramaters[XMultiplier], 4 )
               + " X + " + convertToRootLatexString( m_paramaters[PlasmaInsulinMultiplier], 4 )
               + " I(t)";
      
  // descrip[1] = (format("#frac{dG}{dt} = -%|+6|d G - X(G - %d) + %d Carb(t)") 
                     // % m_paramaters[BGMultiplier] % m_basalGlucoseConcentration
                     // % m_paramaters[CarbAbsorbMultiplier]).str();
  // descrip[0] = (format("#frac{dX}{dt} = -%d X + %d I(t)") 
                     // % m_paramaters[XMultiplier] 
                     // % m_paramaters[PlasmaInsulinMultiplier]).str();
  return descrip;
}//getEquationDescription





template<class Archive>
void NLSimple::serialize( Archive &ar, const unsigned int version )
{
  unsigned int ver = version; //keep compiler from complaining
  ver = ver;
      
  ar & m_t0;
  ar & m_description;                  //Useful for later checking
  
  ar & m_cgmsDelay;                         //initially set to 15 minutes
  ar & m_basalInsulinConc;                  //units per kilo per hour
  ar & m_basalGlucoseConcentration;
  ar & m_customEventDefs;
  
  ar & m_dt;
  ar & m_predictAhead;
  
  ar & m_effectiveDof;         //So Minuit2 can properly interpret errors
  ar & m_paramaters;           //size == NumNLSimplePars
  ar & m_paramaterErrorPlus;
  ar & m_paramaterErrorMinus;
    
  ar & m_cgmsData;
  ar & m_freePlasmaInsulin;
  ar & m_glucoseAbsorbtionRate;
  ar & m_fingerMeterData;
  ar & m_customEvents;
  ar & m_mealData;
  
  ar & m_predictedInsulinX;
  ar & m_predictedBloodGlucose;
      
  ar & m_startSteadyStateTimes;
}//NLSimple::serialize
    



bool NLSimple::saveToFile( std::string filename )
{ 
  if( filename == "" )
  {
    filename = NLSimpleGui::getFileName( true );
    
    if( filename.empty() ) return false;
  }//if( filename == "" )
  
  
  unsigned int beginExtension = filename.find_last_of( "." );
  if( beginExtension == string::npos ) filename += ".dubm";
  else
  {
    string extention = filename.substr( beginExtension );
    if( extention != ".dubm" ) filename += ".dubm";
  }//
  
  std::ofstream ofs( filename.c_str() );
  
  if( !ofs.is_open() )
  {
    cout << "Couldn't open file " << filename << " for writing" << endl;
    exit(1);
  }//if( !ofs.is_open() )
  
  boost::archive::text_oarchive oa(ofs);
  
  oa << *this;
  
  return true;
}//bool ConsentrationGraph::saveToFile( std::string filename )




double NLSimple::getFitDof() const { return m_effectiveDof; }
void NLSimple::setFitDof( double dof ) { m_effectiveDof = dof; }

double ModelTestFCN::Up() const { return m_modelPtr->getFitDof(); }
void ModelTestFCN::SetErrorDef(double dof) {  m_modelPtr->setFitDof(dof); }


ModelTestFCN::ModelTestFCN( NLSimple *modelPtr, 
                            double endPredChi2Weight,
                            std::vector<TimeRange> timeRanges  ) 
  : m_modelPtr( modelPtr ), m_endPredChi2Weight(endPredChi2Weight), 
    m_timeRanges( timeRanges )
{
  assert( m_modelPtr );  
  
  if( m_timeRanges.empty() ) 
    m_timeRanges.push_back( TimeRange( kGenericT0, kGenericT0 ) );
}//ModelTestFCN constructor



Double_t ModelTestFCN::EstimatorFunction( std::vector<Double_t>& parameters )
{
  return this->operator()(parameters);
}//EstimatorFunction

double ModelTestFCN::testParamaters(const std::vector<double>& x, bool updateModel ) const
{
   //make sure Minuit isn't apssing in garbage
  foreach( double d, x )
  {
    if( isnan(d) || isinf(d) || isinf(-d) )
    {
      cout << "ModelTestFCN::operator(): Passed in garbage" << endl;
      return DBL_MAX;
    }//if( crap )
  }//foreach( parameter )
  
  const time_duration cgmsDelay = m_modelPtr->m_cgmsDelay;
  const time_duration predTime = m_modelPtr->m_predictAhead;
  m_modelPtr->setModelParameters( x ); //calls resetPredictions()
  
  double chi2 = 0.0;
  
  // foreach( double xi, x ) cout << " " << xi;
  // cout << endl;
  
  foreach( const TimeRange &tr, m_timeRanges )
  {
    ptime tStart = m_modelPtr->findSteadyStateStartTime( tr.first, tr.second );
    ConsentrationGraph predGluc( m_modelPtr->m_t0, 
                                 m_modelPtr->m_predictedBloodGlucose.getDt(), 
                                 GlucoseConsentrationGraph);    
    
    if( !predTime.is_negative() )
    { 
      // predGluc = m_modelPtr->glucPredUsingCgms( -1, tStart, tr.second );
      chi2 += m_modelPtr->getGraphOfMaxTimePredictions( predGluc, 
                                                        tr.first, tr.second, 
                                                        m_endPredChi2Weight );
    } else
    {
      predGluc = m_modelPtr->performModelGlucosePrediction( tStart, tr.second );
      // ptime chi2Start = ( predTime.is_negative() ) ?  tStart :  tStart + predTime + cgmsDelay;
      chi2 += m_modelPtr->getChi2ComparedToCgmsData( predGluc, m_endPredChi2Weight, 
                                                     false,
                                                   predGluc.getStartTime(), 
                                                   predGluc.getEndTime() );
    }//if( usePredictedChi2 ) else 
    
    
    if( predGluc.empty() )
    {
      cout << "ModelTestFCN: Warning, for paramaters ";
      foreach( double d, x ) cout << " " << d;
      cout << "I could not make a prediction" << endl;
      return DBL_MAX;
    }//
    
    if( updateModel ) m_modelPtr->m_predictedBloodGlucose = predGluc;
  }//foeach(...)
  
  if( !updateModel ) m_modelPtr->setModelParameters( std::vector<double>(0) );
  
  return chi2;
}//testParamaters

double ModelTestFCN::operator()(const std::vector<double>& x) const
{
  return testParamaters(x, false);
}//operator()












FitNLSimpleEvent::FitNLSimpleEvent( const NLSimple *model ) : m_model(model){};

   
unsigned int FitNLSimpleEvent::addEventToFitFor( ConsentrationGraph *fitResult,
                                                 PosixTime *startTime,
                                                 const TimeDuration &timeUncert,
                                                 vector<double> *pars )
{
  assert( pars );
  assert( fitResult );
  
  m_fitParamters.push_back( pars ); 
  m_startTimes.push_back( startTime );
  m_startTimeUncerts.push_back( timeUncert );
  m_fitForEvents.push_back( fitResult );
  
  switch( fitResult->getGraphType() )
  {
    case InsulinGraph:
    case BolusGraph:
      //When I implement a better insulin model, below will change.
      assert( pars->size() == 1 );
    break;
      
    case GlucoseAbsorbtionRateGraph:
    case GlucoseConsumptionGraph:
      assert( pars->size() == 1 || pars->size() == 5 );
    break;
  
    //CustomEvent
    
    case GlucoseConsentrationGraph:
    case BloodGlucoseConcenDeriv:
    case AlarmGraph:
    case NumGraphType:
      cout << "FitNLSimpleEvent::addEventToFitFor(...) only accepts inputs of"
           << " type InsulinGraph, BolusGraph, GlucoseAbsorbtionRateGraph,"
           << " or GlucoseConsumptionGraph, and not " 
           << fitResult->getGraphType() << endl;
      exit(-1);
    break;
  };//switch( fitResult->getGraphType() )
    
  return m_fitForEvents.size() - 1;
}//FitNLSimpleEvent::addEventToFitFor(...)



NLSimple FitNLSimpleEvent::getModelForNewFit() const
{
  NLSimple newModel( "", 1.0 );
  newModel = *m_model;
  
  if( m_fitForEvents.empty() )
  {
    cout << "You must call FitNLSimpleEvent::addEventToFitFor(...) before"
         << " calling FitNLSimpleEvent::getModelForNewFit()" << endl;
    exit(-1);
  }//if( m_fitForEvents.empty() )
  
  
  PosixTime minTime = *(m_startTimes[0]);  
  for( size_t i=0; i < m_startTimes.size(); ++i ) 
  {
    PosixTime thisStartTime = *(m_startTimes[i]);
    minTime = std::min( minTime, thisStartTime );
  }//for( loop over events i )
  
  const bool removeCgms = false;
  newModel.removeInfoAfter( minTime, removeCgms ); //removes X and Predicted
  
  updateModelWithCurrentGuesses( newModel );
  
  return newModel;
}//FitNLSimpleEvent::getModelForNewFit()
    

//function forMinuit2
//below parameters are ordered by m_fitParamters 
// (m_fitParamters[0][0], m_fitParamters[0][1]...), then by t0 of
// each m_fitForEvents where the double is number of 
// minutes after m_fitForEvents[i]->m_t0
double FitNLSimpleEvent::operator()(const std::vector<double>& x) const
{
  assert( m_fitParamters.size() );
  
  //For this function we have to:
  // 1) set new paramaters, including times
  //     so m_fitParamters and m_startTimes will be updated
  // 2) create new model
  // 3) make predictions that include the guessed event
  // 4) get chi2 of the time range --how to calc time range is still undecided
  // 5) return the chi2
  PosixTime minTime(pos_infin);
  PosixTime maxTime(neg_infin);
  unsigned int currPar = 0;
  unsigned int currParSet = 0;
  vector<double> *currParSetPtr = m_fitParamters[currParSet];
  TimeRangeVec effectedTimes;
  
  for( unsigned int par=0; par < x.size(); ++par )
  {
    if( currPar > currParSetPtr->size() )
    {
      currPar = 0;
      currParSetPtr = m_fitParamters[++currParSet];
    }//if( currPar == currParSet->size() )
    
    if( currPar != currParSetPtr->size() )
    {
      (*currParSetPtr)[currPar] = x[par];
    }else
    {
      ConsentrationGraph *thisGraph = m_fitForEvents[currParSet];
      
      const PosixTime thisTime = thisGraph->getT0() + toTimeDuration(x[par]);
      (*(m_startTimes[currParSet])) = thisTime;
      minTime = std::min( minTime, thisTime );
      maxTime = std::max( maxTime, thisTime  + hours(3) );
      effectedTimes.push_back( make_pair(thisTime, thisTime  + hours(3)) );
    }//if( a normal paramater ) / else ( the start time )
    
    ++currPar;
  }//for(...)
  
  assert( effectedTimes.size() == m_fitForEvents.size() );
  assert( !minTime.is_infinity() );
  
  NLSimple thisModel = getModelForNewFit();
  
  double chi2 = 0.0;
  
  for( unsigned int event = 0; event < m_fitForEvents.size(); ++event )
  {
    ConsentrationGraph pred = thisModel.m_predictedBloodGlucose;
    pred.clear();
    
    PosixTime startime = effectedTimes[event].first;
    PosixTime endtime = effectedTimes[event].second;
    chi2 += thisModel.getGraphOfMaxTimePredictions( pred, startime, endtime,
                                                   ModelDefaults::kLastPredictionWeight );
  }//for( loop over events to fit for )
  
  return chi2;
}//FitNLSimpleEvent::operator()


double FitNLSimpleEvent::Up() const
{
  return static_cast<double>( m_fitForEvents.size() );
}//FitNLSimpleEvent::Up()


void FitNLSimpleEvent::SetErrorDef(double dof)
{
  cout << "why are you calling void FitNLSimpleEvent::SetErrorDef( " 
       << dof << " ) ?" << endl;
}//FitNLSimpleEvent::SetErrorDef(double dof)
    
    //Function for TMVA fitters
Double_t FitNLSimpleEvent::EstimatorFunction( std::vector<Double_t>& parameters )
{
  return operator()(parameters);
}//FitNLSimpleEvent::EstimatorFunction( std::vector<double>& parameters )


//below calls updateFitForEvents() and then add these to the model passed in
void FitNLSimpleEvent::updateModelWithCurrentGuesses( NLSimple &model ) const
{
  
  for( size_t i=0; i < m_fitForEvents.size(); ++i )
  {
    const PosixTime startTime = *(m_startTimes[i]);
    ConsentrationGraph *graph = m_fitForEvents[i];
    const vector<double> &pars = *(m_fitParamters[i]);
    graph->clear();
    const GraphType graphType = graph->getGraphType();
    
    switch( graphType )
    {
      case InsulinGraph:
      case BolusGraph:
      {
        assert( pars.size() == 1 );
        const double dt = 1.0;
        ConsentrationGraph concG = novologConsentrationGraph( startTime, pars[0], dt );
        if( graphType == InsulinGraph ) *graph = concG;
        else if( graphType == BolusGraph ) graph->insert( startTime, pars[0] );
        else assert(0);
        
        model.addBolusData( concG );
        break;
      }//case InsulinGraph: case BolusGraph
      
      
      case GlucoseAbsorbtionRateGraph:
      case GlucoseConsumptionGraph:
      {
        const double dt = 1.0;
        ConsentrationGraph foodConsumed( startTime, dt, GlucoseConsumptionGraph );
        ConsentrationGraph foodAbsorbed( startTime, dt, GlucoseAbsorbtionRateGraph );
        foodConsumed.insert( startTime, pars[0] );
        
        if( pars.size() == 5)
        {
          foodAbsorbed = yatesGlucoseAbsorptionRate( 
                  startTime, pars[0], pars[1], pars[2], pars[3], pars[4], 1.0 );
        }else if( pars.size() == 1)
        {   
          foodAbsorbed = CgmsDataImport::carbConsumptionToSimpleCarbAbsorbtionGraph(foodConsumed);
        }else assert(0);
        
        if( graphType == GlucoseAbsorbtionRateGraph ) *graph = foodAbsorbed;
        else if( graphType == GlucoseConsumptionGraph ) *graph = foodConsumed;
        else assert(0);
        
        model.addGlucoseAbsorption( foodAbsorbed );
        break;
      };//case GlucoseAbsorbtionRateGraph: case GlucoseConsumptionGraph:
      
      // CustomEvent
      
      case GlucoseConsentrationGraph: case BloodGlucoseConcenDeriv: 
      case NumGraphType: case AlarmGraph:
        assert(0);
      break;
    };//switch( graph->getGraphType() )
  }//foreach( of the fit for events )
  
}//updateModelWithCurrentGuesses( NLSimple &model )










void drawClarkeErrorGrid( TVirtualPad *pad,
                          const ConsentrationGraph &cmgsGraph, 
                          const ConsentrationGraph &meterGraph,
                          const TimeDuration &cmgsDelay,
                          bool isCgmsVsMeter )
{
  if( !pad ) pad = new TCanvas();
  pad->cd();
  pad->Range(-45.17185,-46.4891,410.4746,410.6538);
  pad->SetFillColor(0);
  pad->SetBorderMode(0);
  pad->SetBorderSize(2);
  pad->SetRightMargin(0.02298851);
  pad->SetTopMargin(0.02330508);
  pad->SetFrameBorderMode(0);
  pad->SetFrameBorderMode(0);
   
  
  vector<TObject *> clarkesObj;
  clarkesObj = getClarkeErrorGridObjs( cmgsGraph, meterGraph, cmgsDelay, isCgmsVsMeter );
  
  //first 5 are TH2D's
  //then TLegend
  //then TLines
  //then TText
  clarkesObj[0]->Draw("SCAT");
  clarkesObj[1]->Draw("SCAT SAME");
  clarkesObj[2]->Draw("SCAT SAME");
  clarkesObj[3]->Draw("SCAT SAME");
  clarkesObj[4]->Draw("SCAT SAME");
  
  for( size_t i=5; i < clarkesObj.size(); ++i ) clarkesObj[i]->Draw();
  
  pad->Update();
}//void drawClarkeErrorGrid( TPad * )


std::vector<TObject *> 
getClarkeErrorGridObjs( const ConsentrationGraph &cmgsGraph, 
                        const ConsentrationGraph &meterGraph,
                        const TimeDuration &cmgsDelay,
                        bool isCgmsVsMeter )

{
  //Function addapted from (BSD license) http://www.mathworks.com/matlabcentral/fileexchange/20545
  std::vector<TObject *> returnObjects;
  
  string xTitle = ";Finger-Prick Value(mg/dl)";
  string yTitle = ";CGMS Value(mg/dl)";
  
  if( !isCgmsVsMeter )
  {
    xTitle = ";CGMS Value(mg/dl)";
    yTitle = ";Predicted Value(mg/dl)";
  }//if( !isCgmsVsMeter )
  
  
  TH2F *regionAH = new TH2F( "regionAH", (xTitle + yTitle).c_str(), 400, 0, 400, 400, 0, 400 );
  TH2F *regionBH = new TH2F( "regionBH", (xTitle + yTitle).c_str(), 400, 0, 400, 400, 0, 400 );
  TH2F *regionCH = new TH2F( "regionCH", (xTitle + yTitle).c_str(), 400, 0, 400, 400, 0, 400 );
  TH2F *regionDH = new TH2F( "regionDH", (xTitle + yTitle).c_str(), 400, 0, 400, 400, 0, 400 );
  TH2F *regionEH = new TH2F( "regionEH", (xTitle + yTitle).c_str(), 400, 0, 400, 400, 0, 400 );
  
  regionAH->SetStats( kFALSE );
  regionBH->SetStats( kFALSE );
  regionCH->SetStats( kFALSE );
  regionDH->SetStats( kFALSE );
  regionEH->SetStats( kFALSE );
  
  regionAH->SetMarkerStyle( 20 );
  regionBH->SetMarkerStyle( 21 );
  regionCH->SetMarkerStyle( 22 );
  regionDH->SetMarkerStyle( 23 );
  regionEH->SetMarkerStyle( 29 );
  
  regionAH->SetMarkerColor( 1  );  //black
  regionBH->SetMarkerColor( 30 );  //mint green
  regionCH->SetMarkerColor( 28 );  //brown
  regionDH->SetMarkerColor( 4  );  //blue
  regionEH->SetMarkerColor( 2  );  //red
  
  size_t numEntries = 0;
  
  foreach( const GraphElement &el, meterGraph )
  {
    const PosixTime &time = el.m_time;
    const double meterValue = el.m_value;
    const double cgmsValue = cmgsGraph.value(time + cmgsDelay);
    
    TH2F *regionH = NULL;
    
    if( (cgmsValue <= 70.0 && meterValue <= 70.0) || (cgmsValue <= 1.2*meterValue && cgmsValue >= 0.8*meterValue) )
    {
       regionH = regionAH;
    }else
    {
      if( ((meterValue >= 180.0) && (cgmsValue <= 70.0)) || ((meterValue <= 70.0) && cgmsValue >= 180.0 ) )
      {
        regionH = regionEH;
      }else
      {
        if( ((meterValue >= 70.0 && meterValue <= 290.0) && (cgmsValue >= meterValue + 110.0) ) || ((meterValue >= 130.0 && meterValue <= 180.0)&& (cgmsValue <= (7.0/5.0)*meterValue - 182.0)) )
        {
           regionH = regionCH;
        }else
        {
          if( ((meterValue >= 240.0) && ((cgmsValue >= 70.0) && (cgmsValue <= 180.0))) || (meterValue <= 175.0/3.0 && (cgmsValue <= 180.0) && (cgmsValue >= 70.0)) || ((meterValue >= 175.0/3.0 && meterValue <= 70.0) && (cgmsValue >= (6.0/5.0)*meterValue)) )
          {
            regionH = regionDH;
          }else
          {
             regionH = regionBH;
          }//if(zone D) else(zoneB)
        }//if(zone C) / else
      }//if(zone E) / else
   }//if(zone A) / else
   
   if( cgmsValue > 10.0 ) 
   {
     ++numEntries;
     regionH->Fill( meterValue, cgmsValue );
   }//if( cgmsValue > 10.0 )
  }//foreach( const GraphElement &el, meterGraph )
  
  if( numEntries > 1000.0 )
  {
    const double weight = 1000.0 / numEntries;
    
    regionAH->Scale( weight );
    regionBH->Scale( weight );
    regionCH->Scale( weight );
    regionDH->Scale( weight );
    regionEH->Scale( weight );
  }//if( numEntries > 1000.0 )
  
  const double nA = regionAH->Integral();
  const double nB = regionBH->Integral();
  const double nC = regionCH->Integral();
  const double nD = regionDH->Integral();
  const double nE = regionEH->Integral();
  double nTotal = nA + nB + nC + nD + nE;
  
  if( nTotal == 0.0 ) nTotal = 1000;
  ostringstream percentA, percentB, percentC, percentD, percentE;
  percentA << "Region A " << setw(4) << setiosflags(ios::fixed | ios::right) 
           << setprecision(1) << 100.0 * nA / nTotal << "%";
  percentB << "Region B " << setw(4) << setiosflags(ios::fixed | ios::right) 
           << setprecision(1) << 100.0 * nB / nTotal << "%";
  percentC << "Region C " << setw(4) << setiosflags(ios::fixed | ios::right) 
           << setprecision(1) << 100.0 * nC / nTotal << "%";
  percentD << "Region D " << setw(4) << setiosflags(ios::fixed | ios::right) 
           << setprecision(1) << 100.0 * nD / nTotal << "%";
  percentE << "Region E " << setw(4) << setiosflags(ios::fixed | ios::right) 
           << setprecision(1) << 100.0 * nE / nTotal << "%";
  
  TLegendEntry *entry;
  TLegend *leg = new TLegend( 0.09482759,0.6,0.3951149,0.91,NULL,"brNDC");
  leg->SetBorderSize(0);
  entry = leg->AddEntry( regionAH, percentA.str().c_str(), "p" );
  entry->SetTextColor( regionAH->GetMarkerColor() );
  entry = leg->AddEntry( regionBH, percentB.str().c_str(), "p" );
  entry->SetTextColor( regionBH->GetMarkerColor() );
  entry = leg->AddEntry( regionCH, percentC.str().c_str(), "p" );
  entry->SetTextColor( regionCH->GetMarkerColor() );
  entry = leg->AddEntry( regionDH, percentD.str().c_str(), "p" );
  entry->SetTextColor( regionDH->GetMarkerColor() );
  entry = leg->AddEntry( regionEH, percentE.str().c_str(), "p" );
  entry->SetTextColor( regionEH->GetMarkerColor() );
  
  
  //okay, now put everything in the return vewctor
  returnObjects.push_back( regionAH );
  returnObjects.push_back( regionBH );
  returnObjects.push_back( regionCH );
  returnObjects.push_back( regionDH );
  returnObjects.push_back( regionEH );
  returnObjects.push_back( leg );
  
  returnObjects.push_back( new TLine( 0.0, 0.0, 400.0, 400.0 ) );
  static_cast<TLine *>(returnObjects.back())->SetLineStyle(2); //dashed
  returnObjects.push_back( new TLine( 0.0,       70.0,      175.0/3.0, 70.0 ) );
  returnObjects.push_back( new TLine( 175.0/3.0, 70.0,      320,       400 ) );
  returnObjects.push_back( new TLine( 70,        84,        70,        400) );
  returnObjects.push_back( new TLine( 0,         180,       70,        180) );
  returnObjects.push_back( new TLine( 70,        180,       290,       400) );
  returnObjects.push_back( new TLine( 70,        0,         70,        175.0/3.0) );
  returnObjects.push_back( new TLine( 70,        175.0/3.0, 400,       320) );
  returnObjects.push_back( new TLine( 180,       0,         180,       70) );
  returnObjects.push_back( new TLine( 180,       70,        400,       70) );
  returnObjects.push_back( new TLine( 240,       70,        240,       180) );
  returnObjects.push_back( new TLine( 240,       180,       400,       180) );
  returnObjects.push_back( new TLine( 130,       0,         180,       70) );
  
  TText *text;
  text = new TText(30,  20,  "A");
  text->SetTextColor( regionAH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(30,  150, "D");
  text->SetTextColor( regionAH->GetMarkerColor() );
  text->SetTextSize(0.05);
  text->SetTextColor( regionDH->GetMarkerColor() );
  returnObjects.push_back( text );
  text = new TText(30,  380, "E");
  text->SetTextColor( regionEH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(150, 380, "C");
  text->SetTextColor( regionCH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(160, 20,  "C");
  text->SetTextColor( regionCH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(380, 20,  "E");
  text->SetTextColor( regionEH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(380, 120, "D");
  text->SetTextColor( regionDH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(380, 220, "B");
  text->SetTextColor( regionBH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );
  text = new TText(265, 350, "B");
  text->SetTextColor( regionBH->GetMarkerColor() );
  text->SetTextSize(0.05);
  returnObjects.push_back( text );

  return returnObjects;
}//getClarkeErrorGridObjs(...)


EventDef::EventDef(): 
  m_name(""), m_spline(NULL), m_times(NULL), m_values(NULL), 
  m_nPoints(0), m_duration(0,0,0,0), m_eventDefType(NumEventDefTypes)
{
}//EventDef::EventDef()



EventDef::EventDef( std::string name, TimeDuration eventLength, 
                    EventDefType defType, unsigned int nPoints) :
m_name(name), m_spline(NULL), m_times( nPoints ? new double[nPoints] : NULL ), 
m_values( nPoints ? new double[nPoints] : NULL ),
  m_nPoints( nPoints ), m_duration(eventLength), m_eventDefType(defType)
{
  const double nMinutes = toNMinutes(eventLength);
  const double dT = nMinutes / (nPoints - 1);
  
  if( nPoints ) m_values[0] = m_times[0]  = 0.0;
  
  for( unsigned int i = 1; i < nPoints; ++i )
  {
    m_times[i]  = m_times[i-1] + dT;
    m_values[i] = 0.0;
  }//for
}//EventDef::EventDef(...)


EventDef::EventDef( const EventDef &rhs ) : 
  m_spline(NULL), m_times(NULL), m_values(NULL), m_nPoints( 0 )
{
  *this = rhs;
}//EventDef::EventDef

EventDef::~EventDef()
{
  if( m_times )  delete [] m_times;
  if( m_values ) delete [] m_values;
  if( m_spline ) delete m_spline;
  
  m_spline = NULL;
  m_times = m_values = NULL;
}//EventDef::~EventDef()


const EventDef &EventDef::operator=( const EventDef &rhs )
{
  if( &rhs == this ) return *this;
  
  m_name = rhs.m_name;
  m_duration = rhs.m_duration;
  m_eventDefType = rhs.m_eventDefType;
  
  if( m_nPoints != rhs.m_nPoints )
  {
    m_nPoints = rhs.m_nPoints;
  
    if( m_times )  delete [] m_times;
    if( m_values ) delete [] m_values;
    
    m_times = m_values = NULL;
    
    if( m_nPoints )
    {
      m_times  = new double[m_nPoints];
      m_values = new double[m_nPoints];
    }//if( rhs.m_nPoints > 0 )
  }//if( m_nPoints != rhs.m_nPoints )
  
  for( size_t i=0; i<m_nPoints; ++i )
  {
    m_times[i]  = rhs.m_times[i];
    m_values[i] = rhs.m_values[i];
  }//for(
  
  if( m_spline )
  {
    delete m_spline;
    m_spline = NULL;
  }//if( m_spline )
  
  if( rhs.m_spline ) buildSpline();
  
  return *this;
}//EventDef::operator=


template<class Archive>
void EventDef::save(Archive & ar, const unsigned int version) const
{
  unsigned int ver = version; //keep compiler from complaining
  ver = ver;
  
  ar & m_name;
  ar & m_nPoints;
  ar & m_duration;
  ar & m_eventDefType;
  
  if( m_nPoints )
  {
    DVec timesV(m_times+0,  m_times+m_nPoints );
    DVec valueV(m_values+0, m_values+m_nPoints );
    ar & timesV;
    ar & valueV;
  }else
  {
    DVec emptyVec1(0), emptyVec2(0);
    ar & emptyVec1;
    ar & emptyVec2;
  }//if( m_nPoints ) / else
}//EventDef::save(...)


template<class Archive>
void EventDef::load(Archive & ar, const unsigned int version)
{
  unsigned int ver = version; //keep compiler from complaining
  ver = ver;
  
  ar & m_name;
  ar & m_nPoints;
  ar & m_duration;
  ar & m_eventDefType;
  
  if( m_times )  delete [] m_times;
  if( m_values ) delete [] m_values;
   
  m_times = m_values = NULL;
  
  if( m_nPoints )
  {
    m_times  = new double[m_nPoints];
    m_values = new double[m_nPoints];
  }//if( m_nPoints )
  
  DVec timesV, valueV;
  ar & timesV;
  ar & valueV;
  
  for( size_t i = 0; i < m_nPoints; ++i )
  {
    m_times[i] = timesV[i];
    m_values[i] = valueV[i];
  }//
}//EventDef::load(...)
    


unsigned int EventDef::getNPoints() const
{
  return m_nPoints;
}//int EventDef:getNPoints() const


void EventDef::setName( const std::string name )
{
  m_name = name;
}//void EventDef::setName( const std::string *name )


const std::string &EventDef::getName() const
{
  return m_name;
}//std::string getName() const


const EventDefType &EventDef::getEventDefType() const
{
  return m_eventDefType;
}//const EventDefType &EventDef::getEventDefType() const

const TimeDuration &EventDef::getDuration() const
{
  return m_duration;
}//TimeDuration EventDef::getDuration() const


double EventDef::getPar( unsigned int parNum ) const
{
  assert( parNum < m_nPoints );
  return m_values[parNum];
}//double EventDef::getPar( int parNum ) const


void EventDef::setPar( unsigned int point, double value )
{
  if( m_spline )
  {
    delete m_spline;
    m_spline = NULL;
  }//if( m_spline )
  
  assert( point < m_nPoints );
  m_values[point] = value;
}//void EventDef::setPar( int point, double value )

void EventDef::setPar( const std::vector<double> &par )
{
  for( unsigned int i = 0; i < par.size(); ++i )
    setPar( i, par[i] );
}//void EventDef::setPar( const std::vector<double> &par )


double EventDef::eval( const TimeDuration &dur ) const
{
  if( !m_nPoints ) return 0.0;
  if( dur > m_duration ) return 0.0;
  
  if( !m_spline ) buildSpline();
  
  return m_spline->Eval( toNMinutes(dur) );
}//double EventDef::eval( const TimeDuration &dur )


void EventDef::buildSpline() const
{
  if( !m_nPoints ) return;
  assert( !m_spline );
  
  //create new spline with zero derivatives at begining and end of functions
  m_spline = new TSpline3( m_name.c_str(), m_times, m_values, m_nPoints, "b1 e1", 0.0, 0.0 );
}//void EventDef::buildSpline() const


void EventDef::draw() const
{
  if( !m_spline ) buildSpline();
  if( !m_spline ) return;
  if( !gPad )     new TCanvas();
  
  string yTitle = "";
  string title = m_name;
  
  switch(m_eventDefType)
  {
    case IndependantEffect:    
      yTitle = "BG Change / Minute";
      title += " - Constant Effect"; 
      break;
    case MultiplyInsulin:
      yTitle = "Insulin Mult. Factor";      
      title += " - Effects Insulin Sens."; 
      break;
    case MultiplyCarbConsumed: 
      yTitle = "Carb. Abs. Mult. Factor";
      title += " - Effects Meal Sens."; 
      break;
    case NumEventDefTypes:     
      title += " - Undefined Effect"; 
      break;
  };//switch(m_eventDefType)
  
  // newSpline->SetTitle( title.c_str() );
  // TSpline3 *newSpline = (TSpline3 *)m_spline->Clone();
  
  TGraph *graph = new TGraph(m_nPoints);
  graph->SetTitle( title.c_str() );
  
  for( int i = 0; i < m_nPoints; ++i ) 
  {
    graph->SetPoint(i, m_times[i], m_spline->Eval( m_times[i] ) );
  }//for( loop over times )
  
  TH1 *hist = graph->GetHistogram();
  hist->GetXaxis()->SetTitle( "Minutes After Begining" );
  hist->GetYaxis()->SetTitle( yTitle.c_str() );
  graph->Draw( "a l c *" );
}//void draw() const



