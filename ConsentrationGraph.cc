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


//Boost includes (using boost 1.38)
#include "boost/range.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/foreach.hpp"
#include "boost/assign/list_of.hpp" //for 'list_of()'
#include "boost/assign/list_inserter.hpp"
#include "boost/lexical_cast.hpp"

#include "ConsentrationGraph.hh"
#include "KineticModels.hh"


using namespace std;
using namespace boost;
using namespace boost::posix_time;

extern TApplication *gTheApp;

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
  time_duration td = t1 - t0;
  //I'm sure theres a better wasy to do this, but I don't have boost
  //  documentation in front of me
  const int nSecondsTotal = td.total_seconds();
 
  const double nMinutes = nSecondsTotal / 60.0;
  
  return nMinutes;
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
    case NumGraphType:               return "NumGraphType";
  };//switch( m_graphType )
  
  assert(0);
  return "";
}//getGraphTypeStr

double ConsentrationGraph::getDt() const { return m_dt; }
boost::posix_time::ptime ConsentrationGraph::getT0() const { return m_t0; }


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
  long mins = (long) nOffsetMinutes;
  nOffsetMinutes -= mins;
  nOffsetMinutes *= 60.0;
  long secs = (long)nOffsetMinutes;
  
  time_duration td = minutes(mins) + seconds(secs);
  
  return m_t0 + td;
}//ptime getAbsoluteTime( double nOffsetMinutes ) const;



bool ConsentrationGraph::containsTime( ptime absoluteTime ) const
{
  return containsTime( getOffset(absoluteTime) );
}//containsTime(...)


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
  if( !size() ) return 0.0;
  if( nOffsetminutes < 0.0 ) return 0.0;
  
  //upper_bound returns element right past where 'nOffsetminutes' would be
  ConstGraphIter lowerBound = GraphElementSet::lower_bound( GraphElement(nOffsetminutes, 0.0) );
  
  if( lowerBound == GraphElementSet::end() )    return 0.0;
  if( lowerBound->m_minutes == nOffsetminutes ) return lowerBound->m_value;
  
  //Also BolusGraph is a discreet graph, so don't return a value
  //  unless time matches exactly, which we already would have
  if( m_graphType == BolusGraph ) return 0.0;
  
  ConstGraphIter prevElement = --lowerBound;
  
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



TGraph *ConsentrationGraph::getTGraph() const
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
  
  //Figure out how sparce to make the labels
  size_t nLabelSkip = 1;
  
  if( every60MinuteLabel.size() > 15 ) 
    nLabelSkip = every60MinuteLabel.size() / 15;
  
  if( newDayLabel.size() < 4 )
  {
    for( size_t i = 0; i < every60MinuteLabel.size(); i += nLabelSkip )
    {
      pair<double,string> label = every60MinuteLabel[i];
      int bin = graph->GetXaxis()->FindBin( label.first );
      graph->GetXaxis()->SetBinLabel( bin, label.second.c_str() );
    }//foreach 15 minute label
  }//if( graph is less than three days long )
  
  for( size_t i=0; i<newDayLabel.size(); ++i )
  {
    pair<double,string> label = newDayLabel[i];
    int bin = graph->GetXaxis()->FindBin( label.first );
    graph->GetXaxis()->SetBinLabel( bin, label.second.c_str() );
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




void ConsentrationGraph::setYOffsetForDrawing( double yOffset )
{
  m_yOffsetForDrawing = yOffset;
}//setYOffsetForDrawing



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
  
  
  Int_t dummy_arg = 0;
  if( !gTheApp ) gTheApp = new TApplication("App", &dummy_arg, (char **)NULL);
  
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
    

ConsentrationGraph ConsentrationGraph::loadFromFile( std::string filename )
{
  std::ifstream ifs( filename.c_str() );
  if( !ifs.is_open() )
  {
    cout << "Couldn't open file " << filename << " for reading" << endl;
    exit(1);
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
        case NumGraphType:
        assert(0);
      };//switch( m_graphType )
    };//case NumAbsorbtionFunctions:
  };//switch( absFunc )
  
  assert(0);
  return NULL; //keep compiler from complaining
}//getFunctionPointer


    








