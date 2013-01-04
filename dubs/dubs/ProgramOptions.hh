#if !defined(PROGRAM_OPTIONS_HH)
#define PROGRAM_OPTIONS_HH

#include "DubsConfig.hh"

#include <map>

// so I should have the gui in a seperate file, but in the interest of easy
//  programming, I'll do this later (maybe)


#include <boost/program_options.hpp>

// include headers that implement a archive in simple text format
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/version.hpp>


#include "dubs/ArtificialPancrease.hh"

namespace ProgramOptions
{
  namespace po = boost::program_options;

  //I want to make 'ProgramOptions' a class, and the below maps static members
  //  and each instance of a NLSimple model will hold it's own ProgramOptions object
  extern po::variables_map                   ns_poVariableMap;
  extern po::options_description             ns_poDescription;
  extern po::positional_options_description  ns_posDescripton; //positional decrip

  TimeDuration roundToNearestSecond( const TimeDuration &td );
  TimeDuration roundToNearestSecond( const double nMinutes );

  void declareOptions();
  void decodeOptions( int argc, char **argv );

  //Below here is variables that may be specified via the command line when
  //  starting the program.  You typically won't need to specify any of them

  //in case you open a Model file imediately from the command line
  //  Can have as first argument, or use with '--file=name', or '-f name'
  extern std::string ns_defaultModelFileName;

  //All variables below here have an equivalent variable in the 'ModelSettings'
  //  class.  'ModelSettings' default value of variables is derived from the
  //  values of the below variables

  extern double kPersonsWeight;          //kg, command line '--weight=78', or '-kg 78'
  extern double kBasalGlucConc;          //

  extern double kCgmsIndivReadingUncert;

  extern TimeDuration kDefaultCgmsDelay; //minutes, ('--cgmsdelay=15')
  extern TimeDuration kPredictAhead;     //default how far to predict ahead of cgms
  extern TimeDuration kIntegrationDt;    //Integration tiemstep in minutes ('--dt=1.0')

  extern double kLastPredictionWeight;   //for calc. chi2 of model ('--last_pred_weight=0.25')

  extern double kTargetBG;               //target blood glucose, ('--target=100')
  extern double kBGLowSigma;             //uncert on low BG, ('--lowsigma=10')
  extern double kBGHighSigma;            //uncert on high BG, ('--lowsigma=20')

  //Genetic minimization paramaters
  extern int kGenPopSize;                //(--genetic_pop_size=100)
  extern int kGenConvergNsteps;          //(--genetic_conv_steps=10)
  extern int kGenNStepMutate;            //(--genetic_nstep_track=6)
  extern int kGenNStepImprove;           //(--genetic_ngen_improve=3)
  extern double kGenSigmaMult;           //(--genetic_mutate_sigma=0.5)
  extern double kGenConvergCriteria;     //(--genetic_convergence_chi2=1.0)
  //When the number of improvments within the last kGenNStepMutate generations is:
  //  a) smaller than kGenNStepImprove, then divide current sigma by kGenSigmaMult
  //  b) equal, do nothing
  //  c) larger than kGenNStepImprove, then multiple current sigma by kGenSigmaMult
  //
  //If convergence hasn't improvedmore than kGenConvergCriteria in the last
  //  kGenConvergNsteps, then consider minimization complete
}//namespace ProgramOptions


class ModelSettings
{
  public:
    ModelSettings();
    ~ModelSettings();


    double m_personsWeight;          //ProgramOptions::kPersonsWeight
    //double m_basalGlucConc;          //ProgramOptions::kBasalGlucConc

    double m_cgmsIndivReadingUncert; //ProgramOptions::kCgmsIndivReadingUncert

    TimeDuration m_defaultCgmsDelay; //ProgramOptions::kDefaultCgmsDelay
    TimeDuration m_cgmsDelay;        //what is actually used for the delay
    TimeDuration m_predictAhead;     //ProgramOptions::kPredictAhead
    TimeDuration m_dt;               //ProgramOptions::kIntegrationDt

    PosixTime m_endTrainingTime;
    PosixTime m_startTrainingTime;


    double m_lastPredictionWeight;   //ProgramOptions::kLastPredictionWeight

    double m_targetBG;               //ProgramOptions::kTargetBG
    double m_bgLowSigma;             //ProgramOptions::kBGLowSigma
    double m_bgHighSigma;            //ProgramOptions::kBGHighSigma

  //Genetic minimization paramaters
    int m_genPopSize;                //ProgramOptions::kGenPopSize
    int m_genConvergNsteps;          //ProgramOptions::kGenConvergNsteps
    int m_genNStepMutate;            //ProgramOptions::kGenNStepMutate
    int m_genNStepImprove;           //ProgramOptions::kGenNStepImprove
    double m_genSigmaMult;           //ProgramOptions::kGenSigmaMult
    double m_genConvergCriteria;     //ProgramOptions::kGenConvergCriteria

    int m_minFingerStickForCharacterization;

    //Will leave serialization funtion in header
    friend class boost::serialization::access;
    template<class Archive>
    void serialize( Archive &ar, const unsigned int /*version*/ )
    {
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

      ar & m_minFingerStickForCharacterization;
    }//serialize
};//class ModelSettings

#endif //PROGRAM_OPTIONS_HH
