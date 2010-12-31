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
using namespace CgmsDataImport;



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
    const string msg = "Could not open '" + filename + "' please fix this.";
    cout << msg << endl;
    throw runtime_error( msg );
  }//if( !inputFile.is_open() 
  
  int nThisMinute = 0;
  PosixTime prevTime = kGenericT0;
  
  string currentLine;
  
  while( !inputFile.eof() )
  {
    getline( inputFile, currentLine, eolChar );

    if( currentLine.size() == 0 ) continue;
    if( currentLine[0] == '#' ) continue;
    if( currentLine.find_first_not_of(", ") == string::npos ) continue;
    
    if( indMap.empty() && source!=NavigatorTab )
    {
      indMap = getHeaderMap( currentLine );
      SpreadSheetSource sssource = getSpreadsheetTypeFromHeader(currentLine);
      if( !indMap.empty() || sssource==NavigatorTab )  source = sssource;
    }else
    {
      assert( source != NumSpreadSheetSource );
      TimeValuePair lineinfo = getInfoFromLine( currentLine, indMap, type, source );

      
      //make sure not a something like 2 bolusses in the same minute, 
      //which would crash program later on. 
      bool isDuplicateTime = false;
      if( (prevTime!=kGenericT0) && (prevTime==lineinfo.first) ) 
      {
        nThisMinute+=2;
        isDuplicateTime = true;
        lineinfo.first += TimeDuration(0, 0, nThisMinute,0);
        // cout << "Adding " << nThisMinute << " seconds to " << lineinfo.first 
             // << "  -value: " << lineinfo.second << endl;
      }//if( second time in a row we have event in same minute )
      
      if( lineinfo.second != kFailValue ) timeValues.push_back( lineinfo );
      
      if( (lineinfo.first!=kGenericT0) && !isDuplicateTime )
      {
        nThisMinute = 0;
        prevTime = lineinfo.first;
      }//the was a new time
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
    case GenericEvent:      graphType = CustomEvent;               break;
    case ISig:              graphType = NumGraphType;              break;
    
    assert(0);
  };//switch(type)
  
  double dt = 5.0;
 
  if( indMap.empty() && source!=NavigatorTab )
  {
    const string msg = "Failed to decode file '" + filename + "' as InfoType="
                       + boost::lexical_cast<string>( int(type) );
    cerr << msg << endl;
    throw runtime_error( msg );
  }//if
  
  if( timeValues.empty() ) return ConsentrationGraph( startTime, dt, graphType );

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
    
    // cout << "On " << thisTime << "  " << thisValue << endl;
    
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
  if( timeValues.size() < 2 ) return TimeDuration(0,5,0,0);
  
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
  
  const SpreadSheetSource source = getSpreadsheetTypeFromHeader( line );

  //if( this wasn't a header line of text )
  if( source == NavigatorTab ) return properKeyedMap;
  if( source == NumSpreadSheetSource ) return properKeyedMap;
  
  algorithm::trim(line);
  
  vector<string> feilds;
  string delimiter = ",";
  if( source == Dexcom7Dm2Tab ) delimiter = " \t";
  else if( source == Dexcom7Dm3Tab ) delimiter = "\t";
  algorithm::split( feilds, line, algorithm::is_any_of(delimiter) );
  
  if( feilds.empty() ) return csvIndexMap; //didn't find anything useful

  int fieldNumber = 0;
  foreach( string thisField, feilds )
  {
	  algorithm::trim( thisField );
	  csvIndexMap[thisField] = fieldNumber++;
  }//foeach( field )
  
  if( source == Dexcom7Dm3Tab ) return csvIndexMap;

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
    
    case Dexcom7Dm2Tab:
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

    case Dexcom7Dm3Tab:
      assert(0);
    break;


    case GenericCsv:
     //lets make sure we don't have any unidentified headers
      properKeyedMap = csvIndexMap;

      foreach( string thisField, feilds )
      {
        if( (thisField != kMeterBgKey)
          && (thisField != kCalibrationKey)
          && (thisField != kDateKey)
          && (thisField != kTimeKey)
          && (thisField != kBolusKey)
          && (thisField != kGlucoseKey)
          && (thisField != kCgmsValueKey)
          && (thisField != kIsigKey) )
        {
            cout << "CgmsDataImport::getHeaderMap(...): Warning, could not"
                <<  "identify the header value '" << thisField << "', you"
                << " may wish to check input formating." << endl;
        }//if( check to see if this name is okay )
      }//foreach column in header
    break;


    case NavigatorTab:
    
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
  if( header.find("PatientInfoField") != string::npos ) return Dexcom7Dm3Tab;
  //"DisplayTime" is in both Dexcom7Dm2Tab and Dexcom7Dm3Tab files
  if( header.find("DisplayTime") != string::npos ) return Dexcom7Dm2Tab;


  //"PatientInfoField"
  //We'll give minimed a couple tries
  if( header.find(kMeterBgKeyMM) != string::npos )            return MiniMedCsv;
  if( header.find(kCalibrationKeyMM) != string::npos )        return MiniMedCsv;
  if( header.find("Sensor Glucose (mg/dL)") != string::npos ) return MiniMedCsv;
  
  if( header.find("DATEEVENT") != string::npos ) 
  {
    assert(  header.find("\t") != string::npos );
    return NavigatorTab;
  }//if( navigator )
  
  if( (header.find(kDateKey) != string::npos)
     && (header.find(kTimeKey) != string::npos) ) return GenericCsv;

  return NumSpreadSheetSource;
}//getSpreadsheetTypeFromHeader
  


//Spreadsheets saved on a apple have a '\r' at end of line, instead of '\n'
char 
CgmsDataImport::getEolCharacter( std::string fileName )
{
    //by default osx has  '\r\n' ofr
  //cout << "CgmsDataImport::getEolCharacter( " << fileName << "):" << endl;

  ifstream inputFile( fileName.c_str() );
  
  if( !inputFile.is_open() )
  {
    const string msg = "Could not open '" + fileName + "' please fix this.";
    cout << msg << endl;
    throw runtime_error( msg );
  }//if( !inputFile.is_open() )
  
  char *line = new char[4048];
  inputFile.getline( line, 4047, '\n' );
  //cout << "getEolCharacter(---): got line " << line << endl;
  string lineStr = line;  //To get rid of some wierd compiler warning
  delete line;

  //if( !inputFile.eof() ) return '\n';

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
CgmsDataImport::getNavigatorInfo( string line, InfoType type )
{
  algorithm::trim(line);

  TimeValuePair returnInfo( kGenericT0, kFailValue );
  
  NavEVENTTYPE navEventTypeWanted = NumNavEVENTTYPE;
  
  switch( type )
  {
    case CgmsReading:      navEventTypeWanted = ET_Glucose_CGMS;  break;
    case MeterReading:     navEventTypeWanted = ET_Glucose_METER; break;
    case MeterCalibration: assert(0); //navEventTypeWanted = ET_Glucose_CALIBRATION;
    case GlucoseEaten:     navEventTypeWanted = ET_Meal;          break;
    case BolusTaken:       navEventTypeWanted = ET_BolusInsulin;  break;
    case GenericEvent:     navEventTypeWanted = ET_Generic;       break;
    case ISig:             assert(0);
  };//switch( type )
  
  algorithm::trim(line);
  if( line.size() == 0 ) return returnInfo;
	if( line[0] == '#' )   return returnInfo;
	if( line.find_first_not_of("\t") == string::npos ) return returnInfo;
 
  
  NavEvent event(line);
  if( !event.isEventType(navEventTypeWanted) ) return returnInfo;
  
  // if( navEventTypeWanted == ET_Glucose_METER ) cout << line << endl;
  
  returnInfo.first = event.getTime();
  returnInfo.second = event.getValue(navEventTypeWanted);
  
  // cout << "For navigator I got " << returnInfo.first << "  -  value=" << returnInfo.second << endl;
  return returnInfo;
}//getNavigatorInfo(...)


CgmsDataImport::TimeValuePair
CgmsDataImport::getDexcomDm3Info( std::string &line,
                                  const IndexMap &headerMap,
                                  InfoType infoWanted )
{
  TimeValuePair answer(kGenericT0, kFailValue);

  vector<string> feilds;
  algorithm::split( feilds, line, algorithm::is_any_of( "\t" ) );
  foreach( string &thisField, feilds ) algorithm::trim( thisField );

  // cerr << "working on line: '" << line << "'" << endl;
  // cerr << "Found " << feilds.size() << " fields:\n   ";
  // foreach( const string &f, feilds ) cerr << "'" << f << "',  ";
  // cerr << endl;

  if( feilds.size() < 2 ) return answer;

  //Either the dateIter and valueIter or dateField and valueField can be removed below
  IndexMap::const_iterator dateIter = headerMap.end();
  IndexMap::const_iterator valueIter = headerMap.end();

  int dateField = -1, valueField = -1;
  const int eventTypeField = headerMap.find("EventType")->second;
  // cerr << "const int eventTypeField=" << eventTypeField << endl;

  assert( eventTypeField < static_cast<int>(feilds.size()) );
  if( eventTypeField >= static_cast<int>(feilds.size()) )
    throw runtime_error( "Not a valid dexcom file!" );

  switch( infoWanted )
  {
    case CgmsReading:
      dateIter = headerMap.find("SensorDisplayTime");
      valueIter = headerMap.find("SensorValue");
      dateField = dateIter->second;
      valueField = valueIter->second;
    break;

    case MeterReading: case MeterCalibration:
      dateIter = headerMap.find("MeterDisplayTime");
      valueIter = headerMap.find("MeterValue");
      dateField = dateIter->second;
      valueField = valueIter->second;
    break;

    case GlucoseEaten:
      dateIter = headerMap.find("EventDisplayTime");
      valueIter = headerMap.find("EventDecription");
      dateField = dateIter->second;
      valueField = valueIter->second;
      if( feilds.at(eventTypeField) != "Carbs" ) valueField = -1;
    break;

    case BolusTaken:
      dateIter = headerMap.find("EventDisplayTime");
      valueIter = headerMap.find("EventDecription");
      dateField = dateIter->second;
      valueField = valueIter->second;
      if( feilds.at(eventTypeField) != "Insulin" ) valueField = -1;
    break;

    case GenericEvent:
      dateIter = headerMap.find("EventDisplayTime");
      valueIter = headerMap.find("EventDecription");
      dateField = dateIter->second;
      valueField = valueIter->second;
      //if( feilds.at(eventTypeField) != "" ) valueField = -1;
      valueField = -1;
      //Possible types of custom events are "Exercise Light", "Exercise Medium", "Exercise Hard", "Health Illness", "Health Stress", "Health HighSymptoms", "Health LowSymptoms", "Health Cycle", "Health Alcohol"
    break;

    case ISig: break;
  };//switch( infoWanted )

  assert( (dateIter!=headerMap.end()) && (valueIter!=headerMap.end()) );
    //throw out_of_range( "Invalid Header Map passed into CgmsDataImport::getDexcomDm3Info()" );

  // cerr << "   dateFieldInd=" << dateField << ", valueFieldInd=" << valueField << endl;

  if( valueField < 0 ) return answer;

  const string &rawValueStr = feilds.at( valueField );
  if( rawValueStr.empty() ) return answer;

  const string &rawDateTime = feilds.at(dateField);
  const string dateTimeStr = sanitizeDateAndTimeInput( rawDateTime, Dexcom7Dm3Tab );

  // cerr << " rawValueStr='" << rawValueStr << "', rawDateTime='" << rawDateTime << "'" << endl;

  try
  {
    answer.first = time_from_string( dateTimeStr );
  }catch(boost::exception &)
  {
    ostringstream msg;
    msg << "Couldn't reconstruct dates from input fields date/time='"
        << rawDateTime << "', which sanatized to '" << dateTimeStr
        << "' which is invalid";
    cerr << msg.str() << endl;
    throw runtime_error( msg.str() );
  }//try/catch( date )

  try
  {
    answer.second = lexical_cast<double>( rawValueStr );
    if( infoWanted == BolusTaken ) answer.second /= 100.0;
  }catch(boost::exception &)
  {
    const string msg = "'" + rawValueStr
                       + "' is an invalid reading for infoWanted="
                       + boost::lexical_cast<string>( static_cast<int>(infoWanted) );
    cerr << msg << endl;
    throw runtime_error( msg );
  }//try/catch( BG reading )

  //cerr << "  returning '" << answer.first << "' = " << answer.second << endl << endl << endl;

  return answer;
}//getDexcomDm3Info(...)


CgmsDataImport::TimeValuePair 
CgmsDataImport::getInfoFromLine( std::string &line,
                                 IndexMap headerMap, 
                                 InfoType infoWanted,
                                 SpreadSheetSource source )
{
  if( source == NavigatorTab ) return getNavigatorInfo( line, infoWanted );
  if( source == Dexcom7Dm3Tab ) return getDexcomDm3Info( line, headerMap, infoWanted );

  algorithm::trim(line);

  string key = "";

  switch( infoWanted )
  {
    case CgmsReading:      key = kCgmsValueKey;   break;
    case MeterReading:     key = kMeterBgKey;     break;
    case MeterCalibration: key = kCalibrationKey; break;
    case GlucoseEaten:     key = kGlucoseKey;     break;
    case BolusTaken:       key = kBolusKey;       break;
    case ISig:             key = kIsigKey;        break;
    case GenericEvent:     
      cout << "GenericEvent only implemented for Navigator Freestyle only" << endl;
    return TimeValuePair();
    
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
  if( source == Dexcom7Dm2Tab ) delim = " \t";

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
    ostringstream msg;
    msg << "Couldn't reconstruct dates from input fields date='"
        << feilds[headerMap[kDateKey]]
        << "', time='" << feilds[headerMap[kTimeKey]]
        << "' which sanatized to '" << dateStr + " " + timeStr
        << "' which is invalid";
    cerr << msg.str() << endl;
    throw runtime_error( msg.str() );
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
    const string msg = "'" + valField + "' is an invalid reading for " + key;
    cerr << msg << endl;
    throw runtime_error( msg );
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

  // cout << "formatted time=" << time << endl;
  
  return time;
}//sanitizeDateAndTimeInput
  


//convert speadsheets time of day format to ISO standard format
string 
CgmsDataImport::sanitizeTimeInput( std::string time,
                                            SpreadSheetSource source )
{
  //do some really basic format checks
  if( time.find(":") == string::npos )
    cerr << "CgmsDataImport::sanitizeTimeInput(...): invalid time '"
         << time << "'" << endl;

  assert(time.find(":") != string::npos );
  boost::algorithm::trim(time);

  if( (source == Dexcom7Dm2Tab) || (source == Dexcom7Dm3Tab) )
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
  
  if( source == GenericCsv ) return originalInput;
  if( source == Dexcom7Dm2Tab ) return originalInput;
  
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
  
  const PosixTime t0 = bolusGraph.getT0();
  const double dt = 1.0;
  ConsentrationGraph insConcen( t0, dt, InsulinGraph );
  foreach( const GraphElement &el, bolusGraph )
  {
    if( el.m_value > 0.0 ) insConcen.add( el.m_value / weight, el.m_time, NovologAbsorbtion );
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
    if( el.m_value < 0.1 ) continue;
    if( el.m_value < 17.0 )
      carbConcen.add( el.m_value, el.m_time, FastCarbAbsorbtionRate );
    else carbConcen.add( el.m_value, el.m_time, MediumCarbAbsorbtionRate );
  }//foreach bolus
  
  carbConcen.removeNonInfoAddingPoints();
  
  return carbConcen;
}//carbConsumptionToSimpleCarbAbsorbtionGraph(...)



unsigned int CgmsDataImport::elfHash( const char *name )
{
  //adapted from http://forums.devshed.com/c-programming-42/hash-function-in-c-54284.html
  //  and https://www.abbottdiabetescare.com/en_US/content/document/DOC14109_Rev-A.pdf
  unsigned int result = 0, x;

  while( *name )
  {
    result = ( result << 4 ) + *name++;  //name isn't necassarily unsigned char...
    x = result & 0xF0000000;
    
    if( x ) result ^= x >> 24;  // '^=' is exclusive or

    result &= ~x;
  }//while( more character left )

  return result; // % 997;
}//unsigned int CgmsDataImport::elfHash( const char *name);


PosixTime CgmsDataImport::getDateFromNavigatorDate( const string &navDate )
{
  using boost::lexical_cast;
  using boost::bad_lexical_cast;
  
  size_t periodPos = navDate.find( "." );
  if( navDate.find_first_of(".") != navDate.find_last_of(".") )
  {
    cout << "Warning, Navigator date shouldn't have than one decimal point: \"" 
         << navDate << "\" does not" << endl;
    return kGenericT0;
  }//
  
  const string dateField = navDate.substr(0, periodPos);
  const string timeField = (periodPos==string::npos) ? "0" : "0" + navDate.substr(periodPos);
  
  
  int nDays = -1;
  double dayFrac = -1.0;
  
  try{ nDays = lexical_cast<int>(dateField); } 
  catch( bad_lexical_cast error )
  {
    cout << "\"" << navDate << "\" is an invalid date (" << dateField 
         << " not integer)" << endl;
    throw error;
  }//try catch
  
  try{ dayFrac = lexical_cast<double>(timeField); } 
  catch ( bad_lexical_cast error )
  {
    cout << "\"" << navDate << "\" is an invalid date (" << timeField 
         << " not double)" << endl;
    throw error;
  }//try catch
  
  assert( nDays > 0 );
  assert( dayFrac >= 0.0 && dayFrac <= 1.0 );
  
  PosixTime time = kNavigatorT0 + TimeDuration( 24*nDays, 0, 0, 0 );
  
  int nMinutes = int(24.0 * 60.0 * dayFrac + 0.5); //0.5 makes it to round to nearest minute
  
  time += toTimeDuration(nMinutes);
  
  // cout << "\"" << navDate << "\"=" << time << endl;
  
  return time;
}//getDateFromNavigatorDate( const string &navDate )


double CgmsDataImport::convertToNavigatorDate( PosixTime time )
{
  const double nMinutesInDay = 24.0 * 60.0;
  double nMinutes = toNMinutes( time.time_of_day() );
  long nDays = (time.date() - kNavigatorT0.date()).days();
  
  return nDays + (nMinutes / nMinutesInDay);
}//double CgmsDataImport::convertToNavigatorDate( const PosixTime &time )


  
NavEvent::NavEvent( const std::string &line ) : m_dateTime(kGenericT0)
{
  using boost::lexical_cast;
  using boost::bad_lexical_cast;
  
  for( NavLinePos pos = NavLinePos(0); 
       pos < NumNavLinePos; pos = NavLinePos(pos+1) )
  {
    m_intData[pos] = -1;
    m_floatData[pos] = 0.0;
    m_data[pos] = "";
  }//for( loop over data positons 'pos' )
  
  vector< string > fields;
  algorithm::split( fields, line, is_any_of("\t") );
  
  int nFields = static_cast<int>(fields.size());
  
  //If the "COMMENT" field is blank, then we get 'NumNavLinePos-1' fields
  if( fields.size() > 1 && (abs(nFields - NumNavLinePos) > 1) )
  {
    cout << "NavEvent: Warning following line not a valid Navigator line" << endl
         << line << endl << "   (" << fields.size() 
         << " fields instead of " << NumNavLinePos 
         << " or " << NumNavLinePos << ")" << endl;
  }//
  
  algorithm::trim(fields[DATEEVENT]);
  if( abs(nFields - NumNavLinePos) > 1 ) return;
  if( fields[DATEEVENT] == "" )          return;
  if( fields[DATEEVENT] == "DATEEVENT" ) return; //it's the header of the file
  
  
  for( size_t pos = 0; pos < fields.size(); ++pos )
  {
    m_data[pos] = fields[pos];
    algorithm::trim( m_data[pos] );
  }//for( loop over data positons 'pos' )
  
  m_dateTime = getDateFromNavigatorDate( m_data[DATEEVENT] );
  
  for( NavLinePos intPos = TIMESLOT; intPos < D0; intPos = NavLinePos(intPos+1) )
  {
    if( m_data[intPos].empty() || intPos==DEVICE_MODEL || intPos==DEVICE_ID 
        || intPos==VENDOR_EVENT_TYPE_ID || intPos==VENDOR_EVENT_ID ) continue; //skip over non integers
  
    
    try{ m_intData[intPos] = lexical_cast<int>(m_data[intPos]); } 
    catch ( bad_lexical_cast error )
    {
      cerr << "\"" << m_data[intPos] << "\" is an invalid int" << endl;
      throw error;
    }//try catch
  }//for( loop over potential integer 
  
  //the isManual flag
  try{ m_intData[ISMANUAL] = lexical_cast<int>(m_data[ISMANUAL]); } 
  catch ( bad_lexical_cast error )
  {
    cerr << "\"" << m_data[ISMANUAL] << "\" is an invalid int" << endl;
    throw error;
  }//try catch
    
    
  for( NavLinePos dPos = D0; dPos <= D4; dPos = NavLinePos(dPos+1) )
  {  
    try{ m_floatData[dPos] = lexical_cast<double>(m_data[dPos]); } 
    catch ( bad_lexical_cast error )
    {
      cerr << "\"" << m_data[dPos] << "\" is an invalid double" << endl;
      throw error;
    }//try catch
  }//for( loop over potential integer 
  //we're done
}//NavEvent::NavEvent( const std::string &line )
  

NavEvent::~NavEvent(){}

bool NavEvent::isEventType( const NavEVENTTYPE evtType ) const
{
  if( evtType==ET_Glucose_CGMS ) return ((m_intData[EVENTTYPE]==ET_Glucose) && isCGMData() && isInRange() );
    
  if( evtType==ET_Glucose_METER ) return ((m_intData[EVENTTYPE]==ET_Glucose) && !isCGMData() && isInRange() );
  
  // if( evtType==ET_Glucose_CALIBRATION ) return ((m_intData[EVENTTYPE]==ET_Glucose) && (m_intData[I5]==0));
  
  
  return (evtType == m_intData[EVENTTYPE]);
}//isEventType(...)


//for all
bool NavEvent::empty() const
{
  return m_data[DATEEVENT].empty();
}//empty()



PosixTime NavEvent::getTime() const
{
  return m_dateTime; 
}//getTime()


double NavEvent::getValue( const NavEVENTTYPE evtType ) const
{
  assert( isEventType(evtType) );
  
  switch( evtType )
  {
    
    case ET_Glucose:
    case ET_Glucose_CGMS:
    case ET_Glucose_METER: return (double)glucose();
    case ET_BolusInsulin:  return bolusAmount();
    case ET_Meal:          return carbs();
    case ET_Generic:       return (double)genericType();
      
    case ET_Exercise:    
    case ET_BasalInsulin:  
    case ET_LabResults:    
    case ET_Medical_Exams: 
    case ET_Medications: 
    case ET_Notes:         
    case ET_StateOfHealth:
    case ET_Ketone:      
    case ET_Alarms:
      assert(0);    
    //Below here is where I have added to distiguish further
    // ET_Glucose_CALIBRATION,  //I don't know how to tell this yet
    case NumNavEVENTTYPE: assert(0);
  };//switch( evtType )
  
  return kFailValue;
}//double NavEvent::getValue
      
//for excersize
TimeDuration NavEvent::duration() const //elfhash key0
{
  assert( m_intData[EVENTTYPE] == ET_Exercise );
  unsigned int duration = elfHash( m_data[KEY0].c_str() );
  
  cout << m_data[KEY0] << " hashes to a duration of " << duration << endl;
  
  return TimeDuration( 0, duration, 0, 0 );
}//duration()

int NavEvent::intensity() const //elfhash key2
{
  assert( m_intData[EVENTTYPE] == ET_Exercise );
  unsigned int intensity = elfHash( m_data[KEY2].c_str() );
  cout << m_data[KEY2] << " hashes to a intensity of " << intensity << endl;
  
  return static_cast<int>(intensity);
}//int NavEvent::intensity()


//for insulin bolus
double NavEvent::bolusAmount() const
{
  assert( m_intData[EVENTTYPE] == ET_BolusInsulin );
  return m_floatData[D0];
}//double NavEvent::bolusAmount() const


bool NavEvent::isManual() const
{
  assert( m_intData[EVENTTYPE] == ET_BolusInsulin );
  return m_intData[ISMANUAL];
}//double NavEvent::bolusAmount() const
      

//for glucose
int NavEvent::glucose() const
{
  assert( m_intData[EVENTTYPE] == ET_Glucose );
  return m_intData[I1];
}//int NavEvent::glucose()


bool NavEvent::isInRange() const  //check if low, high, or a Control Value
{
  assert( m_intData[EVENTTYPE] == ET_Glucose );
  
  return (m_intData[I0]==0 && m_intData[I2]==0 && m_intData[I4]==0);
}//bool NavEvent::isInRange()


bool NavEvent::isCGMData() const  //true if from cgms, false if from fingerstick
{
  assert( m_intData[EVENTTYPE] == ET_Glucose );
  assert( m_intData[I5]==0 || m_intData[I5]==1 );
  return m_intData[I5]==1;
}//isCGMData()


//for Meal
double NavEvent::carbs() const  //field D1
{
  assert( m_intData[EVENTTYPE] == ET_Meal );
  return m_floatData[D1];
}//
//could have proteins, fat, calories etc.
      
//for generic
int NavEvent::genericType() const
{
  assert( m_intData[EVENTTYPE] == ET_Generic );
  
  return m_intData[I0];
}//NavEvent::genericType()
  
  



