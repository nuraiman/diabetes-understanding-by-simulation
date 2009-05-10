#if !defined(PROGRAM_OPTIONS_HH)
#define PROGRAM_OPTIONS_HH

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

#endif //PROGRAM_OPTIONS_HH
