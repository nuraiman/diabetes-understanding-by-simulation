//
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <string>
#include <stdio.h>
#include <math.h>  //contains M_PI
#include <stdlib.h>
#include <fstream>
#include <algorithm> //min/max_element
#include <float.h> // for DBL_MAX

#include "TH1F.h"
#include "TLegend.h"
#include "TGraph.h"
#include "TGAxis.h"
#include "TCanvas.h"
#include "TRandom3.h"
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
#include "boost/foreach.hpp"
#include "boost/bind.hpp"
#include "boost/ref.hpp"
#include "boost/assign/list_of.hpp" //for 'list_of()'
#include "boost/assign/list_inserter.hpp"
#include <boost/serialization/set.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>

#include "ResponseModel.hh"
#include "KineticModels.hh"
#include "CgmsDataImport.hh"
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
     m_dt( toTimeDuration(ModelDefaults::kIntegrationDt) ),
     m_predictAhead( toTimeDuration(ModelDefaults::kPredictAhead) ), 
     m_effectiveDof( 1.0 ),
     m_paramaters( rhs.m_paramaters ),
     m_paramaterErrorPlus( rhs.m_paramaterErrorPlus ), 
     m_paramaterErrorMinus( rhs.m_paramaterErrorMinus ),
     m_cgmsData( rhs.m_cgmsData ),
     m_freePlasmaInsulin( rhs.m_freePlasmaInsulin ),
     m_glucoseAbsorbtionRate( rhs.m_glucoseAbsorbtionRate ),
     m_predictedInsulinX( rhs.m_predictedInsulinX ),
     m_predictedBloodGlucose( rhs.m_predictedBloodGlucose ),
     m_startSteadyStateTimes( rhs.m_startSteadyStateTimes )
{
  m_currCgmsCorrFactor[0]      = rhs.m_currCgmsCorrFactor[0];
  m_currCgmsCorrFactor[1]      = rhs.m_currCgmsCorrFactor[1];
}//NLSimple( const NLSimple &rhs )
*/

NLSimple::NLSimple( const string &description, double basalUnitsPerKiloPerhour,
                    double basalGlucoseConcen, boost::posix_time::ptime t0 ) :
m_description(description), m_cgmsDelay( toTimeDuration(ModelDefaults::kDefaultCgmsDelay) ),
     m_basalInsulinConc( getBasalInsulinConcentration(basalUnitsPerKiloPerhour) ),
     m_basalGlucoseConcentration( basalGlucoseConcen ), 
     m_t0( t0 ),
     m_dt( toTimeDuration(ModelDefaults::kIntegrationDt) ),
     m_predictAhead( toTimeDuration(ModelDefaults::kPredictAhead) ),
     m_effectiveDof(1.0),
     m_paramaters(NumNLSimplePars, kFailValue),
     m_paramaterErrorPlus(0), m_paramaterErrorMinus(0),
     m_cgmsData(t0, 5.0, GlucoseConsentrationGraph),
     m_freePlasmaInsulin(t0, 5.0, InsulinGraph),
     m_glucoseAbsorbtionRate(t0, 5.0, GlucoseAbsorbtionRateGraph),
     m_predictedInsulinX(t0, 5.0, InsulinGraph),
     m_predictedBloodGlucose(t0, 5.0, GlucoseConsentrationGraph),
     m_startSteadyStateTimes(0)
{
  m_currCgmsCorrFactor[0] = m_currCgmsCorrFactor[1] = kFailValue;
}//NLSimple construnctor



NLSimple::NLSimple( std::string fileName ) :
     m_description(""), m_cgmsDelay( toTimeDuration(ModelDefaults::kDefaultCgmsDelay) ),
     m_basalInsulinConc( kFailValue ),
     m_basalGlucoseConcentration( kFailValue ), 
     m_t0( kGenericT0 ),
     m_dt( toTimeDuration(ModelDefaults::kIntegrationDt) ),
     m_predictAhead( toTimeDuration(ModelDefaults::kPredictAhead) ),
     m_effectiveDof(1.0),
     m_paramaters(NumNLSimplePars, kFailValue),
     m_paramaterErrorPlus(0), m_paramaterErrorMinus(0),
     m_cgmsData(kGenericT0, 5.0, GlucoseConsentrationGraph),
     m_freePlasmaInsulin(kGenericT0, 5.0, InsulinGraph),
     m_glucoseAbsorbtionRate(kGenericT0, 5.0, GlucoseAbsorbtionRateGraph),
     m_predictedInsulinX(kGenericT0, 5.0, InsulinGraph),
     m_predictedBloodGlucose(kGenericT0, 5.0, GlucoseConsentrationGraph),
     m_startSteadyStateTimes(0)
{
  
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
  
  m_currCgmsCorrFactor[0]      = rhs.m_currCgmsCorrFactor[0];
  m_currCgmsCorrFactor[1]      = rhs.m_currCgmsCorrFactor[1];
  
  m_cgmsData                   = rhs.m_cgmsData;
  m_freePlasmaInsulin          = rhs.m_freePlasmaInsulin;
  m_glucoseAbsorbtionRate      = rhs.m_glucoseAbsorbtionRate;
  
  m_predictedInsulinX          = rhs.m_predictedInsulinX;
  m_predictedBloodGlucose      = rhs.m_predictedBloodGlucose;
 
  m_startSteadyStateTimes      = rhs.m_startSteadyStateTimes;
    
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





//Functions for integrating the kinetic equations
double NLSimple::dGdT( const ptime &time, double G, double X ) const
{
  assert( m_paramaters.size() == NumNLSimplePars );
  
  const double dCarAbsorbDt = m_glucoseAbsorbtionRate.value( time );
  
  double dGdT = (-m_paramaters[BGMultiplier]*G) 
                - X*(G + m_basalGlucoseConcentration) 
                + m_paramaters[CarbAbsorbMultiplier] * dCarAbsorbDt;
                // + G_parameter[1]*I_basal;
    
  return dGdT;
}//dGdT




double NLSimple::dXdT( const ptime &time, double G, double X ) const
{
  G = G;//keep compiler from complaining
  assert( m_paramaters.size() == NumNLSimplePars );
    
  //The 10 is for convertin U/L, to U/dL
  const double insulin= (m_freePlasmaInsulin.value( time ) / 10.0);
  
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



double NLSimple::getBasalInsulinConcentration( double unitsPerKiloPerhour )
{
  ConsentrationGraph basalConcen( kGenericT0, 5.0, InsulinGraph );
  
  const double uPer5Min = unitsPerKiloPerhour / 12.0;
  const double nMinuteDoBasal = 15.0 * 60.0; //15 hours
  
  for( double time = 0.0; time <= nMinuteDoBasal; time += 5.0 )
  {
    basalConcen.add( uPer5Min, time, NovologAbsorbtion );
  }//for( loop over time making a basal rate )
  
  unsigned int nPoints = 0;
  double basalConc = 0.0;
  
  const double startSteadyState = nMinuteDoBasal-(5*60);
    
  for( double time = startSteadyState; time <= nMinuteDoBasal; time += 5.0 )
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



void NLSimple::addCgmsDataFromIsig( const ConsentrationGraph &isigData,
                                    const ConsentrationGraph &calibrationData,
                                    bool findNewSteadyState )
{
  assert(0);
}




void NLSimple::addConsumedGlucose( ptime time, double amount )
{
  ConsentrationGraph consumpRate( time, 5.0, GlucoseConsumptionGraph);
  consumpRate.insert( time, amount );
  addGlucoseAbsorption( consumpRate );
}//addConsumedGlucose



void NLSimple::addGlucoseAbsorption( const ConsentrationGraph &newData )
{
  if( newData.getGraphType() == GlucoseAbsorbtionRateGraph )
  {
    m_glucoseAbsorbtionRate = m_glucoseAbsorbtionRate.getTotal( newData );
  }else if( newData.getGraphType() == GlucoseConsumptionGraph )
  {
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


void NLSimple::resetPredictions()
{
  m_predictedInsulinX.clear();
  m_predictedBloodGlucose.clear();
}//resetPredictions()


void NLSimple::setModelParameters( const std::vector<double> &newPar )
{
  assert( newPar.size() == NumNLSimplePars || newPar.empty() );
  
  resetPredictions();
  m_paramaters = newPar;
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
  
   
  const ptime cgmsStartTime = m_cgmsData.getT0();
  const ptime insulinStartTime = m_freePlasmaInsulin.getT0();
  
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
    double lastInj = -9999.9;
    bool inSteadyState = false;
    
    if( cgmsStartTime < (insulinStartTime - noInsulinDur) ) 
    {
      m_startSteadyStateTimes.push_back( startTime );
      inSteadyState = true;
    }//if( start time is a steady state )
  
    foreach( const GraphElement &el, m_freePlasmaInsulin )
    {
      const ptime time = m_freePlasmaInsulin.getAbsoluteTime( el.m_minutes );
      
      if( time < startTime || time > endTime ) continue;
      
      if( el.m_value > 0.0 )
      {
        if( inSteadyState ) lastInj = el.m_minutes;
        
        inSteadyState = false;
      }else
      {
        if( !inSteadyState && ((el.m_minutes - lastInj) > 60.0*nHoursNoInsulin) )
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



ConsentrationGraph NLSimple::glucPredUsingCgms( int nMinutesPredict,  //nMinutes ahead of cgms
                                                ptime t_start, ptime t_end )
{
  using namespace boost::posix_time;
  const ptime startTime = findSteadyStateStartTime( t_start, t_end );
  const ptime endTime = (t_end != kGenericT0) 
                        ? t_end : m_cgmsData.getEndTime() - m_cgmsDelay;
  
  const double dt = toNMinutes(m_dt);
  const time_duration durationPredAhead = (nMinutesPredict>0) 
                                          ? time_duration(0,nMinutesPredict, 0, 0) 
                                          : m_predictAhead;
                                    
  ptime startPredGraph = startTime + durationPredAhead + m_cgmsDelay;
  ConsentrationGraph predBgGraph( startPredGraph, dt, GlucoseConsentrationGraph );
  
  updateXUsingCgmsInfo();
  if( m_predictedInsulinX.empty() )
  {
    cout << "glucPredUsingCgms(...): warning, could not compute X, will not make"
         << " glucose prediction" << endl;
    return predBgGraph;
  }//
  
  assert( !m_predictedInsulinX.empty() );
  const double lastXMinutes = (--m_predictedInsulinX.end())->m_minutes;
  const ptime lastXTime = m_predictedInsulinX.getAbsoluteTime( lastXMinutes );
  
  const ptime xStartTime = m_predictedInsulinX.getAbsoluteTime( m_predictedInsulinX.begin()->m_minutes);
  if(  xStartTime > startTime ) cout << startTime << " <= " << xStartTime << endl;
  assert( xStartTime <= startTime );
  
  TimeDuration cgmsDt(0,-5,0,0);//a negative value so we know we haven't fond
  PosixTime previousCgmsTime = kGenericT0;
  vector<double> postKnownXAndG(2, kFailValue);
  RK_PTimeDFunc derivFunc = getRKDerivFunc();
  
  //Lets only make predictions starting from cgms values
  const ConstGraphIter lb = m_cgmsData.lower_bound(startTime + m_cgmsDelay);
  const ConstGraphIter ub = m_cgmsData.upper_bound(endTime + m_cgmsDelay);
  
  
  ptime time = startTime + m_cgmsDelay;  
  for( ConstGraphIter cgmsIter = lb; time < endTime; )
  {
    if(cgmsIter != ub) 
    {
      previousCgmsTime = time;
      time = m_cgmsData.getAbsoluteTime( cgmsIter->m_minutes ) - m_cgmsDelay;
    }//if(cgmsIter != ub) 
    
    if( (cgmsIter == ub) || (time > endTime) )  
    {
      // assert( time >= lastXTime );
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
        cgmsDt = m_cgmsData.getMostCommonPosixDt();
        
        cout << "glucPredUsingCgms(...): starting to simulate cgms data from  "
             << time << " to " << endTime << ", will use dt=" << cgmsDt << endl;
        
        assert( !cgmsDt.is_negative() );
        // assert( previousCgmsTime == kGenericT0 );
      }//if( cgmsDt is uninitialezed yet )
      
      time += cgmsDt;
    }//if( (cgmsIter == ub) OR (time > endTime))
        
    const ptime predEndTime = time + durationPredAhead - m_cgmsDelay;
    
    vector<double> predBgAndX(2);
    predBgAndX[0] = m_cgmsData.value( time + m_cgmsDelay ) - m_basalGlucoseConcentration;
    predBgAndX[1] = m_predictedInsulinX.value( time );
    
    
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
  if( recomputeAll ) m_predictedInsulinX.clear();
  const ptime steadyTime = findSteadyStateStartTime( kGenericT0, kGenericT0 );
  const double lastXMinutes = !m_predictedInsulinX.empty() ? 
                              (--m_predictedInsulinX.end())->m_minutes : 
                              kFailValue;
  const ptime lastXTime = m_predictedInsulinX.getAbsoluteTime( lastXMinutes );
  const ptime startTime = m_predictedInsulinX.empty() ? steadyTime : lastXTime;
  const ptime endTime = m_cgmsData.getEndTime() - m_cgmsDelay;
  
  double prevX = kFailValue;  //purely as a double checky
  
  // if( startTime < endTime )
    // cout << "Updating X using CGMS from " << startTime << " to " << endTime << endl;
  m_predictedInsulinX.insert( startTime, 0.0 ); //get the first point
  
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




double NLSimple::performModelGlucosePrediction( boost::posix_time::ptime t_start,
                                                boost::posix_time::ptime t_end,
                                                double bloodGlucose_initial,
                                                double bloodX_initial )
{    
  cout << "Warning, performModelGlucosePrediction(....) needs some work, use"
       << " with caution" << endl;
  boost::posix_time::ptime endTime   = t_end;
  boost::posix_time::ptime startTime = findSteadyStateStartTime( t_start, t_end );
  
  //if x and cgms data start at t_start, then be are all good, else we nee
  
  
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
           << endTime << "), am resetting m_predictedBloodGlucose" << endl;
      m_predictedBloodGlucose.clear();
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
     
     m_predictedBloodGlucose.insert( time, gAndX[0] + m_basalGlucoseConcentration );
     
     //10.0 is to make up for unit differences, X should be mU/L, not mU/dL like
     //  the differential equations require
     m_predictedInsulinX.insert( time, 10.0 * gAndX[1] );
     
  }//for( loop over time )
  
  
  return 0.0; //getChi2ComparedToCgmsData( m_predictedBloodGlucose, startTime, endTime );
}//performModelGlucosePrediction



double NLSimple::getModelChi2( double fracDerivChi2,
                               boost::posix_time::ptime t_start,
                               boost::posix_time::ptime t_end )
{
  return getChi2ComparedToCgmsData( m_predictedBloodGlucose, fracDerivChi2, t_start, t_end );
}//getModelChi2



double NLSimple::getChi2ComparedToCgmsData( ConsentrationGraph &inputData,
                                            double fracDerivChi2,
                                            boost::posix_time::ptime t_start,
                                            boost::posix_time::ptime t_end) 
{
  double chi2 = 0.0;
  
  const double origInputYOffset = inputData.getYOffset();
  if( inputData.getYOffset() == 0.0 ) inputData.setYOffset(m_basalInsulinConc);
  
  if( fracDerivChi2 != 0.0 )
  {
    ConsentrationGraph modelDerivData = inputData.getDerivativeGraph(60.0, BSplineSmoothing);
    ConsentrationGraph cgmsDerivData = m_cgmsData.getDerivativeGraph(60.0, BSplineSmoothing);
    
    double derivChi2 = getDerivativeChi2( modelDerivData, cgmsDerivData, t_start, t_end );
    
    chi2 += fracDerivChi2 * derivChi2;
  }//if( fracDerivChi2 != 0.0 )
  
  if( fracDerivChi2 != 1.0 )
  {
    double magChi2 = getBgValueChi2( inputData, m_cgmsData, t_start, t_end );
    chi2 += (1.0 - fracDerivChi2) * magChi2;
  }//if( fracDerivChi2 != 1.0 )
  
  inputData.setYOffset(origInputYOffset);
  
  return chi2;
}//getChi2ComparedToCgmsData(...)


double NLSimple::getBgValueChi2( const ConsentrationGraph &modelData,
                                 const ConsentrationGraph &cgmsData,
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
         << cgmsData.getEndTime() << endl;
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
       
  double chi2 = 0.0;
  
  ConstGraphIter start = cgmsData.lower_bound( t_start + m_cgmsDelay );
  ConstGraphIter end = cgmsData.upper_bound( t_end + m_cgmsDelay );
  
  for( ConstGraphIter iter = start; iter != end; ++iter )
  {
    const ptime time = cgmsData.getAbsoluteTime( iter->m_minutes ) - m_cgmsDelay;
    const double cgmsValue  = iter->m_value;
    if( cgmsValue == 0.0 ) continue;
    double modelValue =  modelData.value(time); //+modelData.getYOffset()
    const double uncert = fracUncert*cgmsValue;
    if( modelValue == 0.0 ) modelValue = m_basalGlucoseConcentration;
    // cout << time << ": modelValue=" << modelValue << ", cgmsValue=" <<cgmsValue 
          // << ", uncert=" << uncert << endl;
    chi2 += pow( (modelValue - cgmsValue) / uncert, 2 );
  }//for( loop over cgms data points )
  
  
  return chi2;
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
       
  const double fracUncert = ModelDefaults::kCgmsIndivReadingUncert;
  
  double chi2 = 0.0;
  
  ConstGraphIter start = cgmsDerivData.lower_bound(t_start + m_cgmsDelay);
  ConstGraphIter end = cgmsDerivData.upper_bound(t_end + m_cgmsDelay);
  
  for( ConstGraphIter iter = start; iter != end; ++iter )
  {
    const ptime time = cgmsDerivData.getAbsoluteTime( iter->m_minutes ) - m_cgmsDelay;
    const double cgmsValue  = iter->m_value;
    double modelValue = modelDerivData.value(time);
    // const double uncert = fracUncert*cgmsValue;
    if( modelValue == 0.0 ) modelValue = m_basalGlucoseConcentration;
    
    chi2 += pow( (modelValue - cgmsValue), 2 );
  }//for( loop over cgms data points )
  
  return chi2;
}//getDerivativeChi2
    


double NLSimple::geneticallyOptimizeModel( double fracDerivChi2,
                                           vector<TimeRange> timeRanges )
{
  using namespace TMVA;
  
  m_paramaters.clear();
  resetPredictions();
  findSteadyStateBeginings(3);
  
  Int_t fPopSize      = 100;
  Int_t fNsteps       = 10;
  Int_t fSC_steps     = 6;
  Int_t fSC_rate      = 3;
  Double_t fSC_factor = 0.5;
  Double_t fConvCrit  = 5.0;
  //When the number of improvments within the last fSC_steps
  // a) smaller than fSC_rate, then divide present sigma by fSC_factor
  // b) equal, do nothing
  // c) larger than fSC_rate, then multiply the present sigma by fSC_factor
  //
  //If convergence hasn't improvedmore than fConvCrit in the last
  //  fNsteps, then consider minimization complete
  
  ModelTestFCN fittnesFunc( this, fracDerivChi2, timeRanges );
  fittnesFunc.SetErrorDef(10.0);
  
  vector<TMVA::Interval*> ranges;
  ranges.push_back( new Interval( -0.01, 0.1 )  );
  ranges.push_back( new Interval( 0.1, 10.0 )  );
  ranges.push_back( new Interval( 0.0, 0.1 )  );
  ranges.push_back( new Interval( 0.0, 0.0015 )  );
  
  
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
    double currFitness = fittnesFunc.EstimatorFunction( currentPars );
    cout << "Current fitness is " << currFitness << " fitness" << endl << endl << endl;
    
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
  
  double chi2 = fittnesFunc(gvec);
  
  setModelParameters(gvec);
  
  // double chi2 = 0.0;
  // foreach( const TimeRange &tr, timeRanges )
  // {
    // ptime tStart = findSteadyStateStartTime( tr.first, tr.second );
    // 
    // performModelGlucosePrediction( tStart, tr.second );
    // chi2 += getModelChi2( fracDerivChi2, tStart, tr.second );
  // }//foeach(...)  
  
  return chi2;
}//geneticallyOptimizeModel

//returns true  if all information is updated to time
bool NLSimple::removeInfoAfter( const boost::posix_time::ptime &cgmsEndTime )
{
  //I think maybe 
  const ptime time = cgmsEndTime - m_cgmsDelay; ;  
  ConstGraphIter lastCgmsPoint = m_cgmsData.lower_bound( cgmsEndTime );
  ConstGraphIter lastXPoint    = m_predictedInsulinX.lower_bound( time );
  ConstGraphIter lastPBGPoint  = m_predictedBloodGlucose.lower_bound( time );
    
  unsigned int nUpdated = 0;
  if( lastCgmsPoint != m_cgmsData.end() )
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
  
  return (nUpdated == 3);
}//removeInfoAfter
    
    
    
double NLSimple::fitModelToDataViaMinuit2( double fracDerivChi2,
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
  
  ModelTestFCN modelFCN( this, fracDerivChi2, timeRanges );
  modelFCN.SetErrorDef(10.0);
  
  MnUserParameters upar;
  upar.Add( "BGMult"   , startingVal[BGMultiplier], 0.1 );
  upar.SetLimits( "BGMult", -0.01, 0.1);
  
  upar.Add( "CarbMult" , startingVal[CarbAbsorbMultiplier], 0.1 );
  upar.SetLimits( "CarbMult", 0.1, 10);
  
  upar.Add( "XMult"    , startingVal[XMultiplier], 0.1 );
  upar.SetLimits( "XMult", 0.0, 0.1);
  
  upar.Add( "InsulMult", startingVal[PlasmaInsulinMultiplier], 0.1 );
  upar.SetLimits( "InsulMult", 0.0, 0.001);

  
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
  
  foreach( const TimeRange &tr, timeRanges )
  {
    ptime tStart = findSteadyStateStartTime( tr.first, tr.second );
    
    if( m_predictAhead.is_negative() )
    {
       performModelGlucosePrediction( tStart, tr.second );
       chi2 += getModelChi2( fracDerivChi2, tStart, tr.second );
    }else
    {
      ConsentrationGraph pred = glucPredUsingCgms( -1, tStart, tr.second );
      
      chi2 += getChi2ComparedToCgmsData( pred, fracDerivChi2, tStart, tr.second );
      m_predictedBloodGlucose = m_predictedBloodGlucose.getTotal(pred);
    }//if( doen't use cgms data for prediction ) / else
  }//foeach(...)  
  
  return chi2;
}//fitModelToData


//okay what were going to do is assume every parameter has a 20% error
//  we will perform 100 Pseudo-Experiments foreach parameter, where the
//  paramater is varied within a gaussian
DVec NLSimple::chi2DofStudy( double fracDerivChi2,
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
  ModelTestFCN testFunc( &selfCopy, fracDerivChi2, timeRanges ); 
  
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
    // ptime time = selfCopy.m_cgmsData.getAbsoluteTime( ge.m_minutes );
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
  // Int_t dummy_arg = 0;
  // if( !gTheApp ) gTheApp = new TApplication("App", &dummy_arg, (char **)NULL);
  
   
  TGraph *cgmsBG  = m_cgmsData.getTGraph( t_start, t_end );
  TGraph *predBG  = m_predictedBloodGlucose.getTGraph( t_start, t_end );
  TGraph *glucAbs = m_glucoseAbsorbtionRate.getTGraph( t_start, t_end );
  TGraph *insConc = m_freePlasmaInsulin.getTGraph( t_start, t_end );
  TGraph *xPred   = m_predictedInsulinX.getTGraph( t_start, t_end );
  
  double maxHeight = std::max( cgmsBG->GetMaximum(), predBG->GetMaximum() );
  double minHeight = std::min( cgmsBG->GetMinimum(), predBG->GetMinimum() );
  
  if( predBG->GetN() < 4 )
  {
    maxHeight = cgmsBG->GetMaximum();
    minHeight = cgmsBG->GetMinimum();
  }//if( we don;thave a predictions )
  
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
  
  //Now adjust for the time of CGMS
  map<double, string> labelMap;
  TAxis *cgmsAxis = cgmsBG->GetXaxis();
  
  for( int i=0; i<cgmsBG->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    cgmsBG->GetPoint( i, x, y );
    int origBin = cgmsAxis->FindBin( x );
    labelMap[x - toNMinutes(m_cgmsDelay)] = cgmsAxis->GetBinLabel(origBin);
    cgmsBG->SetPoint( i, x - toNMinutes(m_cgmsDelay), y);
  }//for( loop over glucAbs points )
  
  map<double, string>::iterator iter;
  for( iter = labelMap.begin(); iter != labelMap.end(); ++iter )
  {
    int bin = cgmsAxis->FindBin( iter->first );
    cgmsAxis->SetBinLabel( bin, iter->second.c_str() );
  }//
  
  // minHeight -= 0.2 * abs(minHeight);
  minHeight -= 0.25 * graphRange; 
  maxHeight += 0.23 * abs(maxHeight);

  cgmsBG->SetMinimum( minHeight );
  predBG->SetMinimum( minHeight );
  
  cgmsBG->SetMaximum( maxHeight );
  predBG->SetMaximum( maxHeight );
  
  cgmsBG->SetLineColor(1);  //black
  predBG->SetLineColor(2);   //red
  glucAbs->SetLineColor(4);   //blue
  insConc->SetLineColor(28);  //brown
  xPred->SetLineColor(28);  //brown
  
  cgmsBG->SetLineWidth(2);
  predBG->SetLineWidth(2);
  glucAbs->SetLineWidth(2);
  insConc->SetLineWidth(1);
  xPred->SetLineWidth(1);
  
  insConc->SetLineStyle( 5 );
  
  predBG->SetTitle( "" );
  
  if( !gPad )  new TCanvas();
  
  if( predBG->GetN() )
  {
    predBG->Draw( "Al" );
    cgmsBG->Draw( "l" );
  }else cgmsBG->Draw( "Al" );
  
  if( glucAbs->GetN() ) glucAbs->Draw( "l" );
  if( xPred->GetN() )   xPred->Draw( "l" );
  if( insConc->GetN() ) insConc->Draw( "l" );
  
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
  leg->AddEntry( predBG, "Predicted Blood-Glucose", "l" );
  leg->AddEntry( xPred, "Effective Insulin", "l" );
  leg->AddEntry( insConc, "Predicted Free-Plasma Insulin", "l" );
  leg->AddEntry( glucAbs, "Rate Of Glucose Absorbtion", "l" );
  leg->Draw();
  
  if( pause )
  {
    gTheApp->Run(kTRUE);
    delete gPad;
    delete cgmsBG;
    delete predBG;
    delete glucAbs;
    delete insConc;
    // delete axis;
  }//if( pause )
}//draw()


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

  // ar & m_t0;
  ar & m_dt;
  ar & m_predictAhead;
  
  ar & m_effectiveDof; //So Minuit2 can properly interpret errors
  ar & m_paramaters;           //size == NumNLSimplePars
  ar & m_paramaterErrorPlus;
  ar & m_paramaterErrorMinus;
    
  ar & m_currCgmsCorrFactor;
    
  ar & m_cgmsData;
  ar & m_freePlasmaInsulin;
  ar & m_glucoseAbsorbtionRate;
    
  ar & m_predictedInsulinX;
  ar & m_predictedBloodGlucose;
      
  ar & m_startSteadyStateTimes;
}//NLSimple::serialize
    



bool NLSimple::saveToFile( std::string filename )
{
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
                            double fracDerivChi2,
                            std::vector<TimeRange> timeRanges  ) 
  : m_modelPtr( modelPtr ), m_fracDerivChi2(fracDerivChi2), 
    m_timeRanges( timeRanges )
{
  assert( m_modelPtr );  
  assert( fracDerivChi2 >=0.0 && fracDerivChi2 <=1.0 );
  
  if( m_timeRanges.empty() ) 
    m_timeRanges.push_back( TimeRange( kGenericT0, kGenericT0 ) );
}//ModelTestFCN constructor



Double_t ModelTestFCN::EstimatorFunction( std::vector<Double_t>& parameters )
{
  return this->operator()(parameters);
}//EstimatorFunction



double ModelTestFCN::operator()(const std::vector<double>& x) const
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
  m_modelPtr->setModelParameters( x );
  
  double chi2 = 0.0;
  
  // foreach( double xi, x ) cout << " " << xi;
  // cout << endl;
  
  foreach( const TimeRange &tr, m_timeRanges )
  {
    ptime tStart = m_modelPtr->findSteadyStateStartTime( tr.first, tr.second );
    
    if( predTime > time_duration(0,0,0,0) )
    {
      ConsentrationGraph 
      predGluc =  m_modelPtr->glucPredUsingCgms( -1, tStart, tr.second );
      
      if( predGluc.empty() )
      {
        cout << "ModelTestFCN: Warning, for paramaters ";
        foreach( double d, x ) cout << " " << d;
        cout << "I could not make a prediction" << endl;
        return DBL_MAX;
      }//
      chi2 += m_modelPtr->getChi2ComparedToCgmsData( predGluc, m_fracDerivChi2, 
                                                     tStart + predTime + cgmsDelay, 
                                                     tr.second );
    } else
    {
      m_modelPtr->performModelGlucosePrediction( tStart, tr.second );
      chi2 += m_modelPtr->getModelChi2( m_fracDerivChi2, tStart, tr.second );
    }//if( usePredictedChi2 ) else 
  }//foeach(...)
  
  m_modelPtr->setModelParameters( std::vector<double>(0) );
  
  return chi2;
}//operator()


