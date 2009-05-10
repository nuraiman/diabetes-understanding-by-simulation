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


void NLSimple::setModelParameters( std::vector<double> &newPar )
{
  assert( newPar.size() == NumNLSimplePars );
  
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
  
  const ptime cgmsEndTime =  m_cgmsData.getEndTime() - minutes( (int)m_cgmsDelay);
  const ptime insulinEndTime = m_freePlasmaInsulin.getEndTime();
  
  const ptime endTime   = std::min(cgmsEndTime, insulinEndTime);
  const ptime startTime = std::max(cgmsStartTime, insulinStartTime);
  const double endRelTime = getOffset(endTime);
  const double startRelTime = getOffset(startTime);
 
  const double dt = 1.0;
  
  double X_max = 0.0;
  double X = 0.0;
  vector<double> xValue;
  
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
  if( cgmsStartTime < (insulinStartTime - hours(3)) ) 
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
        cout << "Found start of steady state time at " << to_simple_string(time) 
             << endl;
      }//if( has become zero )
    }//if( isZeroX ) / else
  }//for( loop over X's we have found )
  
}//findSteadyStateBeginings



double NLSimple::performModelGlucosePrediction( boost::posix_time::ptime t_start,
                                                boost::posix_time::ptime t_end,
                                                double bloodGlucose_initial,
                                                double bloodX_initial )
{
  using namespace gregorian;
  
  findSteadyStateBeginings();
  
  boost::posix_time::ptime endTime   = t_end;
  boost::posix_time::ptime startTime = t_start;
  

  
  if( (startTime == kGenericT0) || (bloodX_initial == kFailValue) )
  {
    if( m_startSteadyStateTimes.empty() )
    {
      cout << "double NLSimple::performModelGlucosePrediction(...):"
           << " I was unable to find any approproate steady state start times,"
           << endl;
      exit(1);
    }//if( m_startSteadyStateTimes.empty() )
    
    startTime = m_startSteadyStateTimes[0];
    
    if( (bloodX_initial == kFailValue) && (t_start != kGenericT0) )
    {
      for( size_t i=0; i<m_startSteadyStateTimes.size() && startTime<t_start; ++i )
        startTime = m_startSteadyStateTimes[i];
      
      if( startTime < t_start )
      {
        cout << "I couldn't find a steady state 'X', occuring after " << t_start
             << endl;
        exit(1);
      }//if( startTime < t_start )
      
      bloodX_initial = 0.0;
    }//if( specified start time, but not initial X )
  
    if(startTime == kGenericT0) bloodX_initial = 0.0;
  
    if( bloodGlucose_initial > 0.0 )
    {
      cout << "double NLSimple::performModelGlucosePrediction(...):"
           << " you can specify an initial blood glucose, but no time" << endl;
      exit(1);
    }//if( bloodGlucose_initial < 30 )
    
    bloodGlucose_initial = m_cgmsData.value( startTime + minutes(m_cgmsDelay) );
    
    if( bloodGlucose_initial < 1 )
    {
      cout << "double NLSimple::performModelGlucosePrediction(...):"
           << " I only have cgms data through "
           << to_simple_string( m_cgmsData.getEndTime() )
           << " and the initial time is " 
           << to_simple_string(startTime) << endl;
      exit(1);
    }//if( bloodGlucose_initial < 30 )
  }//if( t_start== kGenericT0 )

    
  //if no end time will go till max of m_freePlasmaInsulin + 1 hour,
  //  or m_glucoseAbsorbtionRate + 1 hour
  if( endTime == kGenericT0 )
  {
    ptime insulinLastTime = m_freePlasmaInsulin.getEndTime() + hours(1);
    ptime glucoseLastTime = m_glucoseAbsorbtionRate.getEndTime() + hours(1);
    
    endTime = std::max( insulinLastTime, glucoseLastTime );
  }//if( endTime== kGenericT0 )
  
  if( m_predictedInsulinX.containsTime( startTime )
      || m_predictedInsulinX.containsTime( endTime ) )
    m_predictedInsulinX.clear();
  
  if( m_predictedBloodGlucose.containsTime( startTime )
      || m_predictedBloodGlucose.containsTime( endTime ) )
    m_predictedBloodGlucose.clear();
  
  const double dtInMin = m_dt.total_microseconds() / 1.0E6 / 60.0;
  vector<double> gAndX(2);
  gAndX[0] = bloodGlucose_initial - m_basalGlucoseConcentration;
  gAndX[1] = bloodX_initial;
  RK_DerivFuntion derivFunc = getRKDerivFunc();
  
  
  cout << "About to start integrating between " << to_simple_string(startTime) 
       << " and  " << to_simple_string(endTime) 
       << ", With G_0=" << gAndX[0] << " and X_0=" << gAndX[1] << endl;
  
  for( ptime time = startTime; time < endTime; time += m_dt )
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
         << " for full time of " << t_start << " to " << t_end << " am assuming"
         << " basal glucose consentrations for these values" << endl;
  }//if( model data doesnt' extend everywhere )
  
  cout << "About to find magnitude based chi2 between " << t_start << " and "
       << t_end << endl;
       
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
    const double modelValue =  modelData.value(time); //+modelData.getYOffset()
    const double uncert = fracUncert*cgmsValue;
    
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
         << " for full time of " << t_start << " to " << t_end << " am assuming"
         << " basal glucose consentrations for these values" << endl;
  }//if( model data doesnt' extend everywhere )

  cout << "About to find slope based chi2 between " << t_start << " and "
       << t_end << endl;
       
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
    const double modelValue = modelDerivData.value(time);
    const double uncert = fracUncert*cgmsValue;
    
    chi2 += pow( (modelValue - cgmsValue), 2 );
  }//for( loop over cgms data points )
  
  
  return chi2;
}//getDerivativeChi2
    
    
double NLSimple::fitModelToData( std::vector<TimeRange> timeRanges ) {assert(0);return 1;}



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
  
  const double glucScale = maxHeight / glucMax;
  const double glucOffset = (minHeight - glucMin) / glucScale;
  const double glucNewMin = minHeight;
  
  //Scale glucose absorbtion rate to be viewable
  for( int i=0; i<glucAbs->GetN(); ++i )
  {
    double x=0.0, y=0.0;
    glucAbs->GetPoint( i, x, y );
    glucAbs->SetPoint( i, x, glucScale * y - glucOffset);
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
    
