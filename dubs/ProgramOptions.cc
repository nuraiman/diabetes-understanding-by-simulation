#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "TGLabel.h"

#include "ArtificialPancrease.hh"
#include "ProgramOptions.hh"
#include "RungeKuttaIntegrater.hh" //for toNMinutes()

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
        ( "cgmsdelay,d", 
          po::value<double>(&ModelDefaults::kDefaultCgmsDelay)->default_value(15.0),
          "Default delay between CGMS and Fingerstick readings" )
        ( "cgms_indiv_uncert", 
          po::value<double>(&ModelDefaults::kCgmsIndivReadingUncert)->default_value(0.025),
          "The uncertainty of one CGMS reading, relative to the" 
          "one immediately before or after it" )
        ( "basal_blood_glucose,g",
          po::value<double>(&PersonConstants::kBasalGlucConc)->default_value(120.0),
          "The target basal blood glucose concentration (mg/dl)")
        ( "predictahead,p",
          po::value<double>(&ModelDefaults::kPredictAhead)->default_value(45.0),
          "How far ahead of the cgms the model should try to predict, in minutes" )
        ("dt",
          po::value<double>(&ModelDefaults::kIntegrationDt)->default_value(1.0),
          "Integration timestep in minutes" )
        ( "last_pred_weight,l",
          po::value<double>(&ModelDefaults::kLastPredictionWeight)->default_value(0.25),
          "Weight of the last prediction (so predictahead of cgms data point),"
          "when finding model paramaters.")
        (
          "target",
          po::value<double>(&ModelDefaults::kTargetBG)->default_value(100.0),
          "Target Blood Glucose Value")
        (
          "lowsigma",
          po::value<double>(&ModelDefaults::kBGLowSigma)->default_value(10.0),
          "Relative weight of BG below Target")
        ("highsigma",
          po::value<double>(&ModelDefaults::kBGHighSigma)->default_value(20.0),
          "Relative weight of BG above Target")
        ("genetic_pop_size",
          po::value<int>(&ModelDefaults::kGenPopSize)->default_value(100),
          "Size of population to genetically optimize with")
        ("genetic_conv_steps",
          po::value<int>(&ModelDefaults::kGenConvergNsteps)->default_value(10),
          "The number generation needed with no improvment, to stop genetic optimiztion")
        ("genetic_nstep_track",
          po::value<int>(&ModelDefaults::kGenNStepMutate)->default_value(6),
          "Number of generation to keep track of")
        ("genetic_ngen_improve",
          po::value<int>(&ModelDefaults::kGenNStepImprove)->default_value(3),
          "The number of generation within last genetic_nstep_track generation that"
          " must improve, or else mutations increased/decreased")
        ("genetic_mutate_sigma",
          po::value<double>(&ModelDefaults::kGenSigmaMult)->default_value(0.5),
          "The mutation multiple")
        ("genetic_convergence_chi2",
          po::value<double>(&ModelDefaults::kGenConvergCriteria)->default_value(1.0),
          "Size of fitness improvment needed in the last genetic_conv_steps so that"
          " minimization will continue");
  
  ns_posDescripton.add( "weight", 0 );  //why is this here?
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


TGNumberEntry *ProgramOptionsGui::addNewEntryField( TGVerticalFrame *parentFrame, 
                                            char *label,
                                            double defaultNumer,
                                            TGNumberFormat::EStyle format,
                                            char *connect )
{
  TGLabel *tglabel = new TGLabel(parentFrame, label);
  tglabel->SetTextJustify(36);
  tglabel->SetMargins(0,0,0,0);
  tglabel->SetWrapLength(-1);
  TGLayoutHints *hint = new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2);
  parentFrame->AddFrame(tglabel, hint);

  TGNumberEntry *entry = new TGNumberEntry(parentFrame, 0.0, 9, -1, format, 
                                           TGNumberFormat::kNEANonNegative);
  
  switch(format)
  {
    case TGNumberFormat::kNESInteger:       // integer number
      entry->SetIntNumber( (Long_t)defaultNumer );
    break;
    
    case TGNumberFormat::kNESRealOne:       // real number with one digit (no exponent)
    case TGNumberFormat::kNESRealTwo:       // real number with two digits (no exponent)
    case TGNumberFormat::kNESRealThree:     // real number with three digits (no exponent)
    case TGNumberFormat::kNESRealFour:      // real number with four digits (no exponent)
    case TGNumberFormat::kNESReal:          // arbitrary real number
      entry->SetNumber( defaultNumer );
    break;
    
    case TGNumberFormat::kNESDegree:        // angle in degree:minutes:seconds format
      assert(0);
    break;
    
    case TGNumberFormat::kNESMinSec:        // time in minutes:seconds format
    case TGNumberFormat::kNESHourMin:       // time in hour:minutes format
    case TGNumberFormat::kNESHourMinSec:    // time in hour:minutes:seconds format
    {
      const TimeDuration td = toTimeDuration( defaultNumer );
      entry->SetTime( td.hours(), td.minutes(), td.seconds() );
      break;
    };//case a time
    
    case TGNumberFormat::kNESDayMYear:      // date in day/month/year format
    case TGNumberFormat::kNESMDayYear:      // date in month/day/year format
      assert(0);
    break;
    
    case TGNumberFormat::kNESHex:
      assert(0);
    break;
  };//switch(format)
  
  parentFrame->AddFrame(entry, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
  if(connect) entry->Connect( "ValueSet(Long_t)", "ProgramOptionsGui", this, connect );
  
  return entry;
}//



ProgramOptionsGui::ProgramOptionsGui( const TGWindow *parentW, UInt_t w, UInt_t h ) 
  : TGCompositeFrame( parentW, w, h - 21)
{
  
  TGHorizontalFrame *horizantalFrame = new TGHorizontalFrame( this, w, h-25, kHorizontalFrame );
  unsigned int frameWdith = (w-10)/4;
  unsigned int frameHeight = h-25;
  
  TGLayoutHints *horizHints = new TGLayoutHints(kLHintsLeft | kLHintsTop,2,8,2,2);
  TGLayoutHints *verticalHints = new TGLayoutHints(kLHintsCenterY | kLHintsCenterX | kLHintsTop | kLHintsExpandY| kLHintsExpandX,1,2,2,2);
  
  
  // vertical frame  
  TGVerticalFrame *personalSettingsVF = new TGVerticalFrame(horizantalFrame,frameWdith,frameHeight,kVerticalFrame);
  TGLabel *columnLabel = new TGLabel(personalSettingsVF, "Personal Settings");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  personalSettingsVF->AddFrame(columnLabel, horizHints);

  
  TGNumberEntry *entry = addNewEntryField( personalSettingsVF, 
                                           "Persons Weight (lbs)",
                                          2.2 * PersonConstants::kPersonsWeight, //2.2 is kg->lbs
                                          TGNumberFormat::kNESInteger, 
                                          "personConstChanged()" );
  
  entry = addNewEntryField( personalSettingsVF, "Basal BG (mg/dl)",
                            PersonConstants::kBasalGlucConc, 
                            TGNumberFormat::kNESInteger,
                            "personConstChanged()" );
  
  horizantalFrame->AddFrame(personalSettingsVF, horizHints);
  
  
  //Okay, now for ones that emit 'modelCalcValueChanged()'  
  TGVerticalFrame *modelCalcVF = new TGVerticalFrame(horizantalFrame,frameWdith,frameHeight,kVerticalFrame);
  
  columnLabel = new TGLabel(modelCalcVF, "Model Calculation");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  modelCalcVF->AddFrame(columnLabel, horizHints);
  
  entry = addNewEntryField( modelCalcVF, "Cgms Delay (mm:ss)",
                            ModelDefaults::kDefaultCgmsDelay, 
                            TGNumberFormat::kNESMinSec,
                            "modelCalcValueChanged()" );

  entry = addNewEntryField( modelCalcVF, "Indiv. Cgms Uncert (%)",
                            100.0*ModelDefaults::kCgmsIndivReadingUncert, 
                            TGNumberFormat::kNESRealOne,
                            "modelCalcValueChanged()" );

  entry = addNewEntryField( modelCalcVF, "Pred. Ahead (hh:mm:ss)",
                            ModelDefaults::kPredictAhead, 
                            TGNumberFormat::kNESHourMinSec,
                            "modelCalcValueChanged()" );
  
  entry = addNewEntryField( modelCalcVF, "Last Pred. Weight",
                            ModelDefaults::kIntegrationDt, 
                            TGNumberFormat::kNESRealTwo,
                            "modelCalcValueChanged()" );

  entry = addNewEntryField( modelCalcVF, "Integ. Dt (mm:ss)",
                            ModelDefaults::kLastPredictionWeight, 
                            TGNumberFormat::kNESMinSec,
                            "modelCalcValueChanged()" );
  horizantalFrame->AddFrame(modelCalcVF, horizHints);
     
  
  TGVerticalFrame *optionVF = new TGVerticalFrame(horizantalFrame, frameWdith, frameHeight, kVerticalFrame);

  columnLabel = new TGLabel(optionVF, "Desired BG");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  optionVF->AddFrame(columnLabel, horizHints);
  
  
  entry = addNewEntryField( optionVF, "Target BG (mg/dl)",
                            ModelDefaults::kTargetBG, 
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  
  entry = addNewEntryField( optionVF, "Below Target Sigma (mg/dl)",
                            ModelDefaults::kBGLowSigma, 
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  
  entry = addNewEntryField( optionVF, "Above Target Sigma (mg/dl)",
                            ModelDefaults::kBGHighSigma, 
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  horizantalFrame->AddFrame( optionVF, horizHints);
  
  
  TGVerticalFrame *geneticVF = new TGVerticalFrame(horizantalFrame, frameWdith, frameHeight, kVerticalFrame);
  columnLabel = new TGLabel(geneticVF, "Genetic Settings");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  geneticVF->AddFrame(columnLabel, horizHints);
  
  entry = addNewEntryField( geneticVF, "Pop. Size",
                            ModelDefaults::kGenPopSize, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  entry = addNewEntryField( geneticVF, "Mutate Conv. NStep",
                            ModelDefaults::kGenNStepMutate, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  entry = addNewEntryField( geneticVF, "Mutate NImprove",
                            ModelDefaults::kGenNStepImprove, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  entry = addNewEntryField( geneticVF, "Mutation Mult.",
                            ModelDefaults::kGenSigmaMult, 
                            TGNumberFormat::kNESRealThree,
                            "modelCalcValueChanged()" );
  
  entry = addNewEntryField( geneticVF, "Conv NStep",
                            ModelDefaults::kGenConvergNsteps, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  entry = addNewEntryField( geneticVF, "Chi2 Conv Criteria",
                            ModelDefaults::kGenConvergNsteps, 
                            TGNumberFormat::kNESRealOne,
                            "modelCalcValueChanged()" );  
  horizantalFrame->AddFrame( geneticVF, horizHints);
  
  AddFrame(horizantalFrame, verticalHints);
}//ProgramOptionsGui



void ProgramOptionsGui::valueChanged()
{
  Emit( "valueChanged()" );
}


void ProgramOptionsGui::optionValueChanged()
{
  // ModelDefaults::kTargetBG
  
  // ModelDefaults::kBGLowSigma
  
  // ModelDefaults::kBGHighSigma
  
  Emit( "optionValueChanged()" );
}//optionValueChanged()




void ProgramOptionsGui::modelCalcValueChanged()
{
  // ModelDefaults::kDefaultCgmsDelay

  // 100.0*ModelDefaults::kCgmsIndivReadingUncert

  // ModelDefaults::kPredictAhead
  
  // ModelDefaults::kIntegrationDt

  // ModelDefaults::kLastPredictionWeight
 
  Emit( "modelCalcValueChanged()" );
}//modelCalcValueChanged()


void ProgramOptionsGui::personConstChanged()
{
   // 2.2 * PersonConstants::kPersonsWeight
  
  // PersonConstants::kBasalGlucConc
  
  Emit( "personConstChanged()" );

}//personConstChanged()



