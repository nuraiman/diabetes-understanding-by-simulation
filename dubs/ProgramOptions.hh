#if !defined(PROGRAM_OPTIONS_HH)
#define PROGRAM_OPTIONS_HH

#include <map>

#include "TGWindow.h"
#include "TGFrame.h"
#include "TGNumberEntry.h"
#include "RQ_OBJECT.h"

// so I should have the gui in a seperate file, but in the interest of easy
//  programming, I'll do this later (maybe)

//We have to hide anything boost from ROOTS CINT
#ifndef __CINT__ 

#include "ArtificialPancrease.hh"
#include "boost/program_options.hpp"

// I would like to add a fucntion to write values to ini file
//  and also make the option vlues a class so indiv. models can customize
namespace ProgramOptions
{
  namespace po = boost::program_options;
  
  extern po::variables_map                   ns_poVariableMap;
  extern po::options_description             ns_poDescription;
  extern po::positional_options_description  ns_posDescripton; //positional decrip
  
  TimeDuration roundToNearestSecond( const TimeDuration &td );
  TimeDuration roundToNearestSecond( const double nMinutes );
  
  
  void declareOptions();
  void decodeOptions( int argc, char **argv );
}
#endif //__CINT__


//This class seems a bit awkward, and is error prone for adding
//  future options into it, so be careful
//Also, THE MAJOR PROBLEM with this class is that it forces 
//  namespace::PersonConstants and namespace::ModelDefaults
//  variables to have the same value as the model you pass in
//  Also, model updating is done via 
//  ProgramOptionsGui::valueChanged(UInt_t bitmask) emitting a signal
//    which this seems less than ideal, but to be worried about later
class NLSimple;
class ProgramOptionsGui : public TGCompositeFrame
{
    RQ_OBJECT("ProgramOptionsGui")
  public:
    enum ProgOptionEnum
    {
      E_kPersonsWeight, E_kBasalGlucConc,
      E_kDefaultCgmsDelay, E_kCgmsIndivReadingUncert,
      E_kPredictAhead, E_kIntegrationDt,
      E_kLastPredictionWeight, E_kTargetBG,
      E_kBGLowSigma, E_kBGHighSigma,
      E_kGenPopSize, E_kGenConvergNsteps,
      E_kGenNStepMutate, E_kGenNStepImprove,
      E_kGenSigmaMult, E_kGenConvergCriteria,
      E_NUMBER_PROGRAM_OPTIONS
    };//enum ProgOptionEnum
  
    NLSimple *m_model;
    std::vector<TGNumberEntry *> m_entries;
    const bool m_debug;
    
    //Pass in  w and h of the parent window, if model!=NULL, then model will
    //  be updated when the gui is changed
    ProgramOptionsGui( const TGWindow *parentW, NLSimple *model, UInt_t w, UInt_t h );
    
    TGNumberEntry *addNewEntryField( TGVerticalFrame *parentFrame, 
                                     const char *label, double defaultNumer,
                                     TGNumberFormat::EStyle format, 
                                     const char *connect = NULL );
        
    //The below both actually change the value of an option
    void valueChanged(UInt_t bitmask); // *SIGNAL*
    void optionValueChanged();         // *SIGNAL*
    void modelCalcValueChanged();      // *SIGNAL*
    void personConstChanged();         // *SIGNAL*
    
    ClassDef(ProgramOptionsGui, 0 );
};


#endif //PROGRAM_OPTIONS_HH
