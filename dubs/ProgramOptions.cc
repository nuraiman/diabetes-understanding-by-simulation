#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "TGLabel.h"

#include "ArtificialPancrease.hh"
#include "ProgramOptions.hh"
#include "RungeKuttaIntegrater.hh" //for toNMinutes()
#include "ResponseModel.hh"

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
          po::value<double>()->default_value(15.0),
          "Default delay between CGMS and Fingerstick readings" )
        ( "cgms_indiv_uncert", 
          po::value<double>(&ModelDefaults::kCgmsIndivReadingUncert)->default_value(0.025),
          "The uncertainty of one CGMS reading, relative to the" 
          "one immediately before or after it" )
        ( "basal_blood_glucose,g",
          po::value<double>(&PersonConstants::kBasalGlucConc)->default_value(120.0),
          "The target basal blood glucose concentration (mg/dl)")
        ( "predictahead,p",
          po::value<double>()->default_value(45.0),
          "How far ahead of the cgms the model should try to predict, in minutes" )
        ("dt",
          po::value<double>()->default_value(1.0),
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
  
  
  ModelDefaults::kDefaultCgmsDelay = roundToNearestSecond( ns_poVariableMap["cgmsdelay"].as<double>() );
  ModelDefaults::kPredictAhead     = roundToNearestSecond( ns_poVariableMap["dt"].as<double>() );
  ModelDefaults::kIntegrationDt    = roundToNearestSecond( ns_poVariableMap["predictahead"].as<double>() );  
  
  
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
      //We gotta get rounding from a double correct
      const TimeDuration td = ProgramOptions::roundToNearestSecond( defaultNumer );
           
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



ProgramOptionsGui::ProgramOptionsGui( const TGWindow *parentW, NLSimple *model,
                                      UInt_t w, UInt_t h ) 
  : TGCompositeFrame( parentW, w, h - 21), 
    m_model( model ),
    m_entries(E_NUMBER_PROGRAM_OPTIONS,(TGNumberEntry *)NULL ),
    m_debug(false)
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

  
  m_entries[E_kPersonsWeight] = addNewEntryField( personalSettingsVF, 
                                           "Persons Weight (lbs)",
                                          2.2 * PersonConstants::kPersonsWeight, //2.2 is kg->lbs
                                          TGNumberFormat::kNESInteger, 
                                          "personConstChanged()" );
  
  if( m_model ) PersonConstants::kBasalGlucConc = m_model->m_basalGlucoseConcentration;
  m_entries[E_kBasalGlucConc] = addNewEntryField( personalSettingsVF, "Basal BG (mg/dl)",
                            PersonConstants::kBasalGlucConc, 
                            TGNumberFormat::kNESInteger,
                            "personConstChanged()" );
  
  horizantalFrame->AddFrame(personalSettingsVF, horizHints);
  
  
  //Okay, now for ones that emit 'modelCalcValueChanged()'  
  TGVerticalFrame *modelCalcVF = new TGVerticalFrame(horizantalFrame,frameWdith,frameHeight,kVerticalFrame);
  
  columnLabel = new TGLabel(modelCalcVF, "Model Calculation");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  modelCalcVF->AddFrame(columnLabel, horizHints);
  
  if( m_model ) ModelDefaults::kDefaultCgmsDelay = m_model->m_cgmsDelay;
  m_entries[E_kDefaultCgmsDelay] = addNewEntryField( modelCalcVF, "Cgms Delay (mm:ss)",
                            toNMinutes(ModelDefaults::kDefaultCgmsDelay), 
                            TGNumberFormat::kNESMinSec,
                            "modelCalcValueChanged()" );

  m_entries[E_kCgmsIndivReadingUncert] = addNewEntryField( modelCalcVF, "Indiv. Cgms Uncert (%)",
                            100.0*ModelDefaults::kCgmsIndivReadingUncert, 
                            TGNumberFormat::kNESRealOne,
                            "modelCalcValueChanged()" );

  if( m_model ) ModelDefaults::kPredictAhead = m_model->m_predictAhead;
  m_entries[E_kPredictAhead] = addNewEntryField( modelCalcVF, "Pred. Ahead (hh:mm:ss)",
                            toNMinutes(ModelDefaults::kPredictAhead), 
                            TGNumberFormat::kNESHourMinSec,
                            "modelCalcValueChanged()" );
  
  if( m_model ) ModelDefaults::kIntegrationDt = m_model->m_dt;
  m_entries[E_kIntegrationDt] = addNewEntryField( modelCalcVF, "Integ. Dt (mm:ss)",
                            toNMinutes( ModelDefaults::kIntegrationDt ), 
                            TGNumberFormat::kNESMinSec,
                            "modelCalcValueChanged()" );

  m_entries[E_kLastPredictionWeight] = addNewEntryField( modelCalcVF, "Last Pred. Weight",
                            ModelDefaults::kLastPredictionWeight, 
                            TGNumberFormat::kNESRealTwo,
                            "modelCalcValueChanged()" );
  horizantalFrame->AddFrame(modelCalcVF, horizHints);
     
  
  TGVerticalFrame *optionVF = new TGVerticalFrame(horizantalFrame, frameWdith, frameHeight, kVerticalFrame);

  columnLabel = new TGLabel(optionVF, "Desired BG");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  optionVF->AddFrame(columnLabel, horizHints);
  
  
  m_entries[E_kTargetBG] = addNewEntryField( optionVF, "Target BG (mg/dl)",
                            ModelDefaults::kTargetBG, 
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  
  m_entries[E_kBGLowSigma] = addNewEntryField( optionVF, "Below Target Sigma (mg/dl)",
                            ModelDefaults::kBGLowSigma, 
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  
  m_entries[E_kBGHighSigma] = addNewEntryField( optionVF, "Above Target Sigma (mg/dl)",
                            ModelDefaults::kBGHighSigma, 
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  horizantalFrame->AddFrame( optionVF, horizHints);
  
  
  TGVerticalFrame *geneticVF = new TGVerticalFrame(horizantalFrame, frameWdith, frameHeight, kVerticalFrame);
  columnLabel = new TGLabel(geneticVF, "Genetic Settings");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  geneticVF->AddFrame(columnLabel, horizHints);
  
  m_entries[E_kGenPopSize] = addNewEntryField( geneticVF, "Pop. Size",
                            ModelDefaults::kGenPopSize, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  m_entries[E_kGenNStepMutate] = addNewEntryField( geneticVF, "Mutate Conv. NStep",
                            ModelDefaults::kGenNStepMutate, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  m_entries[E_kGenNStepImprove] = addNewEntryField( geneticVF, "Mutate NImprove",
                            ModelDefaults::kGenNStepImprove, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  m_entries[E_kGenSigmaMult] = addNewEntryField( geneticVF, "Mutation Mult.",
                            ModelDefaults::kGenSigmaMult, 
                            TGNumberFormat::kNESRealThree,
                            "modelCalcValueChanged()" );
  
  m_entries[E_kGenConvergNsteps] = addNewEntryField( geneticVF, "Conv NStep",
                            ModelDefaults::kGenConvergNsteps, 
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  
  m_entries[E_kGenConvergCriteria] = addNewEntryField( geneticVF, "Chi2 Conv Criteria",
                            ModelDefaults::kGenConvergCriteria, 
                            TGNumberFormat::kNESRealOne,
                            "modelCalcValueChanged()" );  

  horizantalFrame->AddFrame( geneticVF, horizHints);
  
  AddFrame(horizantalFrame, verticalHints);
}//ProgramOptionsGui



void ProgramOptionsGui::valueChanged(UInt_t bitmask)
{
  assert(bitmask);  //should only be called for a reason
  
  if( m_debug ) cout << "Emitting valueChanged(0x" << hex << bitmask << dec << ")" << endl;
  
  Emit( "valueChanged(UInt_t)", bitmask );
}//ProgramOptionsGui::valueChanged()





void ProgramOptionsGui::optionValueChanged()
{
  unsigned int changedPar = 0x0;
  
  double newTargetBG = m_entries[E_kTargetBG]->GetNumber();
  if( newTargetBG != ModelDefaults::kTargetBG ) 
  {
    changedPar |= ( 0x1 << E_kTargetBG );
    ModelDefaults::kTargetBG = newTargetBG;
    if(m_debug) cout << "Changed kTargetBG to " << newTargetBG << endl;
  }//if( newTargetBG != ModelDefaults::kTargetBG ) 
  
  double newLowSigma = m_entries[E_kBGLowSigma]->GetNumber();
  if( newLowSigma != ModelDefaults::kBGLowSigma ) 
  {
    changedPar |= ( 0x1 << E_kBGLowSigma );
    ModelDefaults::kBGLowSigma = newLowSigma;
    if(m_debug) cout << "Changed kBGLowSigma to " << newLowSigma << endl;
  }//if( newTargetBG != ModelDefaults::kTargetBG ) 
  

  double newHighSigma = m_entries[E_kBGHighSigma]->GetNumber();
  if( newHighSigma != ModelDefaults::kBGHighSigma ) 
  {
    changedPar |= ( 0x1 << E_kBGHighSigma );
    ModelDefaults::kBGHighSigma = newHighSigma;
    if(m_debug) cout << "Changed kBGLowSigma to " << newHighSigma << endl;
  }//if( newTargetBG != ModelDefaults::kTargetBG ) 
  

  if(m_debug)  cout << "optionValueChanged()" << hex << changedPar << dec << endl;
  
  valueChanged(changedPar);
  
  
  Emit( "optionValueChanged()" );
}//optionValueChanged()



void ProgramOptionsGui::modelCalcValueChanged()
{
  unsigned int changedPar = 0x0;
  
  int hour, min, sec;
  m_entries[E_kDefaultCgmsDelay]->GetTime(hour, min, sec);
  const TimeDuration newCgmsDelay( hour, min, sec, 0);
  
  if( ModelDefaults::kDefaultCgmsDelay != newCgmsDelay )
  {
    changedPar |= ( 0x1 << E_kDefaultCgmsDelay );
    ModelDefaults::kDefaultCgmsDelay = newCgmsDelay;
    if(m_debug) cout << "Changed kDefaultCgmsDelay to " << newCgmsDelay << endl;
  }//if( oldCgmsDelay != newCgmsDelay )
  

  double oldReadingUncert = ModelDefaults::kCgmsIndivReadingUncert;
  double newReadingUncert = m_entries[E_kCgmsIndivReadingUncert]->GetNumber();
  newReadingUncert /= 100.0;
  
  if( oldReadingUncert != newReadingUncert )
  {
    changedPar |= ( 0x1 << E_kCgmsIndivReadingUncert );
    ModelDefaults::kCgmsIndivReadingUncert = newReadingUncert;
    if(m_debug) cout << "Changed kCgmsIndivReadingUncert to " << newReadingUncert << endl;
  }//if( oldReadingUncert != newReadingUncert )

  m_entries[E_kPredictAhead]->GetTime(hour, min, sec);
  const TimeDuration newPredAhead( hour, min, sec, 0);
  
  if( ModelDefaults::kPredictAhead != newPredAhead )
  {
    changedPar |= ( 0x1 << E_kPredictAhead );
    ModelDefaults::kPredictAhead = newPredAhead;
    if(m_debug) cout << "Changed kPredictAhead to " << newPredAhead << endl;
  }//if( oldPredAhead != newPredAhead )
  
  m_entries[E_kIntegrationDt]->GetTime(hour, min, sec);
  const TimeDuration newDt( hour, min, sec, 0);
  
  if( ModelDefaults::kIntegrationDt != newDt )
  {
    changedPar |= ( 0x1 << E_kIntegrationDt );
    ModelDefaults::kIntegrationDt = newDt;
    if(m_debug) cout << "Changed kIntegrationDt to " << newDt << endl;
  }//if( oldDt != newDt )
   

  double oldWeight = ModelDefaults::kLastPredictionWeight;
  double newWeight = m_entries[E_kLastPredictionWeight]->GetNumber();
  
  if( oldWeight != newWeight )
  {
    changedPar |= ( 0x1 << E_kLastPredictionWeight );
    ModelDefaults::kLastPredictionWeight = newWeight;
    if(m_debug) cout << "Changed kLastPredictionWeight to " << newWeight << endl;
  }//if( oldWeight != newWeight )
 
  
  if( ModelDefaults::kGenPopSize != m_entries[E_kGenPopSize]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenPopSize );
    ModelDefaults::kGenPopSize = m_entries[E_kGenPopSize]->GetIntNumber();
    if(m_debug) cout << "Changed kGenPopSize to " << ModelDefaults::kGenPopSize << endl;
  }//

  if( ModelDefaults::kGenConvergNsteps != m_entries[E_kGenConvergNsteps]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenConvergNsteps );
    ModelDefaults::kGenConvergNsteps = m_entries[E_kGenConvergNsteps]->GetIntNumber();
    if(m_debug) cout << "Changed kGenConvergNsteps to " << ModelDefaults::kGenConvergNsteps << endl;
  }//
    
  if( ModelDefaults::kGenNStepMutate != m_entries[E_kGenNStepMutate]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenNStepMutate );
    ModelDefaults::kGenNStepMutate = m_entries[E_kGenNStepMutate]->GetIntNumber();
    if(m_debug) cout << "Changed kGenNStepMutate to " << ModelDefaults::kGenNStepMutate << endl;
  }//
  
  if( ModelDefaults::kGenNStepImprove != m_entries[E_kGenNStepImprove]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenNStepImprove );
    ModelDefaults::kGenNStepImprove = m_entries[E_kGenNStepImprove]->GetIntNumber();
    if(m_debug) cout << "Changed kGenNStepImprove to " << ModelDefaults::kGenNStepImprove << endl;
  }//
  
  if( ModelDefaults::kGenSigmaMult != m_entries[E_kGenSigmaMult]->GetNumber() )
  {
    changedPar |= ( 0x1 << E_kGenSigmaMult );
    ModelDefaults::kGenSigmaMult = m_entries[E_kGenSigmaMult]->GetNumber();
    if(m_debug) cout << "Changed kGenSigmaMult to " << ModelDefaults::kGenSigmaMult << endl;
  }//
  
  if( ModelDefaults::kGenConvergCriteria != m_entries[E_kGenConvergCriteria]->GetNumber() )
  {
    changedPar |= ( 0x1 << E_kGenConvergCriteria );
    ModelDefaults::kGenConvergCriteria = m_entries[E_kGenConvergCriteria]->GetNumber();
    if(m_debug) cout << "Changed kGenConvergCriteria to " << ModelDefaults::kGenConvergCriteria << endl;
  }//
      
  
  if(m_debug) cout << "modelCalcValueChanged() " << hex << changedPar << dec << endl;
  
  valueChanged(changedPar);
  Emit( "modelCalcValueChanged()" );
}//modelCalcValueChanged()


void ProgramOptionsGui::personConstChanged()
{
  unsigned int changedPar = 0x0;
  
  double newWeight = m_entries[E_kPersonsWeight]->GetNumber() / 2.2; //2.2 is for lbs -> kg
  if( newWeight != PersonConstants::kPersonsWeight ) 
  {
    changedPar |= ( 0x1 << E_kPersonsWeight );
    PersonConstants::kPersonsWeight = newWeight;  
    if(m_debug) cout << "Changed kPersonsWeight to " << 2.2 * newWeight << endl;
  }//if( newTargetBG != ModelDefaults::kTargetBG ) 
  
  
  double newBasal = m_entries[E_kBasalGlucConc]->GetNumber();
  if( newBasal != PersonConstants::kBasalGlucConc ) 
  {
    changedPar |= ( 0x1 << E_kBasalGlucConc );
    PersonConstants::kBasalGlucConc = newBasal;  
    if(m_debug) cout << "Changed kBasalGlucConc to " << newBasal << endl;
  }//if( newTargetBG != ModelDefaults::kTargetBG ) 
  
  
  valueChanged(changedPar);
  if(m_debug) cout << "personConstChanged() " << hex << changedPar << dec << endl;
    
  Emit( "personConstChanged()" );

}//personConstChanged()



