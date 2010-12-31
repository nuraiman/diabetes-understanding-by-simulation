#if !defined(CGMS_DATA_IMPORT_HH)
#define CGMS_DATA_IMPORT_HH

//Started April 29, 2009,
//This namespace is designed to help readin data from spreadsheets to 
//  a useful format for analysis
#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include <stdexcept>
#include "boost/date_time/posix_time/posix_time.hpp"

#include "ArtificialPancrease.hh"
#include "ConsentrationGraph.hh"



namespace CgmsDataImport
{
  enum SpreadSheetSource
  {
    MiniMedCsv,
    Dexcom7Dm2Tab,  //actually space or tab delimited
    Dexcom7Dm3Tab,
    NavigatorTab,
    GenericCsv,  //header must contain kDateKey and kTimeKey, and no Minimed, Dexcom, or Navigator specific keys
    NumSpreadSheetSource
  };//
  
  enum InfoType
  {
    CgmsReading,
    MeterReading,     //includes calibration and other meter data
    MeterCalibration,
    GlucoseEaten,
    BolusTaken,       //treats all bolus;s the same (so ignores squarness, etc.)
    GenericEvent,
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


  //Navigator imports gets special attential since they have wonderful documentation
  //https://www.abbottdiabetescare.com/en_US/content/document/DOC14109_Rev-A.pdf
  //Also, many of the features that could be implmented are not since other
  //  parts of the project are not using them yet
  enum NavLinePos
  {
    DATEEVENT = 0, TIMESLOT, EVENTTYPE, 
    DEVICE_MODEL, DEVICE_ID, VENDOR_EVENT_TYPE_ID, VENDOR_EVENT_ID, //we will ignore
    KEY0, KEY1, KEY2,
    I0, I1, I2, I3, I4, I5, I6, I7, I8, I9, 
    D0, D1, D2, D3, D4, 
    C0, C1, C2, C3, C4, C5, C6, C7, C8, C9,
    ISMANUAL, COMMENT, NumNavLinePos
  };//enum NavLinePos
  
  
  enum NavTimeSlot
  {
    PRE_BREAKFAST = 0, POST_BREAKFAST = 1, PRE_LUNCH = 2, POST_LUNCH = 3,
    PRE_DINNER = 4, POST_DINNER = 5, BED_TIME = 6, SLEEP = 7
  };//enum NavTimeSlot
  
  enum NavEVENTTYPE
  {
    ET_Exercise = 0,      ET_Glucose = 1,
    ET_BasalInsulin = 2,  ET_BolusInsulin = 3,
    ET_LabResults = 4,    ET_Meal = 5, 
    ET_Medical_Exams = 6, ET_Medications = 7, 
    ET_Notes = 8,         ET_StateOfHealth = 9,
    ET_Ketone = 10,       ET_Alarms = 15,
    ET_Generic = 16,
    //Below here is where I have added to distiguish further
    ET_Glucose_CGMS,
    ET_Glucose_METER,
    // ET_Glucose_CALIBRATION,  //I don't know how to tell this yet
    NumNavEVENTTYPE
  };//enum NavEVENTTYPE
  
  TimeValuePair getNavigatorInfo( std::string line, InfoType type );
  TimeValuePair getDexcomDm3Info( std::string &line,
                                  const IndexMap &headerMap,
                                  InfoType infoWanted );

  const PosixTime kNavigatorT0( boost::gregorian::date(1899, 
                                boost::gregorian::Dec, 30), 
                                TimeDuration( 0, 0, 0, 0));
  PosixTime getDateFromNavigatorDate( const std::string &navDate );
  double convertToNavigatorDate( PosixTime time );
  unsigned int elfHash( const char *str );  //str must be null terminated 

  class NavEvent
  {
    public:
      PosixTime m_dateTime;
      int m_intData[NumNavLinePos];      //includes the keys data and ISMANUAL
      int m_floatData[NumNavLinePos];    
      std::string m_data[NumNavLinePos];  //raw, split data
      
      NavEvent( const std::string &line );
      ~NavEvent();
      
      bool isEventType( const NavEVENTTYPE evtType ) const;
      
      //for all
      bool empty() const;
      PosixTime getTime() const;
      double getValue( const NavEVENTTYPE evtType ) const;
      
      //for excersize
      TimeDuration duration() const;  //elfhash key0
      int intensity() const;          //elfhash key2
      
      //for insulin bolus
      double bolusAmount() const;
      bool isManual() const;
      
      //for glucose
      int glucose() const;
      bool isInRange() const;  //check if low, high, or a Control Value
      bool isCGMData() const;  //true if from cgms, false if from fingerstick
      
      //for Meal
      double carbs() const;  //field D1
      //could have proteins, fat, calories etc.
      
      //for generic
      int genericType() const;    
  };//class NavEvent
  
  
  
  
  

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
  
  TimeValuePair getInfoFromLine( std::string &line,
                                 IndexMap headerMap, 
                                 InfoType infoWanted,
                                 SpreadSheetSource source );
  
  double getMostCommonDt( const TimeValueVec &timeValues );
  TimeDuration getMostCommonPosixDt( const TimeValueVec &timeValues );
  
  
  //convert time given by day-month-am/pm to ISO standard format
  std::string sanitizeDateAndTimeInput( std::string time, SpreadSheetSource source );
  
  //convert speadsheets time of day format to ISO standard format
  std::string sanitizeTimeInput( std::string time, SpreadSheetSource source );
  
  //Minimeds date format is not standard ISO format, convert to it
	std::string sanatizeSpreadsheetDate( const std::string input, 
                                       SpreadSheetSource source );
    
};//namespace CgmsDataImport

#endif //CGMS_DATA_IMPORT_HH
