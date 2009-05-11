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

#include "TLegend.h"
#include "TGraph.h"
#include "TGAxis.h"
#include "TCanvas.h"
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


#include "ResponseModel.hh"
#include "KineticModels.hh"
#include "CgmsDataImport.hh"
#include "RungeKuttaIntegrater.hh"


using namespace std;
using namespace boost;
using namespace boost::posix_time;


extern TApplication *gTheApp;

//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH


/*
NLSimple::NLSimple( const NLSimple &rhs ) :
     m_description( rhs.m_description ), 
     m_cgmsDelay( rhs.m_cgmsDelay ),
     m_basalInsulinConc( rhs.m_basalInsulinConc ),
     m_basalGlucoseConcentration( rhs.m_basalGlucoseConcentration ), 
     m_t0( rhs.m_t0 ),
     m_dt( 0, 1, 0, 0),
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
m_description(description), m_cgmsDelay(ModelDefaults::kDefaultCgmsDelay),
     m_basalInsulinConc( getBasalInsulinConcentration(basalUnitsPerKiloPerhour) ),
     m_basalGlucoseConcentration( basalGlucoseConcen ), 
     m_t0( t0 ),
     m_dt( 0, 1, 0, 0),
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


const NLSimple &NLSimple::operator=( const NLSimple &rhs )
{
  m_description                = rhs.m_description;
    
  m_t0                         = rhs.m_t0;
  m_dt                         = rhs.m_dt;
  
  m_cgmsDelay                  = rhs.m_cgmsDelay;
  m_basalInsulinConc           = rhs.m_basalInsulinConc;
  m_basalGlucoseConcentration  = rhs.m_basalGlucoseConcentration;\
    
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
  long mins = (long) nOffsetMinutes;
  nOffsetMinutes -= mins;
  nOffsetMinutes *= 60.0;
  long secs = (long)nOffsetMinutes;
  
  time_duration td = minutes(mins) + seconds(secs);
  
  return m_t0 + td;
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


double NLSimple::dXdT_usingCgmsData( double time, double X ) const
{
  // double cgmsTime = time + m_cgmsDelay;
  // const ptime absCgmsPTime = getAbsoluteTime(cgmsTime);
  const ptime absTime = getAbsoluteTime(time);
  
  //The 10 is for convertin U/L, to U/dL
  const double insulin= (m_freePlasmaInsulin.value( absTime ) / 10.0);
  
  double dXdT = -X * m_paramaters[XMultiplier]
                + insulin * m_paramaters[PlasmaInsulinMultiplier];
               //+ X_parameters[2] *(G - X_parameters[3]);
  
  return dXdT; 
}//double dXdT_usingCgmsData( double time, double X ) const

    


vector<double> NLSimple::dGdT_and_dXdT( double time, vector<double> G_and_X ) const
{
  assert( G_and_X.size() == 2 );
  const ptime properTime = getAbsoluteTime(time);
  
  const double G = G_and_X[0];
  const double X = G_and_X[1];
  
  G_and_X[0] = dGdT(properTime, G, X );
  G_and_X[1] = dXdT(properTime, G, X );
  
  return G_and_X;
}//dGdT_and_dXdT


RK_DerivFuntion NLSimple::getRKDerivFunc() const
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
  const time_duration cgmsDelay = getAbsoluteTime( m_cgmsDelay ) 
                                  - getAbsoluteTime(0);
                                  
  const ptime cgmsEndTime =  m_cgmsData.getEndTime() - cgmsDelay;
  const ptime insulinEndTime = m_freePlasmaInsulin.getEndTime();
  
  const ptime endTime   = std::min(cgmsEndTime, insulinEndTime);
  const ptime startTime = std::max(cgmsStartTime, insulinStartTime);
  const double endRelTime = getOffset(endTime);
  const double startRelTime = getOffset(startTime);
 
  const double dt = 1.0;
  
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
  
  for( double time = startRelTime; time < endRelTime; time += dt )
  {
    ForcingFunction derivFunc 
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
        const ptime time = getAbsoluteTime( startRelTime + mins );
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


double NLSimple::performModelGlucosePrediction( boost::posix_time::ptime t_start,
                                                boost::posix_time::ptime t_end,
                                                double bloodGlucose_initial,
                                                double bloodX_initial )
{    
  boost::posix_time::ptime endTime   = t_end;
  boost::posix_time::ptime startTime = findSteadyStateStartTime( t_start, t_end );
  
  
  if( bloodGlucose_initial > 0.0 && t_start==kGenericT0 )
  {
    cout << "double NLSimple::performModelGlucosePrediction(...):"
         << " you can specify an initial blood glucose, but no time" << endl;
    exit(1);
  }//if( bloodGlucose_initial < 30 )
    
    
  if( bloodX_initial != kFailValue ) startTime = t_start;
  else bloodX_initial = 0.0;
    
  time_duration cgmsDelay;
  cgmsDelay = getAbsoluteTime( getOffset(m_t0) + m_cgmsDelay ) - m_t0;
    
  bloodGlucose_initial = m_cgmsData.value( startTime + cgmsDelay );
  if( bloodGlucose_initial < 1 )
  {
    cout << "double NLSimple::performModelGlucosePrediction(...):"
         << " I only have cgms data through "
         << to_simple_string( m_cgmsData.getEndTime() )
         << " and the initial time is " 
         << to_simple_string(startTime) << endl;
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
  
  
  const double dtInMin = m_dt.total_microseconds() / 1.0E6 / 60.0;
  vector<double> gAndX(2);
  gAndX[0] = bloodGlucose_initial - m_basalGlucoseConcentration;
  gAndX[1] = bloodX_initial;
  RK_DerivFuntion derivFunc = getRKDerivFunc();
  
  
  // cout << "About to start integrating between " << to_simple_string(startTime) 
       // << " and  " << to_simple_string(endTime) 
       // << ", With G_0=" << gAndX[0] << " and X_0=" << gAndX[1] << endl;
  
  for( ptime time = startTime; time <= endTime; time += m_dt )
  {
     double timeInOffset = getOffset( time );
     
     gAndX = rungeKutta4( timeInOffset, gAndX, dtInMin, derivFunc );
     
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
  
  const time_duration cgmsDelay = modelData.getAbsoluteTime(m_cgmsDelay)  
                                   - modelData.getAbsoluteTime(0);
  
  const ptime effCgmsStart = cgmsData.getStartTime() - cgmsDelay;
  const ptime effCgmsEnd = cgmsData.getEndTime() - cgmsDelay;
  
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
  
  double offset = cgmsData.getOffset(t_start + cgmsDelay);
  ConstGraphIter start = cgmsData.lower_bound( GraphElement( offset, 0 ) );
  offset = cgmsData.getOffset(t_end + cgmsDelay);
  ConstGraphIter end = cgmsData.upper_bound( GraphElement( offset, 0 ) );
  
  for( ConstGraphIter iter = start; iter != end; ++iter )
  {
    const ptime time = cgmsData.getAbsoluteTime( iter->m_minutes ) - cgmsDelay;
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
  
  const time_duration cgmsDelay = modelDerivData.getAbsoluteTime(m_cgmsDelay)  
                                   - modelDerivData.getAbsoluteTime(0);
  
  const ptime effCgmsStart = cgmsDerivData.getStartTime() - cgmsDelay;
  const ptime effCgmsEnd = cgmsDerivData.getEndTime() - cgmsDelay;
  
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
  
  double offset = cgmsDerivData.getOffset(t_start + cgmsDelay);
  ConstGraphIter start = cgmsDerivData.lower_bound( GraphElement( offset, 0 ) );
  offset = cgmsDerivData.getOffset(t_end + cgmsDelay);
  ConstGraphIter end = cgmsDerivData.upper_bound( GraphElement( offset, 0 ) );
  
  for( ConstGraphIter iter = start; iter != end; ++iter )
  {
    const ptime time = cgmsDerivData.getAbsoluteTime( iter->m_minutes ) - cgmsDelay;
    const double cgmsValue  = iter->m_value;
    double modelValue = modelDerivData.value(time);
    const double uncert = fracUncert*cgmsValue;
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
  
  Int_t fPopSize      = 200;
  Int_t fNsteps       = 10;
  Int_t fSC_steps     = 6;
  Int_t fSC_rate      = 3;
  Double_t fSC_factor = 0.5;
  Double_t fConvCrit  = 1.0;
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
  ranges.push_back( new Interval( 1.0, 10.0 )  );
  ranges.push_back( new Interval( 0.0, 0.1 )  );
  ranges.push_back( new Interval( 0.0, 0.001 )  );
  
  
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
    
    // reduce the population size to the initially defined one
    ga.GetGeneticPopulation().TrimPopulation(); 
    
    //Here is where we could change the different factors
    ga.SpreadControl( fSC_steps, fSC_rate, fSC_factor );
  }while ( !ga.HasConverged( fNsteps, fConvCrit ) );
  // converged if: fitness-improvement < CONVCRIT within the last CONVSTEPS loops

  GeneticGenes* genes = ga.GetGeneticPopulation().GetGenes( 0 );
  std::vector<Double_t> gvec;
  gvec = genes->GetFactors();

  setModelParameters(gvec);
  
  double chi2 = 0.0;
  foreach( const TimeRange &tr, timeRanges )
  {
    ptime tStart = findSteadyStateStartTime( tr.first, tr.second );
    
    performModelGlucosePrediction( tStart, tr.second );
    chi2 += getModelChi2( fracDerivChi2, tStart, tr.second );
  }//foeach(...)  
  
  return chi2;
}//geneticallyOptimizeModel


    
double NLSimple::fitModelToDataViaMinuit2( double fracDerivChi2, 
                                           vector<TimeRange> timeRanges ) 
{  
  using namespace ROOT::Minuit2;
  
  vector<double> startingVal( NumNLSimplePars );
  startingVal[BGMultiplie]              = 0.0;
  startingVal[CarbAbsorbMultiplier]     = 30.0 / 9.0;
  startingVal[XMultiplier]             = 0.040;
  startingVal[PlasmaInsulinMultiplier] = 0.00015;
      
      //
  if( m_paramaters.size() && m_paramaters[0] != kFailValue ) 
  {
    assert( m_paramaters.size() == NumNLSimplePars );
    
    startingVal = m_paramaters;
    cout << "fitModelToDataViaMinuit2(...): using the existing model parameters"
         << " as starting values" << endl;
  }//if( use existing parameters )
  
  
  m_paramaters.clear();
  resetPredictions();
  findSteadyStateBeginings(3);
  
  ModelTestFCN modelFCN( this, fracDerivChi2, timeRanges );
  modelFCN.SetErrorDef(10.0);
  
  MnUserParameters upar;
  upar.Add( "BGMult"   , startingVal[BGMultiplie], 0.1 );
  upar.SetLowerLimit(0, -0.01);
  upar.SetUpperLimit(0, 0.1);
  
  upar.Add( "CarbMult" , startingVal[CarbAbsorbMultiplier], 0.1 );
  upar.SetLowerLimit(1, 1.0);
  upar.SetUpperLimit(1, 10);
  
  upar.Add( "XMult"    , startingVal[XMultiplier], 0.1 );
  upar.SetLowerLimit(2, 0.0);
  upar.SetUpperLimit(2, 0.1);
  
  upar.Add( "InsulMult", startingVal[PlasmaInsulinMultiplier], 0.1 );
  upar.SetLowerLimit(3, 0.0);
  upar.SetUpperLimit(3, 0.001);

  
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
  
  double chi2 = 0.0;
  foreach( const TimeRange &tr, timeRanges )
  {
    ptime tStart = findSteadyStateStartTime( tr.first, tr.second );
    
    performModelGlucosePrediction( tStart, tr.second );
    chi2 += getModelChi2( fracDerivChi2, tStart, tr.second );
  }//foeach(...)  
  
  return chi2;
}//fitModelToData



void NLSimple::draw( boost::posix_time::ptime t_start,
                     boost::posix_time::ptime t_end  ) 
{
  Int_t dummy_arg = 0;
  
  if( !gTheApp ) gTheApp = new TApplication("App", &dummy_arg, (char **)NULL);
  
  
  // m_predictedBloodGlucose.setYOffsetForDrawing( m_basalGlucoseConcentration );
  // m_freePlasmaInsulin.setYOffsetForDrawing( m_basalInsulinConc );
  
  TGraph *cgmsBG  = m_cgmsData.getTGraph();
  TGraph *predBG  = m_predictedBloodGlucose.getTGraph();
  TGraph *glucAbs = m_glucoseAbsorbtionRate.getTGraph();
  TGraph *insConc = m_freePlasmaInsulin.getTGraph();
  TGraph *xPred   = m_predictedInsulinX.getTGraph();
  
  double maxHeight = std::max( cgmsBG->GetMaximum(), predBG->GetMaximum() );
  double minHeight = std::min( cgmsBG->GetMinimum(), predBG->GetMinimum() );
  
  const double glucMax = glucAbs->GetMaximum();
  const double glucMin = glucAbs->GetMinimum();
  
  const double graphRange = maxHeight - minHeight;
  const double glucRange = glucMax - glucMin;
  const double glucScale = graphRange / glucRange;//maxHeight / glucMax;
  const double glucOffset = (minHeight - glucMin);// / glucScale;
  const double glucNewMin = minHeight;
  
  //Scale glucose absorbtion rate to be viewable
  for( int i=0; i<glucAbs->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    glucAbs->GetPoint( i, x, y );
    glucAbs->SetPoint( i, x, glucScale * (y-glucMin) + glucOffset);
  }//for( loop over glucAbs points )
  
  const double insulinMax = insConc->GetMaximum();
  const double insulinMin = insConc->GetMinimum();
  
  cout << "Insulin concen max=" << insulinMax << ", min=" << insulinMin << endl;
  
  const double insulinScale = maxHeight / insulinMax;
  const double insulinOffset = (minHeight - insulinMin) / insulinScale;
  const double insulinNewMin = minHeight;
  
  cout << "insulinOffset=" << insulinOffset << ", insulinScale=" << insulinScale << endl;
  
  //Scale insulin concentration to be viewable
  for( int i=0; i<insConc->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    insConc->GetPoint( i, x, y );
    insConc->SetPoint( i, x, insulinScale * y - insulinOffset);
  }//for( loop over glucAbs points )
  
  
  
  const double xMax = xPred->GetMaximum();
  const double xMin = xPred->GetMinimum();
  const double xScale = maxHeight / xMax;
  const double xOffset = (minHeight - xMin) / xScale;
  const double xNewMin = minHeight;
  
  for( int i=0; i<xPred->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    xPred->GetPoint( i, x, y );
    xPred->SetPoint( i, x, xScale * y - xOffset);
  }//for( loop over glucAbs points )
  
  //Now adjust for the time of CGMS
  for( int i=0; i<cgmsBG->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    cgmsBG->GetPoint( i, x, y );
    cgmsBG->SetPoint( i, x - m_cgmsDelay, y);
  }//for( loop over glucAbs points )
  
  
  minHeight -= 0.1 * abs(minHeight);
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
  insConc->SetLineWidth(2);
  xPred->SetLineWidth(2);
  
  insConc->SetLineStyle( 5 );
  
  predBG->SetTitle( "" );
  
  if( !gPad )  new TCanvas();
  
  predBG->Draw( "Al" );
  cgmsBG->Draw( "l" );
  glucAbs->Draw( "l" );
  xPred->Draw( "l" );
  insConc->Draw( "l" );
  
  TGaxis *axis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(),
                            gPad->GetUxmax(), gPad->GetUymax(), glucNewMin, maxHeight,510,"+L");
  axis->SetLineColor( 4 );
  axis->SetLabelColor( 4 );
  axis->SetTitleColor( 4 );
  axis->SetTitle( "Glucose Absorption Rate (mg/dL/min)" );
  axis->Draw();
  
  TLegend *leg = new TLegend( 0.65, 0.6, 0.95, 0.90);
  leg->SetBorderSize(0);
  leg->AddEntry( cgmsBG, "time-corrected CGMS data", "l" );
  leg->AddEntry( predBG, "Predicted Blood-Glucose", "l" );
  leg->AddEntry( xPred, "Effective Insulin", "l" );
  leg->AddEntry( insConc, "Predicted Free-Plasma Insulin", "l" );
  leg->AddEntry( glucAbs, "Rate Of Glucose Absorbtion", "l" );
  leg->Draw();
  
  
  gTheApp->Run(kTRUE);
  delete gPad;
  delete cgmsBG;
  delete predBG;
  delete glucAbs;
  delete insConc;
  delete axis;
}//draw()



double NLSimple::getFitDof() const { return m_effectiveDof; }
void NLSimple::setFitDof( double dof ) { m_effectiveDof = dof; }

double ModelTestFCN::Up() const { return m_modelPtr->getFitDof(); }
void ModelTestFCN::SetErrorDef(double dof) {  m_modelPtr->setFitDof(dof); }


ModelTestFCN::ModelTestFCN( NLSimple *modelPtr, 
                            double fracDerivChi2,
                            std::vector<TimeRange> timeRanges  ) 
  : m_modelPtr( modelPtr ), m_fracDerivChi2(fracDerivChi2), m_timeRanges( timeRanges )
{
  assert( m_modelPtr );  
  
  if( m_timeRanges.empty() ) 
    m_timeRanges.push_back( TimeRange( kGenericT0, kGenericT0 ) );
}//ModelTestFCN constructor



Double_t ModelTestFCN::EstimatorFunction( std::vector<Double_t>& parameters )
{
  return this->operator()(parameters);
}//EstimatorFunction



double ModelTestFCN::operator()(const std::vector<double>& x) const
{
  m_modelPtr->resetPredictions();
  m_modelPtr->setModelParameters( x );
  
  double chi2 = 0.0;
  
  // foreach( double xi, x ) cout << " " << xi;
  // cout << endl;
  
  foreach( const TimeRange &tr, m_timeRanges )
  {
    ptime tStart = m_modelPtr->findSteadyStateStartTime( tr.first, tr.second );
    
    m_modelPtr->performModelGlucosePrediction( tStart, tr.second );
    chi2 += m_modelPtr->getModelChi2( m_fracDerivChi2, tStart, tr.second );
  }//foeach(...)
  
  m_modelPtr->resetPredictions();
  m_modelPtr->setModelParameters( std::vector<double>(0) );
  
  return chi2;
}//operator()


