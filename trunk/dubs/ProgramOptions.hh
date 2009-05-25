#if !defined(PROGRAM_OPTIONS_HH)
#define PROGRAM_OPTIONS_HH

#include <map>

#include "TGWindow.h"
#include "TGFrame.h"
#include "TGNumberEntry.h"

// so I should have the gui in a seperate file, but in the interest of easy
//  programming, I'll do this later (maybe)

//We have to hide anything boost from ROOTS CINT
#ifndef __CINT__ 
#include "boost/program_options.hpp"

namespace ProgramOptions
{
  namespace po = boost::program_options;
  
  extern po::variables_map                   ns_poVariableMap;
  extern po::options_description             ns_poDescription;
  extern po::positional_options_description  ns_posDescripton; //positional decrip
  
  void declareOptions();
  void decodeOptions( int argc, char **argv );
}
#endif //__CINT__

class ProgramOptionsGui : public TGCompositeFrame
{
  public:
    //Pass in  w and h of the parent window
    ProgramOptionsGui( const TGWindow *parentW, UInt_t w, UInt_t h );
    
    TGNumberEntry *addNewEntryField( TGVerticalFrame *parentFrame, 
                                     char *label, double defaultNumer,
                                     TGNumberFormat::EStyle format, 
                                     char *connect = NULL );
    
    //The below both actually change the value of an option
    void valueChanged();           // *SIGNAL*
    void optionValueChanged();     // *SIGNAL*
    void modelCalcValueChanged();  // *SIGNAL*
    void personConstChanged();     // *SIGNAL*
};


#endif //PROGRAM_OPTIONS_HH
