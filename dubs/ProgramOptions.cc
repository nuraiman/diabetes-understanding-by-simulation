#include <cstdlib>
#include <iostream>
#include <iomanip>


#include "ArtificialPancrease.hh"
#include "ProgramOptions.hh"

#include "boost/program_options.hpp"

using namespace std;
using namespace PersonConstants;
namespace po = boost::program_options;

//instantiate the variable that are declared 'extern' elsewhere
po::variables_map         ProgramOptions::ns_poVariableMap;
po::options_description   ProgramOptions::ns_poDescription( "Allowed Options" );
po::positional_options_description  ProgramOptions::ns_posDescripton;



void ProgramOptions::declareOptions()
{
  ns_poDescription.add_options()
        ("help", "produce help message")
        ("weight,kg", po::value<double>(&PersonConstants::kPersonsWeight)->default_value(78.0), 
          "Your weight in kilo-grams")
        ( "cgmsdelay", 
          po::value<double>(&ModelDefaults::kDefaultCgmsDelay)->default_value(15.0),
          "Default delay between CGMS and Fingerstick readings" )
        ( "cgms_indiv_uncert", 
          po::value<double>(&ModelDefaults::kCgmsIndivReadingUncert)->default_value(0.025),
          "The uncertainty of one CGMS reading, relative to the" 
          "one immediately before or after it" )
        ( "basal_blood_glucose,bbg",
          po::value<double>(&PersonConstants::kBasalGlucConc)->default_value(120.0),
          "The target basal blood glucose concentration (mg/dl)");
  ns_posDescripton.add( "weight", 0 );
}//void declareOptions()


void ProgramOptions::decodeOptions( int argc, char **argv )
{
  declareOptions();
  
  po::store(po::parse_command_line(argc, argv, ns_poDescription), ns_poVariableMap);
  po::store(po::command_line_parser(argc, argv).
            options(ns_poDescription).positional(ns_posDescripton).run(), 
            ns_poVariableMap);
  
  po::notify(ns_poVariableMap); 
  
  if( ns_poVariableMap.count( "help") )
  {
    ns_poDescription.print( cout );
    exit(0);
  }//if( ns_poVariableMap.count( "help") )
  
}//void decodeOptions( int argv, char **argc )
