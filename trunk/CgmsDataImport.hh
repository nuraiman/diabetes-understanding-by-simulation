#if !defined(CGMS_DATA_IMPORT_HH)
#define CGMS_DATA_IMPORT_HH

//Started April 29, 2009,
//This namespace is designed to help readin data from spreadsheets to 
//  a useful format for analysis
#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include "boost/date_time/posix_time/posix_time.hpp"

#include "ArtificialPancrease.hh" //contains useful typedefs and constants
#include "ConsentrationGraph.hh"



namespace CgmsDataImport
{
  enum SpreadSheetSource
  {
    MiniMedCsv,
    Dexcom7Csv,  //I don't claim this works, actually is space or tab seperated
    NumSpreadSheetSource
  };//
  
  enum InfoType
  {
    CgmsReading,
    MeterReading,     //includes calibration and other meter data
    MeterCalibration,
    GlucoseEaten,
    BolusTaken,       //treats all bolus;s the same (so ignores squarness, etc.)
    ISig
  };//enum InfoType
  
  typedef std::map<std::string, int>                  IndexMap;
	typedef IndexMap::iterator                          IndexMapIter;
  typedef std::pair<boost::posix_time::ptime, double> TimeValuePair;
  typedef std::vector< TimeValuePair >                TimeValueVec;
  
  
  const std::string kMeterBgKey       = "MeterValue";
  const std::string kCalibrationKey   = "Calibration";
  const std::string kDateKey          = "Date";
  const std::string kTimeKey          = "Time";
  const std::string kBolusKey         = "Bolus";
  const std::string kGlucoseKey       = "Glucose";
  const std::string kCgmsValueKey     = "CgmsValue";
  const std::string kIsigKey          = "ISIG Value";

  //Mini-Med specific row labels in spreadsheet
  const std::string kMeterBgKeyMM     = "BWZ BG Input (mg/dL)";//"BG Reading (mg/dL)"; 
  const std::string kCalibrationKeyMM = "Sensor Calibration BG (mg/dL)";
  const std::string kBolusKeyMM       = "Bolus Volume Delivered (U)";
  const std::string kGlucoseKeyMM     = "BWZ Carb Input (grams)";
  const std::string kCgmsValueKeyMM   = "Sensor Glucose (mg/dL)";


  //Now Start the functions
  ConsentrationGraph importSpreadsheet( std::string filename, InfoType type, 
                                boost::posix_time::ptime startTime = kGenericT0,
                                boost::posix_time::ptime endTime = kGenericT0 );
  
   //weight in kg needed for conversion from U to mU/L
  ConsentrationGraph bolusGraphToInsulinGraph( 
                                           const ConsentrationGraph &bolusGraph, 
                                           double weight ); 
  
  //A simple debug convertion that makes everything under 17gram carbs into
  //  a short digested food, everything over 17gCarbs, a medium food
  //  returns graph of carb absorbtion rate into blood stream
  ConsentrationGraph 
  carbConsumptionToSimpleCarbAbsorbtionGraph( 
                                   const ConsentrationGraph &consumptionGraph );
  
  //Pass in the header row of the spreadsheet, so we know which column 
  //  contains which information.  If non-header passed in, returns empty map
  IndexMap getHeaderMap( std::string line );
  
  //Attempt to figure what generated the spreadsheet
  SpreadSheetSource getSpreadsheetTypeFromHeader( std::string header );
  
  //Spreadsheets saved on a apple have a '\r' at end of line, instead of '\n'
  char getEolCharacter( std::string fileName );
  
  TimeValuePair getInfoFromLine( std::string line, 
                                 IndexMap headerMap, 
                                 InfoType infoWanted,
                                 SpreadSheetSource source );
  
  double getMostCommonDt( const TimeValueVec &timeValues );
  
  //convert time given by day-month-am/pm to ISO standard format
  std::string sanitizeDateAndTimeInput( std::string time, SpreadSheetSource source );
  
  //convert speadsheets time of day format to ISO standard format
  std::string sanitizeTimeInput( std::string time, SpreadSheetSource source );
  
  //Minimeds date format is not standard ISO format, convert to it
	std::string sanatizeSpreadsheetDate( const std::string input, 
                                       SpreadSheetSource source );
    
};//namespace CgmsDataImport

#endif //CGMS_DATA_IMPORT_HH
