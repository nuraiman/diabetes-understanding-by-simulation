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
}//ModelSettings::ModelSettings()

ModelSettings::~ModelSettings(){}

template<class Archive>
void ModelSettings::serialize( Archive &ar, const unsigned int version )
{
  unsigned int ver = version; //keep compiler from complaining
  ver = ver;

  ar & m_personsWeight;
  //ar & m_basalGlucConc;

  ar & m_cgmsIndivReadingUncert;

  ar & m_defaultCgmsDelay;
  ar & m_cgmsDelay;
  ar & m_predictAhead;
  ar & m_dt;

  ar & m_endTrainingTime;
  ar & m_startTrainingTime;

  ar & m_lastPredictionWeight;

  ar & m_targetBG;
  ar & m_bgLowSigma;
  ar & m_bgHighSigma;

  //Genetic minimization paramaters
  ar & m_genPopSize;
  ar & m_genConvergNsteps;
  ar & m_genNStepMutate;
  ar & m_genNStepImprove;
  ar & m_genSigmaMult;
  ar & m_genConvergCriteria;
}//serialize






TGNumberEntry *ProgramOptionsGui::addNewEntryField( TGVerticalFrame *parentFrame,
                                            const char *label,
                                            double defaultNumer,
                                            TGNumberFormat::EStyle format,
                                            const char *connect )
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
                                          2.2 * ProgramOptions::kPersonsWeight, //2.2 is kg->lbs
                                          TGNumberFormat::kNESInteger,
                                          "personConstChanged()" );

  if( m_model ) ProgramOptions::kBasalGlucConc = m_model->m_basalGlucoseConcentration;
  m_entries[E_kBasalGlucConc] = addNewEntryField( personalSettingsVF, "Basal BG (mg/dl)",
                            ProgramOptions::kBasalGlucConc,
                            TGNumberFormat::kNESInteger,
                            "personConstChanged()" );

  horizantalFrame->AddFrame(personalSettingsVF, horizHints);


  //Okay, now for ones that emit 'modelCalcValueChanged()'
  TGVerticalFrame *modelCalcVF = new TGVerticalFrame(horizantalFrame,frameWdith,frameHeight,kVerticalFrame);

  columnLabel = new TGLabel(modelCalcVF, "Model Calculation");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  modelCalcVF->AddFrame(columnLabel, horizHints);

  if( m_model ) ProgramOptions::kDefaultCgmsDelay = m_model->m_settings.m_cgmsDelay;
  m_entries[E_kDefaultCgmsDelay] = addNewEntryField( modelCalcVF, "Cgms Delay (mm:ss)",
                            toNMinutes(ProgramOptions::kDefaultCgmsDelay),
                            TGNumberFormat::kNESMinSec,
                            "modelCalcValueChanged()" );

  m_entries[E_kCgmsIndivReadingUncert] = addNewEntryField( modelCalcVF, "Indiv. Cgms Uncert (%)",
                            100.0*ProgramOptions::kCgmsIndivReadingUncert,
                            TGNumberFormat::kNESRealOne,
                            "modelCalcValueChanged()" );

  if( m_model ) ProgramOptions::kPredictAhead = m_model->m_settings.m_predictAhead;
  m_entries[E_kPredictAhead] = addNewEntryField( modelCalcVF, "Pred. Ahead (hh:mm:ss)",
                            toNMinutes(ProgramOptions::kPredictAhead),
                            TGNumberFormat::kNESHourMinSec,
                            "modelCalcValueChanged()" );

  if( m_model ) ProgramOptions::kIntegrationDt = m_model->m_settings.m_dt;
  m_entries[E_kIntegrationDt] = addNewEntryField( modelCalcVF, "Integ. Dt (mm:ss)",
                            toNMinutes( ProgramOptions::kIntegrationDt ),
                            TGNumberFormat::kNESMinSec,
                            "modelCalcValueChanged()" );

  m_entries[E_kLastPredictionWeight] = addNewEntryField( modelCalcVF, "Last Pred. Weight",
                            ProgramOptions::kLastPredictionWeight,
                            TGNumberFormat::kNESRealTwo,
                            "modelCalcValueChanged()" );
  horizantalFrame->AddFrame(modelCalcVF, horizHints);


  TGVerticalFrame *optionVF = new TGVerticalFrame(horizantalFrame, frameWdith, frameHeight, kVerticalFrame);

  columnLabel = new TGLabel(optionVF, "Desired BG");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  optionVF->AddFrame(columnLabel, horizHints);


  m_entries[E_kTargetBG] = addNewEntryField( optionVF, "Target BG (mg/dl)",
                            ProgramOptions::kTargetBG,
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );

  m_entries[E_kBGLowSigma] = addNewEntryField( optionVF, "Below Target Sigma (mg/dl)",
                            ProgramOptions::kBGLowSigma,
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );

  m_entries[E_kBGHighSigma] = addNewEntryField( optionVF, "Above Target Sigma (mg/dl)",
                            ProgramOptions::kBGHighSigma,
                            TGNumberFormat::kNESInteger,
                            "optionValueChanged()" );
  horizantalFrame->AddFrame( optionVF, horizHints);


  TGVerticalFrame *geneticVF = new TGVerticalFrame(horizantalFrame, frameWdith, frameHeight, kVerticalFrame);
  columnLabel = new TGLabel(geneticVF, "Genetic Settings");
  columnLabel->SetTextFont("-adobe-courier-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
  geneticVF->AddFrame(columnLabel, horizHints);

  m_entries[E_kGenPopSize] = addNewEntryField( geneticVF, "Pop. Size",
                            ProgramOptions::kGenPopSize,
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  m_entries[E_kGenNStepMutate] = addNewEntryField( geneticVF, "Mutate Conv. NStep",
                            ProgramOptions::kGenNStepMutate,
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  m_entries[E_kGenNStepImprove] = addNewEntryField( geneticVF, "Mutate NImprove",
                            ProgramOptions::kGenNStepImprove,
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );
  m_entries[E_kGenSigmaMult] = addNewEntryField( geneticVF, "Mutation Mult.",
                            ProgramOptions::kGenSigmaMult,
                            TGNumberFormat::kNESRealThree,
                            "modelCalcValueChanged()" );

  m_entries[E_kGenConvergNsteps] = addNewEntryField( geneticVF, "Conv NStep",
                            ProgramOptions::kGenConvergNsteps,
                            TGNumberFormat::kNESInteger,
                            "modelCalcValueChanged()" );

  m_entries[E_kGenConvergCriteria] = addNewEntryField( geneticVF, "Chi2 Conv Criteria",
                            ProgramOptions::kGenConvergCriteria,
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
  if( newTargetBG != ProgramOptions::kTargetBG )
  {
    changedPar |= ( 0x1 << E_kTargetBG );
    ProgramOptions::kTargetBG = newTargetBG;
    if(m_debug) cout << "Changed kTargetBG to " << newTargetBG << endl;
  }//if( newTargetBG != ProgramOptions::kTargetBG )

  double newLowSigma = m_entries[E_kBGLowSigma]->GetNumber();
  if( newLowSigma != ProgramOptions::kBGLowSigma )
  {
    changedPar |= ( 0x1 << E_kBGLowSigma );
    ProgramOptions::kBGLowSigma = newLowSigma;
    if(m_debug) cout << "Changed kBGLowSigma to " << newLowSigma << endl;
  }//if( newTargetBG != ProgramOptions::kTargetBG )


  double newHighSigma = m_entries[E_kBGHighSigma]->GetNumber();
  if( newHighSigma != ProgramOptions::kBGHighSigma )
  {
    changedPar |= ( 0x1 << E_kBGHighSigma );
    ProgramOptions::kBGHighSigma = newHighSigma;
    if(m_debug) cout << "Changed kBGLowSigma to " << newHighSigma << endl;
  }//if( newTargetBG != ProgramOptions::kTargetBG )


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

  if( ProgramOptions::kDefaultCgmsDelay != newCgmsDelay )
  {
    changedPar |= ( 0x1 << E_kDefaultCgmsDelay );
    ProgramOptions::kDefaultCgmsDelay = newCgmsDelay;
    if(m_debug) cout << "Changed kDefaultCgmsDelay to " << newCgmsDelay << endl;
  }//if( oldCgmsDelay != newCgmsDelay )


  double oldReadingUncert = ProgramOptions::kCgmsIndivReadingUncert;
  double newReadingUncert = m_entries[E_kCgmsIndivReadingUncert]->GetNumber();
  newReadingUncert /= 100.0;

  if( oldReadingUncert != newReadingUncert )
  {
    changedPar |= ( 0x1 << E_kCgmsIndivReadingUncert );
    ProgramOptions::kCgmsIndivReadingUncert = newReadingUncert;
    if(m_debug) cout << "Changed kCgmsIndivReadingUncert to " << newReadingUncert << endl;
  }//if( oldReadingUncert != newReadingUncert )

  m_entries[E_kPredictAhead]->GetTime(hour, min, sec);
  const TimeDuration newPredAhead( hour, min, sec, 0);

  if( ProgramOptions::kPredictAhead != newPredAhead )
  {
    changedPar |= ( 0x1 << E_kPredictAhead );
    ProgramOptions::kPredictAhead = newPredAhead;
    if(m_debug) cout << "Changed kPredictAhead to " << newPredAhead << endl;
  }//if( oldPredAhead != newPredAhead )

  m_entries[E_kIntegrationDt]->GetTime(hour, min, sec);
  const TimeDuration newDt( hour, min, sec, 0);

  if( ProgramOptions::kIntegrationDt != newDt )
  {
    changedPar |= ( 0x1 << E_kIntegrationDt );
    ProgramOptions::kIntegrationDt = newDt;
    if(m_debug) cout << "Changed kIntegrationDt to " << newDt << endl;
  }//if( oldDt != newDt )


  double oldWeight = ProgramOptions::kLastPredictionWeight;
  double newWeight = m_entries[E_kLastPredictionWeight]->GetNumber();

  if( oldWeight != newWeight )
  {
    changedPar |= ( 0x1 << E_kLastPredictionWeight );
    ProgramOptions::kLastPredictionWeight = newWeight;
    if(m_debug) cout << "Changed kLastPredictionWeight to " << newWeight << endl;
  }//if( oldWeight != newWeight )


  if( ProgramOptions::kGenPopSize != m_entries[E_kGenPopSize]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenPopSize );
    ProgramOptions::kGenPopSize = m_entries[E_kGenPopSize]->GetIntNumber();
    if(m_debug) cout << "Changed kGenPopSize to " << ProgramOptions::kGenPopSize << endl;
  }//

  if( ProgramOptions::kGenConvergNsteps != m_entries[E_kGenConvergNsteps]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenConvergNsteps );
    ProgramOptions::kGenConvergNsteps = m_entries[E_kGenConvergNsteps]->GetIntNumber();
    if(m_debug) cout << "Changed kGenConvergNsteps to " << ProgramOptions::kGenConvergNsteps << endl;
  }//

  if( ProgramOptions::kGenNStepMutate != m_entries[E_kGenNStepMutate]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenNStepMutate );
    ProgramOptions::kGenNStepMutate = m_entries[E_kGenNStepMutate]->GetIntNumber();
    if(m_debug) cout << "Changed kGenNStepMutate to " << ProgramOptions::kGenNStepMutate << endl;
  }//

  if( ProgramOptions::kGenNStepImprove != m_entries[E_kGenNStepImprove]->GetIntNumber() )
  {
    changedPar |= ( 0x1 << E_kGenNStepImprove );
    ProgramOptions::kGenNStepImprove = m_entries[E_kGenNStepImprove]->GetIntNumber();
    if(m_debug) cout << "Changed kGenNStepImprove to " << ProgramOptions::kGenNStepImprove << endl;
  }//

  if( ProgramOptions::kGenSigmaMult != m_entries[E_kGenSigmaMult]->GetNumber() )
  {
    changedPar |= ( 0x1 << E_kGenSigmaMult );
    ProgramOptions::kGenSigmaMult = m_entries[E_kGenSigmaMult]->GetNumber();
    if(m_debug) cout << "Changed kGenSigmaMult to " << ProgramOptions::kGenSigmaMult << endl;
  }//

  if( ProgramOptions::kGenConvergCriteria != m_entries[E_kGenConvergCriteria]->GetNumber() )
  {
    changedPar |= ( 0x1 << E_kGenConvergCriteria );
    ProgramOptions::kGenConvergCriteria = m_entries[E_kGenConvergCriteria]->GetNumber();
    if(m_debug) cout << "Changed kGenConvergCriteria to " << ProgramOptions::kGenConvergCriteria << endl;
  }//


  if(m_debug) cout << "modelCalcValueChanged() " << hex << changedPar << dec << endl;

  valueChanged(changedPar);
  Emit( "modelCalcValueChanged()" );
}//modelCalcValueChanged()


void ProgramOptionsGui::personConstChanged()
{
  unsigned int changedPar = 0x0;

  double newWeight = m_entries[E_kPersonsWeight]->GetNumber() / 2.2; //2.2 is for lbs -> kg
  if( newWeight != ProgramOptions::kPersonsWeight )
  {
    changedPar |= ( 0x1 << E_kPersonsWeight );
    ProgramOptions::kPersonsWeight = newWeight;
    if(m_debug) cout << "Changed kPersonsWeight to " << 2.2 * newWeight << endl;
  }//if( newTargetBG != ProgramOptions::kTargetBG )


  double newBasal = m_entries[E_kBasalGlucConc]->GetNumber();
  if( newBasal != ProgramOptions::kBasalGlucConc )
  {
    changedPar |= ( 0x1 << E_kBasalGlucConc );
    ProgramOptions::kBasalGlucConc = newBasal;
    if(m_debug) cout << "Changed kBasalGlucConc to " << newBasal << endl;
  }//if( newTargetBG != ProgramOptions::kTargetBG )


  valueChanged(changedPar);
  if(m_debug) cout << "personConstChanged() " << hex << changedPar << dec << endl;

  Emit( "personConstChanged()" );

}//personConstChanged()



