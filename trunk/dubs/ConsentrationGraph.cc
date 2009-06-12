//
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>  //contains M_PI
#include <stdlib.h>
#include <fstream>
#include <cmath>

//ROOT includes (using root 5.14)
#include "TSystem.h"
#include "TStyle.h"
#include "TF1.h"
#include "TGraph.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TPaveText.h"
#include "TVirtualFFT.h"


//GSL includes
//You may comment out these includes and code will still 
//  compile (edit Makefile too though), you just can't call spline interpolation
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_rng.h>

//Boost includes (using boost 1.3)
#include "boost/range.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/time_serialize.hpp"
#include "boost/date_time/gregorian/greg_serialize.hpp"

#include "boost/foreach.hpp"
#include "boost/assign/list_of.hpp" //for 'list_of()'
#include "boost/assign/list_inserter.hpp"
#include "boost/lexical_cast.hpp"

#include "ConsentrationGraph.hh"
#include "KineticModels.hh"
#include "CgmsDataImport.hh"
#include "RungeKuttaIntegrater.hh"
#include "ConsentrationGraphGui.hh"

using namespace std;
using namespace boost;
using namespace boost::posix_time;

//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH



GraphElement::GraphElement() : m_minutes(-999.9), m_value(-999.9) 
{}


GraphElement::GraphElement( double nMinutes, double value ) : 
     m_minutes(nMinutes), m_value(value) 
{}


bool GraphElement::operator<( const GraphElement &lhs ) const
{
  return (m_minutes < lhs.m_minutes);
}//GraphElement::operator<(...)






ConsentrationGraph::ConsentrationGraph( const ConsentrationGraph &rhs ) :
                    GraphElementSet( rhs ), 
                    m_t0( rhs.m_t0 ), 
                    m_dt( rhs.m_dt ), 
                    m_yOffsetForDrawing( rhs.m_yOffsetForDrawing ),
                    m_graphType( rhs.m_graphType )
{};


ConsentrationGraph::ConsentrationGraph( ptime t0, double dt, GraphType graphType ) :
                    GraphElementSet(), 
                    m_t0( t0 ), 
                    m_dt( dt ), 
                    m_yOffsetForDrawing( 0.0 ),
                    m_graphType( graphType )
{};

ConsentrationGraph::ConsentrationGraph( const std::string &savedFileName )
{
  *this = loadFromFile( savedFileName );
}//ConsentrationGraph(from a file )

const ConsentrationGraph &ConsentrationGraph::operator=( 
                                                const ConsentrationGraph &rhs )
{
  m_graphType = rhs.m_graphType;
  
  // if( m_graphType != rhs.m_graphType )
  // {
    // cout << "ConsentrationGraph, you cannot assign a" 
         // << " graphs of different types" << endl;
    // exit(1);
  // }//if( m_graphType != rhs.m_graphType )
  
  m_t0 = rhs.m_t0;
  m_dt = rhs.m_dt;
  m_yOffsetForDrawing = rhs.m_yOffsetForDrawing;
  
  std::set<GraphElement>::operator=( rhs );
  
  return *this;
}//operator=


void ConsentrationGraph::setT0_dontChangeOffsetValues( ptime newT0 ) 
{ 
  m_t0 = newT0; 
}//setT0_keepOffsetsSame


double ConsentrationGraph::getDt( const ptime &t0, const ptime &t1 )
{
  return toNMinutes( t1 - t0 );
}//static double getDt( ptime t0, ptime t1 ) const


GraphType ConsentrationGraph::getGraphType() const { return m_graphType; }

std::string ConsentrationGraph::getGraphTypeStr() const
{
  switch( m_graphType )
  {
    case InsulinGraph:               return "InsulinGraph";
    case BolusGraph:                 return "BolusGraph";
    case GlucoseConsentrationGraph:  return "GlucoseConsentrationGraph";
    case GlucoseAbsorbtionRateGraph: return "GlucoseAbsorbtionRateGraph";
    case GlucoseConsumptionGraph:    return "GlucoseConsumptionGraph";
    case BloodGlucoseConcenDeriv:    return "BloodGlucoseConcenDeriv";
    case NumGraphType:               return "NumGraphType";
  };//switch( m_graphType )
  
  assert(0);
  return "";
}//getGraphTypeStr

double ConsentrationGraph::getDt() const { return m_dt; }
boost::posix_time::ptime ConsentrationGraph::getT0() const { return m_t0; }

boost::posix_time::ptime ConsentrationGraph::getStartTime() const
{
  if( empty() ) return m_t0;
  
  return getAbsoluteTime( begin()->m_minutes );
}//getStartTime



boost::posix_time::ptime ConsentrationGraph::getEndTime() const
{
  if( empty() ) return m_t0;
  
  std::set<GraphElement>::const_reverse_iterator riter = rbegin();
  
  while( riter != rend() && riter->m_value==0.0 ) ++riter;
  
  if( riter == rend() ) return m_t0;
  
  return getAbsoluteTime( riter->m_minutes );
}//getEndTime



double ConsentrationGraph::getOffset( const ptime &absoluteTime ) const
{
  return getDt( m_t0, absoluteTime );
}//getOffset(..) in minutes


ptime ConsentrationGraph::getAbsoluteTime( double nOffsetMinutes ) const
{
  time_duration td = toTimeDuration(nOffsetMinutes) ;
  
  return m_t0 + td;
}//ptime getAbsoluteTime( double nOffsetMinutes ) const;



void ConsentrationGraph::trim( const PosixTime &t_start, const PosixTime &t_end )
{
  
  if( t_start != kGenericT0 )
  {
    ConstGraphIter ub = upper_bound(t_start);
    ConstGraphIter lb = lower_bound(t_start);
    
    if( lb == end() ) 
    {
      cout << "ConsentrationGraph::trim(...): Warning, you are to removing"
           << " all data, t_start (" << t_start 
           << ") is after the last data point (" << getEndTime() << ")" << endl;
    }// if( lb == end() ) 
    
    
    if( lb == ub )//the set<> does not exactly contain this time
    {
      double val = value(t_start);
      GraphElementSet::erase( begin(), lb );
      insert( t_start, val );
    }else //the set<> exactly contains this time
    {
      GraphElementSet::erase( begin(), lb );
    }//
  }//if( remove begining )
  
  if( t_end != kGenericT0 )
  {
    ConstGraphIter ub = upper_bound(t_start);
    ConstGraphIter lb = lower_bound(t_start);
    
    if( ub == begin() ) 
    {
      cout << "ConsentrationGraph::trim(...): Warning, you are to removing"
           << " all data, t_end (" << t_end
           << ") is before first data point (" << getStartTime() << ")" << endl;
    }// if( lb == end() ) 
    
    
    if( lb == ub )//the set<> does not exactly contain this time
    {
      double val = value(t_end);
      GraphElementSet::erase( ub, end() );
      insert( t_end, val );
    }else //the set<> exactly contains this time
    {
      GraphElementSet::erase( begin(), ub );
    }//
    
  }//if( remove begining )
  
  if( (t_start == kGenericT0) && (t_end == kGenericT0) )
  {
    cout << "ConsentrationGraph::trim(...): Warning, you made a useless call\n";
  }//
  
}//trim


bool ConsentrationGraph::containsTime( ptime absoluteTime ) const
{
  return containsTime( getOffset(absoluteTime) );
}//containsTime(...)


void trim( PosixTime t_start, PosixTime t_end );


bool ConsentrationGraph::containsTime( double nMinutesOffset ) const
{
  ConstGraphIter ending = GraphElementSet::end();
  ConstGraphIter lowB = GraphElementSet::lower_bound( GraphElement(nMinutesOffset, 0.0) );
  
  const bool timeMatch = (lowB != ending) && (lowB->m_minutes == nMinutesOffset );
  
  return timeMatch;
}//containsTime(...)

double ConsentrationGraph::valueUsingOffset( double nOffsetminutes ) const
{
  return value( nOffsetminutes );
}//ConsentrationGraph::valueUsingOffset(...)


double ConsentrationGraph::value( double nOffsetminutes ) const
{
  if( empty() ) return 0.0;
  if( nOffsetminutes < begin()->m_minutes ) return 0.0;
  
  //upper_bound returns element right past where 'nOffsetminutes' would be
  ConstGraphIter lowerBound = GraphElementSet::lower_bound( GraphElement(nOffsetminutes, 0.0) );
  
  if( lowerBound == GraphElementSet::end() )    return 0.0;
  if( lowerBound->m_minutes == nOffsetminutes ) return lowerBound->m_value;
  
  //Also BolusGraph is a discreet graph, so don't return a value
  //  unless time matches exactly, which we already would have
  if( m_graphType == BolusGraph ) return 0.0;
  
  ConstGraphIter prevElement = lowerBound;
  --prevElement;
  
  const double thisDt = lowerBound->m_minutes - prevElement->m_minutes;
  const double thisDConsen = lowerBound->m_value - prevElement->m_value;
  
  const double fracTime = (nOffsetminutes - prevElement->m_minutes) / thisDt;
  
  if( thisDConsen == 0.0 ) return prevElement->m_value;
  
  return prevElement->m_value + fracTime*thisDConsen;
}//double ConsentrationGraph::value( unsigned int nOffsetminutes ) const


ConstGraphIter ConsentrationGraph::insert( double offsetTime, double value )
{
  return addNewDataPoint(offsetTime, value );
  
  // const GraphElement newElement( offsetTime, value );
  // pair<ConstGraphIter,bool> pos = std::set<GraphElement>::insert( newElement );
  // 
  // if( !pos.second )
  // {
    // assert( lower_bound(newElement)->m_value == value );
  // }// 
  // return pos.first;
}//ConsentrationGraph::insert


ConstGraphIter ConsentrationGraph::insert( const ptime &absTime, double value )
{
  return addNewDataPoint(absTime, value );
  // double offset = getOffset( absTime );
  // return insert( offset, value );
}//insert


ConstGraphIter ConsentrationGraph::lower_bound( const PosixTime &time ) const
{
  const double offset = getOffset( time );
  return GraphElementSet::lower_bound( GraphElement(offset, kFailValue) );
}//lower_bound( ptime )

ConstGraphIter ConsentrationGraph::upper_bound( const PosixTime &time ) const
{
  const double offset = getOffset( time );
  return GraphElementSet::upper_bound( GraphElement(offset, kFailValue) );
}//upper_bound( ptime )



double ConsentrationGraph::value( ptime absTime ) const
{
  return value( getOffset(absTime) );
}//double ConsentrationGraph::value( ptime absTime ) const


double ConsentrationGraph::value( time_duration offset ) const
{
  return value( getOffset( m_t0 + offset ) );
}//double ConsentrationGraph::value( time_duration offset ) const
    

ConstGraphIter ConsentrationGraph::addNewDataPoint( double offset, double value )
{
  const GraphElement newElement( offset, value );
  ConstGraphIter posIter = GraphElementSet::lower_bound( newElement );
  
  if( !empty() && posIter!= end() && posIter!= begin() )
  {
    if( posIter->m_value != 0.0 )
    {
      ptime time = getAbsoluteTime(offset);
      ConstGraphIter endIter = end();
      --endIter;
      ptime lastTime = getAbsoluteTime( endIter->m_minutes );
    
      cout << "ConstGraphIter ConsentrationGraph::addNewDataPoint( double, double):"
           << " can not add time " << to_simple_string( time ) << " with value "
           << value << " to current "
           << " graph with t0 of " << to_simple_string( m_t0 ) 
           << " and end time of " << to_simple_string( lastTime ) 
           << endl;
      assert(0);
    }else
    {
      GraphElementSet::erase( posIter );
      ConstGraphIter newPos = GraphElementSet::insert( posIter, newElement ); 
      assert( newPos == posIter );
      return newPos;
    }//if( posIter->m_value != 0.0 ) / else
  }//if( need to protect against improperly added points )
    
  //If we made it here, we are in the clear
  pair<ConstGraphIter,bool> pos = GraphElementSet::insert( newElement ); 
  
  return pos.first;
}//addNewDataPoint


ConstGraphIter ConsentrationGraph::addNewDataPoint( ptime time, double value )
{
  return addNewDataPoint( getOffset(time), value );
}//add(...)

    
unsigned int ConsentrationGraph::removeNonInfoAddingPoints()
{
  if( size() < 3 ) return 0;
  
  vector<ConstGraphIter> pointsToErase;
  ConstGraphIter endPoint = end();
  --endPoint;  //endPoint is now last define element in ::GraphElementSet
  
  ConstGraphIter leftPoint =  begin();
  ConstGraphIter midPoint = begin();
  ++midPoint;
  ConstGraphIter rightPoint = begin();
  ++rightPoint;
  ++rightPoint;
  
  
  while( rightPoint != endPoint ) 
  {
    if( (leftPoint->m_value == 0.0) 
        && (midPoint->m_value == 0.0) 
        && (rightPoint->m_value == 0.0) )
    {
      pointsToErase.push_back( midPoint );
    }else ++leftPoint;  //only increment left point if all are not zero
    
    ++midPoint;
    ++rightPoint;
  }//while( midPoint != end() ) 
  

  if( (size() - pointsToErase.size()) > 1 )
  {
    leftPoint  = endPoint;
    --leftPoint;
  
    if( (leftPoint->m_value == 0.0) && (endPoint->m_value == 0.0) ) 
    {
      pointsToErase.push_back( endPoint );
    } 
  }//if( more than one point left, remove trailing zeros )
  
  if( (size() - pointsToErase.size()) > 1 )
  {
    leftPoint = begin();
    rightPoint = begin();
    ++rightPoint;
  
    if( (leftPoint->m_value == 0.0) && (rightPoint->m_value == 0.0) ) 
    {
      pointsToErase.push_back( leftPoint );
    } 
  }//if( more than one point left, remove leading zeros )

  
  foreach( const ConstGraphIter &iter, pointsToErase )
  {
    std::set<GraphElement>::erase( iter );
  }
  
  return pointsToErase.size();
}//removeNonInfoAddingPoints


unsigned int ConsentrationGraph::add( double amountPerVol, 
                                      double beginTimeOffset, 
                                      AbsFuncPointer absorbFunc )
{ 
  //We will allow absorbFunc(...) to be zero to begin with, but once it
  //  becomes non-zero, the next time it is zero, it will be zero forever on.
  bool hasBecomeNonZero = false;

  unsigned int nPointsAdded = 0;
  double addValue = 0.0;
  //First we will add elements to the base GraphElementSet where absorbFunc
  //  will be non-zero
  //  Note that new points inbetween existing points will be added in as well,
  //  This mmight change in the future (there is some possibility of numerical
  //  instabilities in doing this)
  for( double currentTime = 0.0; 
      (!hasBecomeNonZero || (addValue > 0.0)); currentTime += m_dt )
  {
    double relTime = beginTimeOffset + currentTime;
    double thisAbsorbOffsetTime = currentTime - beginTimeOffset;
    
    if( !containsTime( relTime ) )
    {
      GraphElementSet::insert( GraphElement(relTime, 0.0) );
      ++nPointsAdded;
    }//if( we don't have an element for this time yet )
    
    addValue = absorbFunc( amountPerVol, thisAbsorbOffsetTime );
    
    if( !hasBecomeNonZero ) hasBecomeNonZero = (addValue > 0.0 );
  }//for( the absorbtion function has more in it )
  
  
  //We have to do is loop through already defined times, and update those values
  //The first place equal to, or past beginTimeOffset
  ConstGraphIter posIter = GraphElementSet::upper_bound( GraphElement(beginTimeOffset, 0.0) );
  
  for( ; posIter != GraphElementSet::end(); ++posIter )
  {
    double currentTime = posIter->m_minutes;
    double thisAbsorbOffsetTime = currentTime - beginTimeOffset;
    
    assert( thisAbsorbOffsetTime >= 0.0 );
    
    const double addValue = absorbFunc( amountPerVol, thisAbsorbOffsetTime );
    const double totalValue = addValue + value( posIter->m_minutes );
    
    GraphElement newElement( posIter->m_minutes, totalValue );
    
    GraphElementSet::erase( posIter );
    ConstGraphIter newPos = GraphElementSet::insert( posIter, newElement ); 
    assert( newPos == posIter );
  }//for( loop over previously defined times )
  
  removeNonInfoAddingPoints();
  
  return nPointsAdded;
}//ConsentrationGraph::add(...)
    


unsigned int ConsentrationGraph::add( double amountPerVol, ptime absTime, 
                                      AbsFuncPointer absFunction )
{
  const double nMinutes = getOffset( absTime );
  
  return add( amountPerVol, nMinutes, absFunction );
}//ConsentrationGraph::add(...)


unsigned int ConsentrationGraph::add( double amountPerVol, ptime absoluteTime, 
                                                     AbsorbtionFunction absFunc)
{
  const double nMinutes = getOffset( absoluteTime );
  
   switch( absFunc )
  {
    case NovologAbsorbtion:        
      assert( m_graphType==InsulinGraph ); break;
      
    case FastCarbAbsorbtionRate:   
      assert( m_graphType==GlucoseAbsorbtionRateGraph ); break;
      
    case MediumCarbAbsorbtionRate: 
      assert( m_graphType==GlucoseAbsorbtionRateGraph ); break;
      
    case SlowCarbAbsorbtionRate:   
      assert( m_graphType==GlucoseAbsorbtionRateGraph ); break;
      
    case  NumAbsorbtionFunctions: break;
  };//switch( absFunction )
  
  return add( amountPerVol, nMinutes, absFunc );
}//ConsentrationGraph::add(...)




unsigned int ConsentrationGraph::add( double amountPerVol, double timeOffset, 
                                      AbsorbtionFunction absFunc )
{
  AbsFuncPointer func = getFunctionPointer( absFunc );
  
  return add( amountPerVol, timeOffset, func );
}//ConsentrationGraph::add(...)



ConsentrationGraph ConsentrationGraph::getTotal( const ConsentrationGraph &rhs, 
                                                 const double weight ) const
{
  
  if( m_graphType != rhs.m_graphType )
  {
    cout << "ConsentrationGraph::getTotal(...): I can't add"
         << " a graph of type " << rhs.getGraphTypeStr()
         << " to this graph of type " <<  getGraphTypeStr() << endl;
    exit(1);
  }//if( m_graphType != rhs.m_graphType )
  
  
  const double minDt = std::min( m_dt, rhs.m_dt );
  const ptime startTime = std::min( m_t0, rhs.m_t0 );
  
  ConsentrationGraph total( startTime, minDt, m_graphType);
  
  foreach( const GraphElement &ge, *static_cast<const GraphElementSet *>(this) )
  {
      const ptime absTime = getAbsoluteTime( ge.m_minutes );
      const double value = ge.m_value + (weight * rhs.value( absTime ));
    
    total.insert( absTime, value );
  }//foreach( base element of *this )
  
  foreach( const GraphElement &ge, static_cast< const GraphElementSet &>(rhs) )
  {
    const ptime absTime = getAbsoluteTime( ge.m_minutes );
    
    if( !total.containsTime( absTime ) )
    {
      const double val =  value(absTime) + (weight * ge.m_value);
    
      total.insert( absTime, val );
    }//if( we haven't already inserted this time )
  }//foreach( base element of rhs )
  
  return total;
}//getTotal




ConsentrationGraph ConsentrationGraph::getTotal( double amountPerVol,  
                                                 ptime functionT0, 
                                                 AbsFuncPointer f ) const
{
  const ptime startTime = std::min( m_t0, functionT0 );
  
  ConsentrationGraph duplicateGraph( startTime, m_dt, m_graphType);
  
  //makesure 'total' has an element for all necassary times
  foreach( const GraphElement &ge, *static_cast< const GraphElementSet *>(this) )
  {
    const ptime absTime = getAbsoluteTime( ge.m_minutes );
    const double answerRelOffset = duplicateGraph.getOffset( absTime );
    // const double functionRelOffset = getDt( functionT0, absTime );
    
    const double value = ge.m_value;
    
    duplicateGraph.insert( answerRelOffset, value );
  }//foreach( element in *this )
  
  
  ConsentrationGraph total( startTime, m_dt, m_graphType);
  
  foreach( const GraphElement &ge, static_cast<GraphElementSet &>(duplicateGraph) )
  {
    const ptime absTime = total.getAbsoluteTime( ge.m_minutes );
    const double functionRelOffset = getDt( functionT0, absTime );
    double val = ge.m_value + f( amountPerVol, functionRelOffset);
    
    total.insert( ge.m_minutes, val );
  }//foreach( eleement in 'total' )
  
  return total;
}//getTotal



ConsentrationGraph ConsentrationGraph::getTotal( double amountPerVol, 
                                                 ptime absoluteTime, 
                                             AbsorbtionFunction absFunc ) const
{
  AbsFuncPointer func = getFunctionPointer( absFunc );
  
  return getTotal( amountPerVol, absoluteTime, func );
}//getTotal


std::string ConsentrationGraph::getTimeNoDate( boost::posix_time::ptime time )
{
  string a = to_simple_string(time.time_of_day());
  a = a.substr(0, a.length()-3);
  return a;
}
std::string ConsentrationGraph::getTimeAndDate( boost::posix_time::ptime time )
{
  return to_simple_string(time.time_of_day()) + ", " + to_simple_string( time.date() );
}//std::string getTimeAndDate() const


std::string ConsentrationGraph::getDate( boost::posix_time::ptime time )
{
  string a = to_simple_string( time.date() );
  a = a.substr( 5 );
  return a;
}//getDate()


string ConsentrationGraph::getDateForGraphTitle( ptime time )
{
  string dateStr = to_simple_string( time.date() );
  dateStr = dateStr.substr( 5, 3 ); //Get the 3 letter month
  
  int dayOfMonth = time.date().day();
  
  dateStr += " ";
  dateStr += lexical_cast<string>(dayOfMonth);
  
  if( dayOfMonth == 1 || dayOfMonth == 31 ) dateStr += "^{st}";
  else if( dayOfMonth == 2 )                dateStr += "^{nd}";
  else if( dayOfMonth == 3 )                dateStr += "^{rd}";
  else                                      dateStr += "^{th}";
  
  return dateStr;
}//sstring ConsentrationGraph::getDateForGraphTitle( ptime time )



ptime ConsentrationGraph::roundDownToNearest15Minutes( ptime time, int slop )
{
  time_duration timeOfDay = time.time_of_day();
  ptime roundedTime( time.date(), time_duration( timeOfDay.hours(), 0,0,0) );
  if( timeOfDay.minutes() >= 15 ) roundedTime += time_duration( 0, 15, 0, 0 );
  if( timeOfDay.minutes() >= 30 ) roundedTime += time_duration( 0, 15, 0, 0 );
  if( timeOfDay.minutes() >= 45 ) roundedTime += time_duration( 0, 15, 0, 0 );

  ptime lostTime = time - timeOfDay;
  assert( lostTime.time_of_day() <= time_duration( 0,slop,0,0) );
  
  return roundedTime;
}//roundDownToNearest15Minutes



double ConsentrationGraph::getMostCommonDt( ) const
{
  TimeDuration dt = getMostCommonPosixDt();
  
  return toNMinutes(dt);
}//double ConsentrationGraph::getMostCommonDt( ) const



TimeDuration ConsentrationGraph::getMostCommonPosixDt() const
{
  typedef std::pair<boost::posix_time::ptime, double> TimeValuePair;
  typedef std::vector< TimeValuePair >                TimeValueVec;
  
  TimeValueVec timeValues;
  
  foreach( const GraphElement &el, static_cast< set<GraphElement> >(*this) )
  {
    ptime baseTime = getAbsoluteTime( el.m_minutes );
    timeValues.push_back( TimeValuePair(baseTime, el.m_value) );
  }//foreach( base pair )
  
  return CgmsDataImport::getMostCommonPosixDt( timeValues );
}//getMostCommonPosixDt




ConsentrationGraph 
ConsentrationGraph::getSmoothedGraph( double wavelength, 
                                      SmoothingAlgo smoothType) const
{
  ConsentrationGraph smoothedGraph( *this );
    
  switch( smoothType )
  {
    case FourierSmoothing: 
      smoothedGraph.fastFourierSmooth( wavelength );
      break;
      
    case ButterworthSmoothing:
      smoothedGraph.butterWorthFilter( wavelength, 4 );
      break;
      
    case BSplineSmoothing:
      #ifndef __GSL_BSPLINE_H__
        assert(0);
      #else
      smoothedGraph.bSplineSmoothOrDeriv( false, wavelength / 2.0, 6 );
      #endif  //#ifndef __GSL_BSPLINE_H__
      break;
      
    case NoSmoothing:
      break;
  };//switch( smoothType )
  
  return smoothedGraph;
}//ConsentrationGraph::getSmoothedGraph


ConsentrationGraph 
ConsentrationGraph::getDerivativeGraph( double wavelength, 
                                        SmoothingAlgo smoothType) const
{
  ConsentrationGraph derivGraph( *this );
    
  switch( smoothType )
  {
    case FourierSmoothing: 
      derivGraph.fastFourierSmooth( wavelength );
      derivGraph.differntiate(5);
      break;
      
    case ButterworthSmoothing:
      derivGraph.butterWorthFilter( wavelength, 4 );
      derivGraph.differntiate(5);
      break;
      
    case BSplineSmoothing:
      #ifndef __GSL_BSPLINE_H__
        assert(0);
      #else
      derivGraph.bSplineSmoothOrDeriv( true, wavelength / 2.0, 6 );
      #endif  //#ifndef __GSL_BSPLINE_H__
      break;
      
    case NoSmoothing:
      break;
  };//switch( smoothType )
  
  return derivGraph;
}//ConsentrationGraph::getDerivativeGraph


//see below link for good summary of numerical differentiation
// http://www.physics.arizona.edu/~restrepo/475A/Notes/sourcea/node39.html#dint5
void 
ConsentrationGraph::differntiate( int nPoint )
{
  removeNonInfoAddingPoints();
  
  m_yOffsetForDrawing = 0.0;
  
  assert( nPoint > 0 );
  
  if( empty() )
  {
    m_graphType = BloodGlucoseConcenDeriv;
    return;
  }//don't do anything if there is no data
  
  if( size() <= (unsigned int)nPoint )//we don't need the equal, but so what
  {
    cerr << "differntiate(): I can't take " << nPoint 
         << "-point derivative of a graph with "
         << size() << " data points" << endl;
    exit(1);
  }//if( size() <= nPoint )
  
  const double h = getMostCommonDt();
  const double t0 = begin()->m_minutes;
  const double tEnd = getOffset( getEndTime() );
  
  if( (nPoint != 3) && (nPoint!=5) )
  {
    cerr << "You can only choose nPoint=3 or 5 when"
         << " calling differntiate( nPoint)" << endl;
    exit(1);
  }//if( (nPoint != 3) && (nPoint!=5) )
  
  if( m_graphType != GlucoseConsentrationGraph )
  {
    cerr << "differentiation isn't allowed for grapgsof type "
         << getGraphTypeStr() << endl;
    exit(1);
  }//if( we shouldn't take the derivative )
    
  //Change the type of the graph
  m_graphType = BloodGlucoseConcenDeriv;
    
  GraphElementSet theDerivs;
  
  const double h0 = (nPoint == 3) ? h : 2.0*h;
  double deriv = -999.9;
  
  //Okay now lets put in derivative of the first point
  if( nPoint == 3 )
  {
    deriv = (-3.0*value(t0) + 4.0*value(t0+h) - value(t0+2.0*h)) / (2.0 * h);
  }else if( nPoint == 5 )
  {
    deriv = ( -25.0*value(t0) + 48.0*value(t0+h) - 36.0*value(t0+2.0*h)
              + 16.0*value(t0+3.0*h) - 3.0*value(t0+4*h)) / (12.0*h);
  }else assert(0);
  
  theDerivs.insert( GraphElement( t0, deriv ) );
  
  //Okay, now loop over the time between the start and end times
  for( double t = t0+h0; t <= (tEnd-h); t += h0 )
  {
    if( nPoint == 3 )
    {
      deriv = ( value(t + h) - value(t - h) ) / (2.0*h);
    }else if( nPoint == 5 )
    {
      deriv = ( value(t-(2.0*h)) - 8.0*value(t-h) 
                + 8.0*value(t+h) - value(t+(2.0*h)) ) / (12.0*h);
    }else assert(0);
    
    theDerivs.insert( GraphElement( t, deriv ) );
  }//for( loop over all times of graph, except endpoints P
  
  
  //Okay now lets put in derivative of the last point, 
  //  we have to use 3-point formula
  deriv = (value(tEnd-2.0*h) - 4.0*value(tEnd-h) + 3.0*value(tEnd)) / (2.0 * h);
  theDerivs.insert( GraphElement( tEnd, deriv ) );
    
  //remove all the current data points
  clear();
  
  foreach( const GraphElement &e, theDerivs ) insert( e.m_minutes, e.m_value );
  
}//differntiate( int )



double 
ConsentrationGraph::bSplineSmoothOrDeriv(  bool takeDeriv, 
                                           double knotDist, int splineOrder )
{
#ifndef __GSL_BSPLINE_H__
    cerr << "You may not call bSplineSmoothOrDeriv(...) unless your using GSL" 
         << endl;
    assert(0);
    if( takeDeriv ) knotDist = (double)splineOrder; //Just to keep from geting warnings
    return 0.0;
#endif
  const double readingUncert = ModelDefaults::kCgmsIndivReadingUncert;
  const double graphDuration = getOffset(getEndTime()) - getOffset(getT0());
  
  const size_t n = size();
  const size_t nKnots = ceil( graphDuration / knotDist );
  const size_t nbreak = nKnots + 2 - splineOrder;
       
  gsl_rng_env_setup();
  gsl_rng *r = gsl_rng_alloc(gsl_rng_default);
  gsl_bspline_workspace *bw = gsl_bspline_alloc(splineOrder, nbreak);
  gsl_vector *B = gsl_vector_alloc(nKnots);
  gsl_vector *x = gsl_vector_alloc(n);
  gsl_vector *y = gsl_vector_alloc(n);
  gsl_matrix *X = gsl_matrix_alloc(n, nKnots);
  gsl_vector *c = gsl_vector_alloc(nKnots);
  gsl_vector *w = gsl_vector_alloc(n);
  gsl_matrix *cov = gsl_matrix_alloc(nKnots, nKnots);
  gsl_multifit_linear_workspace *mw = gsl_multifit_linear_alloc(n, nKnots);
  const size_t nCoef = gsl_bspline_ncoeffs( bw );
  gsl_matrix *dB = gsl_matrix_alloc( nCoef, 2);
  gsl_vector *derivs = gsl_vector_alloc( nCoef );
  gsl_bspline_deriv_workspace *derivWS = gsl_bspline_deriv_alloc(splineOrder);

  
  size_t position = 0;
  ConstGraphIter iter;
  for( iter = begin(); iter != end(); ++iter )
  {
    double weight = 1.0 / pow( readingUncert*iter->m_value, 2 );
    gsl_vector_set(x, position, iter->m_minutes);
    gsl_vector_set(y, position, iter->m_value);
    gsl_vector_set(w, position, weight);
    ++position;
  }//for( loop over base points )
  
  // use uniform breakpoints
  gsl_bspline_knots_uniform(getOffset(getT0()), getOffset(getEndTime()), bw);
     
  // construct the fit matrix X 
  for( size_t i = 0; i < n; ++i )
  {
    double xi = gsl_vector_get(x, i);
    gsl_bspline_eval(xi, B, bw); //compute B_j(xi) for all j
     
    for( size_t j = 0; j < nKnots; ++j) //fill in row i of X
    {
      double Bj = gsl_vector_get(B, j);
      gsl_matrix_set(X, i, j, Bj);
    }//for( loop over the coeficients )
  }//for( loop over the time points )
     
  /* do the fit */
  double chisq = -999;
  const double dof = n - nKnots;
  gsl_multifit_wlinear(X, w, y, c, cov, &chisq, mw);
  
    
  const double dt = getMostCommonDt();
  const double t0 = begin()->m_minutes;
  const double tEnd = getOffset(getEndTime());
  
  clear(); //removeall data points
  
  if( takeDeriv )
  {
    m_yOffsetForDrawing = 0.0;
    insert( t0, 0.0 );
    for( double time = t0 + 0.5*dt; time <= (tEnd - 0.5*dt); time += dt)
    {
      double deriv = 0.0; //lets avearge the derivative over the 
      
      for( int i=-5; i<=4; ++i )
      {  
        double dydt = 0.0, error=0.0;
        double t = time + dt * i/10.0;
        gsl_bspline_deriv_eval(t, 1, dB, bw, derivWS);
        gsl_matrix_get_col(derivs, dB, 1);
        gsl_multifit_linear_est(derivs, c, cov, &dydt, &error);
        deriv += dydt;
      }//for( loop over the sub times )
      
      insert( time, deriv/10.0 );
    }//for( loop over time this graph is defined for )
    
    //need to catch last point now
    double dydt = 0.0, error=0.0;
    gsl_bspline_deriv_eval(tEnd, 1, dB, bw, derivWS);
    gsl_matrix_get_col(derivs, dB, 1);
    gsl_multifit_linear_est(derivs, c, cov, &dydt, &error);
    insert( tEnd, dydt );
  }else
  {
    for( double time = t0; time <= tEnd; time += dt)
    {
      double val = 0.0, valErr= 0.0;
      gsl_bspline_eval(time, B, bw);
      gsl_multifit_linear_est(B, c, cov, &val, &valErr);
      insert( time, val );
    }//for( loop over time this graph is defined for )
  }//if( takeDeriv ) / else
  
  gsl_rng_free(r);
  gsl_vector_free(x);
  gsl_vector_free(y);
  gsl_matrix_free(X);
  gsl_vector_free(w);
  gsl_multifit_linear_free(mw);
  gsl_matrix_free(cov);
  gsl_matrix_free(dB);
  gsl_vector_free(B);
  gsl_vector_free(c);
  gsl_vector_free(derivs);
  gsl_bspline_free(bw);
  gsl_bspline_deriv_free( derivWS );
  
  
  if( takeDeriv )
  {
    if( m_graphType != GlucoseConsentrationGraph )
    {
      cerr << "differentiation isn't allowed for grapgsof type "
           << getGraphTypeStr() << endl;
      exit(1);
    }//if( we shouldn't have taken the derivative )
    
    m_graphType = BloodGlucoseConcenDeriv;
  }//if( takeDeriv )
  
  return chisq / dof;
}//bSplineDerivative






void ConsentrationGraph::fastFourierSmooth( double lambda_min, double time_window )
{ 
  removeNonInfoAddingPoints();
 
  if( empty() ) return;
  
  GraphElementSet xFormResult;
  
  //Since this is primarily intended for CGMS readings, using a dt of the 
  //  most common time would be adequate.
  const double dt = getMostCommonDt();
  const double freq_max = dt / lambda_min;
  
  const double t0 = this->begin()->m_minutes;
  const double totalTime = getOffset(getEndTime()) - t0;
 
  // if((time_window > totalTime) || (time_window <= 0.0))
  if(time_window >= 0.0) cout << "I am ignoring your time_window request" << endl;
  
  time_window = totalTime;

  const int nPoints = floor( totalTime / dt ); //we might miss a litle info
  int pointsPerWindow = floor( time_window / dt );
  const int nWindows = nPoints / pointsPerWindow;  
    
  double *xAxis = new double[pointsPerWindow];
  double *input = new double[pointsPerWindow];
  double *xformed_real = new double [pointsPerWindow];
  double *xformed_imag = new double [pointsPerWindow];
 
  
  for( int window=0; window < nWindows; ++window )
  {
    for( int point = 0; point < pointsPerWindow; ++point )
    {
      const unsigned int discreetT = point + window*pointsPerWindow;
      const double t = t0 + discreetT*dt;
      xAxis[point] = t;
      input[point] = value(t);
    }//for
    
    
    TVirtualFFT *fft_own = TVirtualFFT::FFT(1, &pointsPerWindow, "R2C");
    fft_own->SetPoints(input);
    fft_own->Transform();
    fft_own->GetPointsComplex(xformed_real, xformed_imag);

    //Below is just a guess as to how to filter off unwanted frequencies
    int highestBin = freq_max * pointsPerWindow;
    // cout << "removing bins>=" << highestBin << endl;
    for( int point = highestBin; point < pointsPerWindow; ++point )
    {
      xformed_real[point] = 0.0;
      xformed_imag[point] = 0.0;
    }
    
    TVirtualFFT *fft_back = TVirtualFFT::FFT(1, &pointsPerWindow, "C2R");
    fft_back->SetPointsComplex(xformed_real, xformed_imag);
    fft_back->Transform();
    fft_back->GetPoints(input);

    for( int point = 0; point < pointsPerWindow; ++point )
    {
      double val = input[point]/pointsPerWindow;
      xFormResult.insert( GraphElement( xAxis[point], val ) );
    }//
  }//for( loop over the time of the graph )
  
  // delete fft_own;
  // delete fft_back;
  delete [] xAxis;
  delete [] input;
  delete [] xformed_real;
  delete [] xformed_imag;
 
  std::set<GraphElement>::erase( begin(), end() );
  std::set<GraphElement>::insert( xFormResult.begin(), xFormResult.end() );
}//void fastFourierSmooth( double ohmega_min = 20.0, double time_window = 180.0 )



//Butterworth filter adapter from code found at:
//  http://pagesperso-orange.fr/jean-pierre.moreau/Cplus/tfilters_cpp.txt
//Calc Butterworth coef
void ConsentrationGraph::calcButterworthCoef(double cuttOffFrequ, double samplingTime, 
                                     int filterOrder, Filter_Coef C,
                                     int *NSections, double *groupDelay )  
{
  const double Pi = M_PI; //3.1415926535;
  
  *groupDelay=0.0;
  
  double Arg = Pi * samplingTime * cuttOffFrequ;
  
  if( fabs(samplingTime * cuttOffFrequ) > 2.0 ) 
  {
    Arg -= 2.0 * Pi * floor(samplingTime * cuttOffFrequ/2.0);
    cout << "Butterworth(...): You can't filter at a frequency higher than 1/2"
         << " the sampling freuency, instead I will filter at" 
         << Arg / Pi / samplingTime << "Hz" << endl;
  }//if( asking to filter something higher than one-half the sampling frequency )
  
  const double Omega = tan( Arg );   
  const double OmegaSq = Omega * Omega;
  const double temp = (filterOrder%2) ? 0.0 : 0.5;
  
  int Ns2 = filterOrder/2;
  *NSections = Ns2 + (filterOrder % 2);
  
  if (filterOrder>1) 
  {
    for( int i=1; i<Ns2+1; i++ ) 
    {
      double Rep = Omega * cos( Pi*(i-temp)/filterOrder );
      *groupDelay = *groupDelay + (samplingTime * Rep) / OmegaSq;
      const double W0=2.0*Rep;
      const double W1=1.0 +W0+OmegaSq;
      C[1][i] = -2.0*(OmegaSq-1.0)/W1;
      C[2][i] = -(1.0-W0+OmegaSq)/W1;
      C[3][i] = 2.0;
      C[4][i] = 1.0;
      C[5][i] = OmegaSq/W1;
    }
    
    if( temp == 0.0 ) 
    {
      C[1][*NSections]=(1.0-Omega)/(1.0+Omega);
      C[2][*NSections]= 0.0;
      C[3][*NSections]= 1.0;
      C[4][*NSections]= 0.0;
      C[5][*NSections]= Omega/(1.0+Omega);
      *groupDelay= *groupDelay+samplingTime/(2.0*Omega);
    }//if( 1st or 3rd order filter )
  }//if (filterOrder>1) 
} // Butterworth()


/**********************************************************************
*          Filtering a signal F(t) by Butterworth method              *
*             (removing frequencies greater then Fc)                  *
* ------------------------------------------------------------------- */
void ConsentrationGraph::butterworthFilter( double *Xs,double Xd, int NSections,
 	                                          Filter_Coef C, Memory_Coef D )  
{
  double x = Xd;
  
  for( int i=1; i<NSections+1; i++) 
  {
    double err = x+C[1][i]*D[1][i]+C[2][i]*D[2][i];
    double y = C[5][i]*(err +C[3][i]*D[1][i]+C[4][i]*D[2][i]);
    D[2][i]=D[1][i];
    D[1][i]=err;
    x=y;
  }
  
  *Xs=x;
}//butterworthFilter


//INIT FILTER PROCEDURE
void ConsentrationGraph::butterworthInit( double Xdc, Filter_Coef C, 
                                          int NSections, Memory_Coef D ) 
{
  double dc=Xdc;
  for( int j=1; j<NSections+1; j++) 
  {
    D[2][j]=dc/(1-C[1][j]-C[2][j]);
    D[1][j]=D[2][j];
    
    double Csum=0;
    for( int i=1; i<5; i++) Csum = Csum + C[i][j];
    
    dc = C[5][j] * (dc+D[2][j]*Csum);
  } //j loop
} // butterworthInit()



void ConsentrationGraph::butterWorthFilter( double timescale, int filterOrder )
{
  removeNonInfoAddingPoints();
 
  if( empty() ) return;
  
  GraphElementSet xFormResult;
  
  const double dt = getMostCommonDt();
  const double t0 = this->begin()->m_minutes;
  const double totalTime = getOffset(getEndTime()) - t0;
  const int totalPoints = totalTime / dt + 1;
  
  double *input = new double[totalPoints+1];
  
  for( int point = 0; point <= totalPoints; ++point )
  {
    const double t = t0 + point*dt;
    input[point] = value(t);
  }//for
  
  //input cut off frequencys and order of filter (1 to 4)
  double cuttOffFreq = 1.0 / timescale; 
  Filter_Coef C;  //determines how the filter filters
  Memory_Coef D;  //to storeinfo for use in butterworthFilter(...)
  int      NSections; //Number of required 2nd order sections (integer)
  double   timeDelay; //The butter worth filter introduces a timedelay we must corect for
  
  
  // 1. Calculate the filter coefficients
  calcButterworthCoef( cuttOffFreq, dt, filterOrder, C, &NSections, &timeDelay);

  // 2. Initialize filter memory
  butterworthInit( value(t0), C, NSections, D);

  // 3. Recursively call Butterworth filter
  for( int point=0; point<=totalPoints; ++point) 
  {
    double val = 0.0;
    const double t = t0 + point*dt;
    butterworthFilter(&val, input[point], NSections, C, D);
    xFormResult.insert( GraphElement( t - timeDelay, val ) );
  }//for( loop over data points )
  
  delete [] input;
 
  std::set<GraphElement>::erase( begin(), end() );
  std::set<GraphElement>::insert( xFormResult.begin(), xFormResult.end() );
}//




TGraph *ConsentrationGraph::getTGraph( boost::posix_time::ptime t_start,
                                       boost::posix_time::ptime t_end  ) const
{
  unsigned int nPoints = size(); //(((int)duration) / 1);
  
  double *xAxis = new double[nPoints]; 
  double *yAxis = new double[nPoints];

  vector< pair<double,string> > newDayLabel;
  vector< pair<double,string> > every60MinuteLabel;
  
  //Start the graph at the nearest 15min mark before first value
  const ptime firstValuesTime = getAbsoluteTime( begin()->m_minutes );
  time_duration timeOfDay = firstValuesTime.time_of_day();
  
  ptime previousLabelTime =  roundDownToNearest15Minutes( firstValuesTime );
  
  //So we can clearly label new days
  //  This is complicated by the fact the input data might not exactly
  //  stradel midnight
  gregorian::date currentDate = firstValuesTime.date();
  double offsetToFirstMidnight = -999;
  
  nPoints = 0;
  foreach( const GraphElement &el, static_cast<GraphElementSet>(*this) )
  {
    const ptime t = getAbsoluteTime( el.m_minutes );
    
    if( t_start != kGenericT0 && t < t_start ) continue;
    if( t_end   != kGenericT0 && t > t_end   ) continue;
    
    xAxis[nPoints] = el.m_minutes;
    yAxis[nPoints] = el.m_value + m_yOffsetForDrawing;
    ++nPoints;
    
    ptime currentTime = getAbsoluteTime( el.m_minutes );
    
    if( (currentTime - previousLabelTime) >= minutes(60) )
    {
      previousLabelTime = currentTime;
      
      int slop = (int)m_dt + 1;
      ptime labelsVal = roundDownToNearest15Minutes( previousLabelTime, slop );
      
      every60MinuteLabel.push_back( make_pair(el.m_minutes, getTimeNoDate(labelsVal)) );
    }//15 minutes have elapsed
    
    if( currentDate != currentTime.date() )
    {
      if( offsetToFirstMidnight < 0.0 ) offsetToFirstMidnight = el.m_minutes;
        
      //get number of days that have elapsed
      gregorian::date_duration dd = currentTime.date() - firstValuesTime.date() - gregorian::days(1);
      double nMinutesToThisMidnight = offsetToFirstMidnight + 1440*dd.days();
      
      newDayLabel.push_back( make_pair(nMinutesToThisMidnight, getDate(currentTime)) );
      currentDate = currentTime.date();
    }//if( same day as last point ) / else
  }//for( loop over data points )
  
  TGraph *graph = new TGraph( nPoints, xAxis, yAxis );
  
  if( nPoints < 4 ) return graph;
  
  //Figure out how sparce to make the labels
  size_t nLabelSkip = 1;
  
  if( every60MinuteLabel.size() > 15 ) 
    nLabelSkip = every60MinuteLabel.size() / 15;
  
  const int nBins = graph->GetXaxis()->GetNbins();
  
  if( newDayLabel.size() < 4 )
  {
    for( size_t i = 0; i < every60MinuteLabel.size(); i += nLabelSkip )
    {
      pair<double,string> label = every60MinuteLabel[i];
      int bin = graph->GetXaxis()->FindBin( label.first );
      if( bin <= nBins ) graph->GetXaxis()->SetBinLabel( bin, label.second.c_str() );
    }//foreach 15 minute label
  }//if( graph is less than three days long )
  
  for( size_t i=0; i<newDayLabel.size(); ++i )
  {
    pair<double,string> label = newDayLabel[i];
    int bin = graph->GetXaxis()->FindBin( label.first );
    if( bin <= nBins ) graph->GetXaxis()->SetBinLabel( bin, label.second.c_str() );
  }//foreach 15 minute label
  
  // cout << "Axis has " << graph->GetXaxis()->GetNbins() << " bins" << endl;
  
  graph->GetXaxis()->CenterLabels(kFALSE);
  
  ptime lastTime = getAbsoluteTime( (--end())->m_minutes );
  
  string graphTitle = ", From " + getDateForGraphTitle( m_t0 ) 
                      + " Through " + getDateForGraphTitle( lastTime ) ;
  
  if( m_t0.date() == lastTime.date() ) graphTitle = ", " + getDateForGraphTitle( m_t0 );
  
  
  switch( m_graphType )
  {
    case InsulinGraph: 
      graph->GetYaxis()->SetTitle( "Plasma Insulin Concentration (mU/L)" ); 
      graphTitle = "Plasma Insulin Concentration" + graphTitle;
    break;
    
    case BolusGraph:             
      graph->GetYaxis()->SetTitle( "Insulin Boluses (U)" ); 
      graphTitle = "Insulin Boluses" + graphTitle;
    break;
    
    case GlucoseConsentrationGraph:
      graph->GetYaxis()->SetTitle( "Blood Glucose Concentration (mg/dL)" );
      graphTitle = "Blood Glucose Concentration" + graphTitle;
    break;
    
    case GlucoseAbsorbtionRateGraph:
      graph->GetYaxis()->SetTitle( "Glucose Absorption Rate (mg/dL/min)" ); 
      graphTitle = "Glucose Absorption Rate" + graphTitle;
    break; 
    
    case GlucoseConsumptionGraph:
      graph->GetYaxis()->SetTitle( "Carbs Consumed (g)" ); 
      graphTitle = "Carbohydrates Consumed" + graphTitle;
    break;
    
    case BloodGlucoseConcenDeriv:
      graph->GetYaxis()->SetTitle( "#frac{dG^{blood}}{dt} (mg/dl/min)" ); 
      // graph->GetYaxis()->SetTitleOffset(0.1);
      graphTitle = "Change in Glucose Concentration" + graphTitle;
    case NumGraphType:
      graph->GetYaxis()->SetTitle( "" ); 
    break;
  }//switch( m_graphType )
  
  
  graph->SetTitle( graphTitle.c_str() );
  
  
  double *maxHeight = max_element( yAxis+0, yAxis+nPoints );
  double *minHeight = min_element( yAxis+0, yAxis+nPoints );
  graph->SetMaximum( *maxHeight );
  graph->SetMinimum( *minHeight );
  
  delete [] xAxis;
  delete [] yAxis;
  
  return graph;
}//getTGraph




void ConsentrationGraph::setYOffset( double yOffset )
{
  m_yOffsetForDrawing = yOffset;
}//setYOffsetForDrawing

double ConsentrationGraph::getYOffset() const {return m_yOffsetForDrawing;}


//Forms a TGraph and draws on current active TPad (gPad)
TGraph* ConsentrationGraph::draw( string options, 
                               string title, bool pause, int color ) const
{  
  if( title == "" ) title = "ConsentrationGraph";
  
  if( !size() )
  {
    cout << "You tried to draw an empty graph, ignoring request" << endl;
    return (TGraph *)NULL;
  }//if( !size() )
  
  
  assert(gTheApp);
  // Int_t dummy_arg = 0;
  // if( !gTheApp ) gTheApp = new TApplication("App", &dummy_arg, (char **)NULL);
  
  if( !gPad ) new TCanvas();
  
  gPad->SetTitle( title.c_str() );
  
  TGraph *graph = getTGraph();  
  
  if( color > 0 ) graph->SetLineColor( color );
  
  
  switch( m_graphType )
  {
    case InsulinGraph: 
    break;
    
    case BolusGraph:             
      if( options == "" ) options += "A*";
    break;
    
    case GlucoseConsentrationGraph:
    break;
    
    case GlucoseAbsorbtionRateGraph:
    break; 
    
    case GlucoseConsumptionGraph:
      if( options == "" ) options += "A*";
    break;
    
    case BloodGlucoseConcenDeriv:
    break;
    
    case NumGraphType:
    break;
  }//switch( m_graphType )
  

  if( options == "" ) options += "Al";
  graph->Draw( options.c_str() );
  
  if( pause ) 
  {
    gTheApp->Run(kTRUE);
    delete gPad;
    delete graph;
    
    return (TGraph *)NULL;
  }//if( pause )
  
  return graph;
}//draw(...)
    


template<class Archive>
void ConsentrationGraph::serialize( Archive &ar, const unsigned int version )
{
  unsigned int ver = version; //keep compiler from complaining
  ver = ver;
      
  ar & m_t0;
  ar & m_dt;
  ar & m_yOffsetForDrawing;
  ar & m_graphType;
  ar & boost::serialization::base_object< std::set<GraphElement> >(*this);
}//serialize



ConsentrationGraph ConsentrationGraph::loadFromFile( std::string filename )
{
  
  if( filename == "" )
  {
    ConsentrationGraph *newGraph = NULL;
    new CreateGraphGui( newGraph, gClient->GetRoot(), gClient->GetDefaultRoot() );
    
    if( !newGraph )
    {
      cout << "Error opening file; Exiting now" << endl;
      exit(-1);
    }//if( !newGraph )
    
    ConsentrationGraph tempGraph( *newGraph );
    delete newGraph;
    
    return tempGraph;
  }//if( you didn't specify a name )
  
  std::ifstream ifs( filename.c_str() );
  if( !ifs.is_open() )
  {
    cout << "Couldn't open file " << filename << " for reading" << endl;
    exit(-1);
  }//if( !ofs.is_open() )
  
  boost::archive::text_iarchive ia(ifs);
  ConsentrationGraph tempGraph( kGenericT0, 0.0, NumGraphType);
  // restore the schedule from the archive
  ia >> tempGraph;
  
  return tempGraph;
}//loadFromFile


bool ConsentrationGraph::saveToFile( std::string filename )
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



AbsFuncPointer ConsentrationGraph::getFunctionPointer( 
                                             AbsorbtionFunction absFunc ) const
{
  switch( absFunc )
  {
    case NovologAbsorbtion:                return &NovologConsentrationFunc;
    case FastCarbAbsorbtionRate:           return &FastCarbAbsorbtionFunc;
    case MediumCarbAbsorbtionRate:         return &MediumCarbAbsorbtionFunc;
    case SlowCarbAbsorbtionRate:           return &SlowCarbAbsorbtionFunc;
    case NumAbsorbtionFunctions:
    {
      switch( m_graphType )
      {
        case InsulinGraph:                 return &NovologConsentrationFunc;
        case GlucoseConsentrationGraph:    assert(0); return NULL;
        case GlucoseAbsorbtionRateGraph:   return &MediumCarbAbsorbtionFunc;
          
        case BolusGraph:
        case GlucoseConsumptionGraph:
        case BloodGlucoseConcenDeriv:
        case NumGraphType:
        assert(0);
      };//switch( m_graphType )
    };//case NumAbsorbtionFunctions:
  };//switch( absFunc )
  
  assert(0);
  return NULL; //keep compiler from complaining
}//getFunctionPointer


    





