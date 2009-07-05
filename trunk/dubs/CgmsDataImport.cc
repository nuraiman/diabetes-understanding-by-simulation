#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "boost/range.hpp"
#include "boost/foreach.hpp";
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/exception/exception.hpp"
#include "boost/format.hpp"

#include "CgmsDataImport.hh"
#include "RungeKuttaIntegrater.hh" //for toNMinutes()

using namespace std;
using namespace boost;
using namespace boost::posix_time;

//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH





ConsentrationGraph 
CgmsDataImport::importSpreadsheet( string filename, InfoType type, 
                                   ptime startTime, ptime endTime )
{
  IndexMap indMap;
  TimeValueVec timeValues;
  SpreadSheetSource source = NumSpreadSheetSource;
  const char eolChar = getEolCharacter( filename );
  ifstream inputFile( filename.c_str() );
  
  if( !inputFile.is_open() )
  {
    cout << "Could not open '" << filename << "' please fix this." << endl;
    exit(1);
  }//if( !inputFile.is_open() 
  
  string currentLine;
  
  while( !inputFile.eof() )
  {
    getline( inputFile, currentLine, eolChar );
    algorithm::trim(currentLine);
    if( currentLine.size() == 0 ) continue;
	  if( currentLine[0] == '#' ) continue;
	  if( currentLine.find_first_not_of(", ") == string::npos ) continue;
    
    // cout << "On Line: " << currentLine << endl;
    
    if( indMap.empty() )
    {
      indMap = getHeaderMap( currentLine );
      if( !indMap.empty() )  source = getSpreadsheetTypeFromHeader(currentLine);
    }else
    {
      assert( source != NumSpreadSheetSource );
      TimeValuePair lineinfo = getInfoFromLine( currentLine, indMap, type, source );
      
      if( lineinfo.second != kFailValue ) timeValues.push_back( lineinfo );
    }//if( indMap.empty() ) / else
  }//while( !inputFile.eof() )
    
  GraphType graphType = NumGraphType;
  switch(type)
  {
    case CgmsReading:       graphType = GlucoseConsentrationGraph; break;
    case MeterReading:      graphType = GlucoseConsentrationGraph; break;
    case MeterCalibration:  graphType = GlucoseConsentrationGraph; break;
    case GlucoseEaten:      graphType = GlucoseConsumptionGraph;   break;
    case BolusTaken:        graphType = BolusGraph;                break;
    case ISig:              graphType = NumGraphType;              break;
    
    assert(0);
  };//switch(type)
  
  double dt = 5.0;
 
  if( indMap.empty() ) cout << "Couldn't decode file " << filename << endl;
    
  assert( !indMap.empty() );
  
  if( timeValues.empty() ) return  ConsentrationGraph( startTime, dt, graphType );

  //Now create a ConsentrationGraph
  ptime graphStarTime = startTime;
  if( graphStarTime == kGenericT0 ) graphStarTime = timeValues.begin()->first;
  
  dt = getMostCommonDt( timeValues );
  // cout << "importSpreadsheet(..): found a mode dt of " << dt << endl;
  
  ConsentrationGraph returnAswer( graphStarTime, dt, graphType );
  
  for( size_t pairnum = 0; pairnum < timeValues.size(); ++pairnum )
  {
    const TimeValuePair &pairInfo = timeValues[pairnum];
    bool timeOkay = true;
    const ptime thisTime   = pairInfo.first;
    const double thisValue = pairInfo.second;
    
    // cout << "On " << to_simple_string(thisTime) << "  " << thisValue << endl;
    
    if( (startTime != kGenericT0) && (thisTime <= startTime) ) timeOkay = false;
    if( (endTime   != kGenericT0) && (thisTime >= endTime) ) timeOkay = false;

    if( timeOkay ) returnAswer.insert( thisTime, thisValue );
  }//foreach( point we read in )
  
  return returnAswer;
}//importSpreadsheet




double CgmsDataImport::getMostCommonDt( const TimeValueVec &timeValues )
{
  const TimeDuration dt = getMostCommonPosixDt(timeValues);
  
  return toNMinutes(dt);
}//getMostCommonDt



TimeDuration CgmsDataImport::getMostCommonPosixDt( const TimeValueVec &timeValues )
{
  if( timeValues.empty() ) return TimeDuration(0,5,0,0);
  
  double dt = 0.0;
  
  //Now get mode-average seperation time of measurements
  vector<time_duration> timeDeltas;
  ptime lastTime = timeValues.begin()->first;
  
  foreach( const TimeValuePair &pairInfo, timeValues )
  {
    time_duration thisDelta = pairInfo.first - lastTime;
    timeDeltas.push_back( thisDelta );
    lastTime = pairInfo.first;
  }//foreach( point we read in )
  
  timeDeltas.erase( timeDeltas.begin() ); //remove first element
  
  sort( timeDeltas.begin(), timeDeltas.end() );
  
  size_t largestSize = 0;
  vector<time_duration>::iterator vend = timeDeltas.end();
  vector<time_duration>::iterator curentPos = timeDeltas.begin();
  vector<time_duration>::iterator startOfLargestSize = vend;
  
  while( curentPos != vend )
  {
    vector<time_duration>::iterator ub = upper_bound( curentPos, vend, *curentPos );
    const size_t length = ub - curentPos;
    
    if( length > largestSize )
    {
      largestSize = length;
      startOfLargestSize = curentPos;
    }//if( new most number of elements )
    
    curentPos = ub;
  }//while( curentPos != vend )
  
  time_duration td_dt;
  if( largestSize == 1 ) //take mediun
  {
    td_dt = timeDeltas[ timeDeltas.size() / 2 ];
  }else
  {
    assert( startOfLargestSize != vend );
    td_dt = *startOfLargestSize;
  }
  
  dt = td_dt.total_microseconds() / 1000000.0 / 60.0;
       
  return td_dt;
}//getMostCommonPosixDt



//Pass in the header row of the spreadsheet, so we know which column 
//  contains which information.  If non-header passed in, returns empty map
CgmsDataImport::IndexMap 
CgmsDataImport::getHeaderMap( std::string line )
{
  IndexMap csvIndexMap;
  IndexMap properKeyedMap;
  
  SpreadSheetSource source = getSpreadsheetTypeFromHeader( line );
  
  //if( this wasn't a header line of text )
  if( source == NumSpreadSheetSource ) return properKeyedMap;
  
  algorithm::trim(line);
  
  vector<string> feilds;
  algorithm::split( feilds, line, algorithm::is_any_of(",") );
  
  if( feilds.size() < 2 ) //It was a dexcom file
  {
    boost::algorithm::split( feilds, line, boost::algorithm::is_any_of(" \t") );
    if( feilds.size() > 1 ) assert( source == Dexcom7Csv );
    else                    return csvIndexMap; //didn't find anything useful
  }//if( feilds.size() < 2 ) 
  
  
  int fieldNumber = 0;
  foreach( string thisField, feilds )
  {
	  algorithm::trim( thisField );
	  csvIndexMap[thisField] = fieldNumber++;
  }//foeach( field )
  
  switch( source )
  {
    case MiniMedCsv: case NumSpreadSheetSource:
      if( csvIndexMap.find( kMeterBgKeyMM ) != csvIndexMap.end() )
        properKeyedMap[kMeterBgKey]     = csvIndexMap[kMeterBgKeyMM]; 
      else  if( csvIndexMap.find( kMeterBgKey ) != csvIndexMap.end() )
        properKeyedMap[kMeterBgKey]     = csvIndexMap[kMeterBgKey]; 
      
      if( csvIndexMap.find( kCalibrationKeyMM ) != csvIndexMap.end() )
        properKeyedMap[kCalibrationKey] = csvIndexMap[kCalibrationKeyMM];
      
      if( csvIndexMap.find( kDateKey ) != csvIndexMap.end() )
        properKeyedMap[kDateKey]        = csvIndexMap[kDateKey];
      
      if( csvIndexMap.find( kTimeKey ) != csvIndexMap.end() )
        properKeyedMap[kTimeKey]        = csvIndexMap[kTimeKey];
      
      if( csvIndexMap.find( kBolusKeyMM ) != csvIndexMap.end() )
        properKeyedMap[kBolusKey]       = csvIndexMap[kBolusKeyMM];
      else if( csvIndexMap.find( kBolusKey ) != csvIndexMap.end() )
        properKeyedMap[kBolusKey]       = csvIndexMap[kBolusKey];
        
      if( csvIndexMap.find( kGlucoseKeyMM ) != csvIndexMap.end() )
        properKeyedMap[kGlucoseKey]     = csvIndexMap[kGlucoseKeyMM];
      else if( csvIndexMap.find( kGlucoseKey ) != csvIndexMap.end() )
        properKeyedMap[kGlucoseKey]     = csvIndexMap[kGlucoseKey];
      
      if( csvIndexMap.find( kCgmsValueKeyMM ) != csvIndexMap.end() )
        properKeyedMap[kCgmsValueKey]   = csvIndexMap[kCgmsValueKeyMM];
      
      if( csvIndexMap.find(kIsigKey) != csvIndexMap.end() )
        properKeyedMap[kIsigKey]        = csvIndexMap[kIsigKey];
    break;
    
    case Dexcom7Csv:  
      if( csvIndexMap.find( "MeterValue" ) != csvIndexMap.end() )
        properKeyedMap[kMeterBgKey] = 4;
      
      if( csvIndexMap.find( "SensorValue" ) != csvIndexMap.end() )
        properKeyedMap[kCgmsValueKey] = 4;
      
      
      if( csvIndexMap.find( "DisplayTime" ) != csvIndexMap.end() )
      {
        properKeyedMap[kDateKey] = 2;
        properKeyedMap[kTimeKey] = 3;
      }//if( has DisplayTime )
    break;
   
    assert(0);
  }//switch( source )
    
  if( properKeyedMap.find(kTimeKey) == properKeyedMap.end() ) properKeyedMap.clear();
  if( properKeyedMap.find(kDateKey) == properKeyedMap.end() ) properKeyedMap.clear();
  
  // if( !properKeyedMap.empty() )
  // {
    // IndexMapIter iter = properKeyedMap.begin();
    // for( ; iter != properKeyedMap.end(); ++iter )
      // cout << iter->first << "=" << iter->second << ",  ";
    // cout << endl;
  // }//if( we found something )
  
  return properKeyedMap;
}//getHeaderMap(...)
  
//Attempt to figure what generated the spreadsheet
CgmsDataImport::SpreadSheetSource 
CgmsDataImport::getSpreadsheetTypeFromHeader( std::string header )
{
  if( header.find("DisplayTime") != string::npos ) return Dexcom7Csv;
  
  //We'll give minimed a couple tries
  if( header.find(kMeterBgKeyMM) != string::npos )            return MiniMedCsv;
  if( header.find(kCalibrationKeyMM) != string::npos )        return MiniMedCsv;
  if( header.find("Sensor Glucose (mg/dL)") != string::npos ) return MiniMedCsv;
  
  return NumSpreadSheetSource;
}//getSpreadsheetTypeFromHeader
  


//Spreadsheets saved on a apple have a '\r' at end of line, instead of '\n'
char 
CgmsDataImport::getEolCharacter( std::string fileName )
{
  ifstream inputFile( fileName.c_str() );
  
  if( !inputFile.is_open() )
  {
    cout << "Could not open '" << fileName << "' please fix this." << endl;
    exit(1);
  }//if( !inputFile.is_open() )
  
  char line[2024];
  inputFile.getline( line, 2024, '\n' );
  
  if( !inputFile.eof() ) return '\n';
  
  string lineStr = line;  //To get rid of some wierd compiler warning
  
  foreach( const char c, lineStr )  //I like boost
  {
    if( c == '\r' ) return '\r';
    if( c == '\n' ) return '\n';
  }//foreach( read in character )
  
  cout << "CgmsDataImport::getEolCharacter(...): Warning, couldn't find EOL"
       << "character, returning \\n" << endl;
  
  return '\n';
}//getEolCharacter

  
CgmsDataImport::TimeValuePair 
CgmsDataImport::getInfoFromLine( std::string line, 
                                 IndexMap headerMap, 
                                 InfoType infoWanted,
                                 SpreadSheetSource source )
{
  string key = "";

  switch( infoWanted )
  {
    case CgmsReading:      key = kCgmsValueKey; break;
    case MeterReading:     key = kMeterBgKey; break;
    case MeterCalibration: key = kCalibrationKey; break;
    case GlucoseEaten:     key = kGlucoseKey; break;
    case BolusTaken:       key = kBolusKey; break;
    case ISig:             key = kIsigKey; break;
    assert(0);
  };//switch( infoWanted )
  
  
  if( headerMap.find(key) == headerMap.end() )
  {
    cout << "No key for " << key << " in :";
    IndexMap::iterator it = headerMap.begin();
    for( ; it != headerMap.end(); ++it) cout << it->first << "->" << it->second << " ";
    cout << endl;
  }//if( headerMap.find(key) == headerMap.end() )
  
  
  assert( headerMap.find(key) != headerMap.end() );
  assert( headerMap.find(kDateKey) != headerMap.end() );
  assert( headerMap.find(kTimeKey) != headerMap.end() );
  
  string delim = ",";
  if( source == Dexcom7Csv ) delim = " \t";
  
  vector<string> feilds;
  algorithm::split( feilds, line, algorithm::is_any_of( delim ) );
  foreach( string &thisField, feilds ) algorithm::trim( thisField );

  assert( feilds.size() > (unsigned int)headerMap[key] );
  assert( feilds.size() > (unsigned int)headerMap[kDateKey] );
  assert( feilds.size() > (unsigned int)headerMap[kTimeKey] );
  
  string dateStr = feilds[headerMap[kDateKey]];
  string timeStr = feilds[headerMap[kTimeKey]];
  
  dateStr = sanatizeSpreadsheetDate( dateStr, source );
  timeStr = sanitizeTimeInput( timeStr, source );
  
  TimeValuePair returnAnswer;
  
  try
  {
    returnAnswer.first = time_from_string( dateStr + " " + timeStr );
  }catch(boost::exception &) 
  {
    cout << "Couldn't reconstruct dates from input fields date='" 
         << feilds[headerMap[kDateKey]]
         << "', time='" << feilds[headerMap[kTimeKey]]
         << "' which sanatized to '" << dateStr + " " + timeStr
         << "' which is invalid" << endl;
	  exit(1);
  }//try/catch( date )
  
  // cout << "key=" << key << ", field=" << headerMap[key] 
       // << " value=" << feilds[headerMap[key]] << endl;
  string valField = feilds[headerMap[key]];
  
  if( (infoWanted == MeterReading) && (valField == "") ) 
  {
    if( headerMap.find(kCalibrationKey) != headerMap.end() )
      valField = feilds[headerMap[kCalibrationKey]];
  }//if( MeterReading, wants a second try )
  
  returnAnswer.second = kFailValue;
  
  try
  {
    if( valField != "" ) returnAnswer.second = lexical_cast<double>( valField );
  }catch(boost::exception &){
	  cout << "'" << valField << "' is an invalid reading for " << key << endl;
	  exit(1);
  }//try/catch( BG reading )
  
  return returnAnswer;
}//getInfoFromLine
  


//convert time given by day-month-am/pm to ISO standard format
std::string
CgmsDataImport::sanitizeDateAndTimeInput( std::string time, SpreadSheetSource source)
{
   //do some really basic format checks
  assert(time.find(" ") != string::npos );
  assert(time.find(":") != string::npos );

  string monthDay  = time.substr(0, time.find(" "));
  string timeOfDay  = time.substr( time.find(" ") );
  timeOfDay = sanitizeTimeInput( timeOfDay, source );
  time = monthDay + " " + timeOfDay;

  cout << "formatted time=" << time << endl;
  
  return time;
}//sanitizeDateAndTimeInput
  


//convert speadsheets time of day format to ISO standard format
string 
CgmsDataImport::sanitizeTimeInput( std::string time,
                                            SpreadSheetSource source )
{
  //do some really basic format checks
  assert(time.find(":") != string::npos );
  boost::algorithm::trim(time);

  if( source == Dexcom7Csv ) 
  {
    size_t pos = time.find(".");
    time = time.substr( 0, pos );
    return time;
  }//if( sm_isDexcomFile ) 
  
  string monthDay  = time.substr(0, time.find(" "));
  
  if( time.find("am") != string::npos )
  {
    //get rid of the 'am' now  
	  time = time.substr( 0, time.find("am") );
 
    int hourBegin = time.find(" ") + 1;
    int hourLength = time.find(":") - time.find(" ") - 1;
    assert( hourLength>0 && hourLength<3 );

    int properHours = atoi( time.substr( hourBegin, hourLength ).c_str() );
	
    //12:XXam needs to be in the format 00:XX
    if( properHours >= 12 )
    {
      assert( properHours == 12 );
      assert( hourLength == 2 );
      for( int i=hourBegin; i<hourLength; i++ )  time[i] = '0';
    }//if( properHours>=12 )
  }//if( contains 'am'

  if( time.find("pm") != string::npos )
  {
    //watch -- like a wrist watch
    string watchHours = time.substr( 0, time.find(":") - 1);
    string watchMinutes = time.substr( time.find(":") + 1, 2 ); //start at ':', get rid  of 'pm'

    int properHours = atoi( watchHours.c_str() );
    if( properHours < 12 ) properHours += 12; //wach out for 12pm

    assert( properHours < 25 ); //mak sure things are somewhate okay

    time = (format("%s %i:%s") % monthDay % properHours %watchMinutes).str();
    // time = Form("%i:%s", properHours, watchMinutes.c_str() );
  }//if a pm time

  //check for seconds field
  if( time.find_last_of( ":" ) == time.find_first_of( ":" ) ) time += ":00";
	
	
  return time;
}//sanitizeTimeInput


  
//Minimeds date format is not standard ISO format, convert to it
string 
CgmsDataImport::sanatizeSpreadsheetDate( const std::string input, 
                                         SpreadSheetSource source )
{
  string originalInput = input;
  algorithm::trim( originalInput );
  
  if( source == Dexcom7Csv ) return originalInput;
  
  vector<string> dateParts;
  algorithm::split( dateParts, originalInput, boost::algorithm::is_any_of("/-") );
  
  if( dateParts.size() != 3 ) cout << input << " not valid date" << endl;
  assert( dateParts.size() == 3 );
  
  //fix year up
  if( dateParts[2].size() == 2 ) dateParts[2] = "20" + dateParts[2];
  else assert( dateParts[2].size() == 4 );
  
  //fix month up
  if( dateParts[0].size() == 1 ) dateParts[0] = "0" + dateParts[0];
  else assert( dateParts[0].size() == 2 );
  
  //fix day up
  if( dateParts[1].size() == 1 ) dateParts[1] = "0" + dateParts[1];
  else assert( dateParts[1].size() == 2 );
  
  return dateParts[2] + "-" + dateParts[0] + "-" + dateParts[1];
}//sanatizeSpreadsheetDate








ConsentrationGraph 
CgmsDataImport::bolusGraphToInsulinGraph( const ConsentrationGraph &bolusGraph, 
                                                                 double weight )
{
  assert( bolusGraph.getGraphType() == BolusGraph );
  
  const ptime t0 = bolusGraph.getT0();
  const double dt = 1.0;
  
  ConsentrationGraph insConcen( t0, dt, InsulinGraph );
  
  foreach( const GraphElement &el, bolusGraph )
  {
    insConcen.add( el.m_value / weight, el.m_minutes, NovologAbsorbtion );
  }//foreach bolus
  
  insConcen.removeNonInfoAddingPoints();
  
  return insConcen;
}//bolusGraphToInsulinGraph



//A simple debug convertion that makes everything under 17gram carbs into
  //  a short digested food, everything over 17gCarbs, a medium food
  //  returns graph of carb absorbtion rate into blood stream
ConsentrationGraph 
CgmsDataImport::carbConsumptionToSimpleCarbAbsorbtionGraph( 
                                    const ConsentrationGraph &consumptionGraph )
{
  assert( consumptionGraph.getGraphType() == GlucoseConsumptionGraph );
  
  const ptime t0 = consumptionGraph.getT0();
  const double dt =  1.0;
  
  ConsentrationGraph carbConcen( t0, dt, GlucoseAbsorbtionRateGraph );
  
  foreach( const GraphElement &el, consumptionGraph )
  {
    if( el.m_value < 17.0 )
      carbConcen.add( el.m_value, el.m_minutes, FastCarbAbsorbtionRate );
    else carbConcen.add( el.m_value, el.m_minutes, MediumCarbAbsorbtionRate );
  }//foreach bolus
  
  carbConcen.removeNonInfoAddingPoints();
  
  return carbConcen;
}//carbConsumptionToSimpleCarbAbsorbtionGraph(...)





