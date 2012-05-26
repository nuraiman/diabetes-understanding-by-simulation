#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "TGLabel.h"

#include "ArtificialPancrease.hh"
#include "ProgramOptions.hh"
#include "RungeKuttaIntegrater.hh" //for toNMinutes()
#include "ResponseModel.hh"

#include <boost/program_options.hpp>

using namespace std;
namespace po = boost::program_options;

//instantiate the variable that are declared 'extern' elsewhere
po::variables_map         ProgramOptions::ns_poVariableMap;
po::options_description   ProgramOptions::ns_poDescription( "Allowed Options" );
po::positional_options_description  ProgramOptions::ns_posDescripton;

//these variable get values through ProgramOptions
double ProgramOptions::kPersonsWeight  = kFailValue;
double ProgramOptions::kBasalGlucConc  = kFailValue;
double ProgramOptions::kCgmsIndivReadingUncert = kFailValue;
TimeDuration ProgramOptions::kDefaultCgmsDelay(0,0,0,0);
TimeDuration ProgramOptions::kPredictAhead(0,0,0,0);
TimeDuration ProgramOptions::kIntegrationDt(0,0,0,0);
double ProgramOptions::kLastPredictionWeight = kFailValue;
double ProgramOptions::kTargetBG = kFailValue;
double ProgramOptions::kBGLowSigma = kFailValue;
double ProgramOptions::kBGHighSigma = kFailValue;
int ProgramOptions::kGenPopSize = -999;
int ProgramOptions::kGenConvergNsteps = -999;
int ProgramOptions::kGenNStepMutate = -999;
int ProgramOptions::kGenNStepImprove = -999;
double ProgramOptions::kGenSigmaMult = kFailValue;
double ProgramOptions::kGenConvergCriteria = kFailValue;
std::string ProgramOptions::ns_defaultModelFileName = "";



void ProgramOptions::declareOptions()
{
  ns_poDescription.add_options()
        ("help", "produce help message")
        ("weight,kg", po::value<double>(&kPersonsWeight)->default_value(78.0),
          "Your weight in kilo-grams")
        ( "cgmsdelay,d",
          po::value<double>()->default_value(15.0),
          "Default delay between CGMS and Fingerstick readings" )
        ( "cgms_indiv_uncert",
          po::value<double>(&kCgmsIndivReadingUncert)->default_value(0.025),
          "The uncertainty of one CGMS reading, relative to the"
          "one immediately before or after it" )
        ( "basal_blood_glucose,g",
          po::value<double>(&kBasalGlucConc)->default_value(120.0),
          "The target basal blood glucose concentration (mg/dl)")
        ( "predictahead,p",
          po::value<double>()->default_value(45.0),
          "How far ahead of the cgms the model should try to predict, in minutes" )
        ("dt",
          po::value<double>()->default_value(1.0),
          "Integration timestep in minutes" )
        ( "last_pred_weight,l",
          po::value<double>(&kLastPredictionWeight)->default_value(0.25),
          "Weight of the last prediction (so predictahead of cgms data point),"
          "when finding model paramaters.")
        (
          "target",
          po::value<double>(&kTargetBG)->default_value(100.0),
          "Target Blood Glucose Value")
        (
          "lowsigma",
          po::value<double>(&kBGLowSigma)->default_value(10.0),
          "Relative weight of BG below Target")
        ("highsigma",
          po::value<double>(&kBGHighSigma)->default_value(20.0),
          "Relative weight of BG above Target")
        ("genetic_pop_size",
          po::value<int>(&kGenPopSize)->default_value(100),
          "Size of population to genetically optimize with")
        ("genetic_conv_steps",
          po::value<int>(&kGenConvergNsteps)->default_value(10),
          "The number generation needed with no improvment, to stop genetic optimiztion")
        ("genetic_nstep_track",
          po::value<int>(&kGenNStepMutate)->default_value(6),
          "Number of generation to keep track of")
        ("genetic_ngen_improve",
          po::value<int>(&kGenNStepImprove)->default_value(3),
          "The number of generation within last genetic_nstep_track generation that"
          " must improve, or else mutations increased/decreased")
        ("genetic_mutate_sigma",
          po::value<double>(&kGenSigmaMult)->default_value(0.5),
          "The mutation multiple")
        ("genetic_convergence_chi2",
          po::value<double>(&kGenConvergCriteria)->default_value(1.0),
          "Size of fitness improvment needed in the last genetic_conv_steps so that"
          " minimization will continue")
        ( "file,f",
          po::value<string>(&ns_defaultModelFileName)->default_value(""),
          "NLSimple File to automatically load");
     //ns_posDescripton.add( "weight", 0 );  //why is this here?
     ns_posDescripton.add( "file", 1 ); //untested Dec 2009
}//void declareOptions()


void ProgramOptions::decodeOptions( int argc, char **argv )
{
  //declareOptions();

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


  ProgramOptions::kDefaultCgmsDelay = roundToNearestSecond( ns_poVariableMap["cgmsdelay"].as<double>() );
  ProgramOptions::kIntegrationDt    = roundToNearestSecond( ns_poVariableMap["dt"].as<double>() );
  ProgramOptions::kPredictAhead     = roundToNearestSecond( ns_poVariableMap["predictahead"].as<double>() );
}//void decodeOptions( int argv, char **argc )


TimeDuration ProgramOptions::roundToNearestSecond( const TimeDuration &td )
{
  long totalMiliSeconds = td.total_milliseconds();
  long nSecond = totalMiliSeconds / 1000;
  long nMili = totalMiliSeconds % 1000;

  if( nMili > 499 ) ++nSecond;

  return TimeDuration( 0, 0, nSecond, 0 );
}//toNSeconds

TimeDuration ProgramOptions::roundToNearestSecond( const double nMinutes )
{
  return roundToNearestSecond( toTimeDuration(nMinutes) );
}//roundToNearestSecond



ModelSettings::ModelSettings()
{
  m_personsWeight          = ProgramOptions::kPersonsWeight;
  //m_basalGlucConc          = ProgramOptions::kBasalGlucConc;

  m_cgmsIndivReadingUncert = ProgramOptions::kCgmsIndivReadingUncert;

  m_defaultCgmsDelay       = ProgramOptions::kDefaultCgmsDelay;
  m_cgmsDelay              = m_defaultCgmsDelay;
  m_predictAhead           = ProgramOptions::kPredictAhead;
  m_dt                     = ProgramOptions::kIntegrationDt;

  m_endTrainingTime        = kGenericT0;
  m_startTrainingTime      = kGenericT0;

  m_lastPredictionWeight   = ProgramOptions::kLastPredictionWeight;

  m_targetBG               = ProgramOptions::kTargetBG;
  m_bgLowSigma             = ProgramOptions::kBGLowSigma;
  m_bgHighSigma            = ProgramOptions::kBGHighSigma;

  //Genetic minimization paramaters
  m_genPopSize             = ProgramOptions::kGenPopSize;
  m_genConvergNsteps       = ProgramOptions::kGenConvergNsteps;
  m_genNStepMutate         = ProgramOptions::kGenNStepMutate;
  m_genNStepImprove        = ProgramOptions::kGenNStepImprove;
  m_genSigmaMult           = ProgramOptions::kGenSigmaMult;
  m_genConvergCriteria     = ProgramOptions::kGenConvergCriteria;

  m_minFingerStickForCharacterization = 10;
}//ModelSettings::ModelSettings()

ModelSettings::~ModelSettings(){}




