#if !defined(GENETIC_EVAL_UTILS_HH)
#define GENETIC_EVAL_UTILS_HH


#include "DubsConfig.hh"

#include <vector>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class WtGui;

extern "C"
{
  struct entity_t;
  struct population_t;
  typedef struct entity_t entity;
  typedef struct population_t population;
}//extern "C"



namespace GeneticEvalUtils
{

double perform_optimiation( WtGui *gui,
                          std::vector<boost::posix_time::time_period> timeRanges,
                          boost::function<void (double bestChi2)> genBestCallBackFcn,
                          boost::function<bool (void)> continueFcn
                          );

bool eval_ga_struggle_score( population *pop, entity *dude );
bool seed_initial_parameters( population *pop, entity *adam );
bool generation_start_hook( const int generation, population *pop );
void mutate_ga_params( population *pop, entity *father, entity *son );


struct GenOptData
{
  /*
   *Data struct to hold data for genetic optimization with the GAUL library
   */

  //XXX - m_originalParameters is not currently implemented
  std::vector<double> m_originalParameters;

  std::vector<double> m_generationsBestChi2;

  //We will kinda use similar convergence criteria to the TMVA genetic algorithm
  //  to determine when convergence occurs
  //When the number of improvments within the last m_genNStepMutate
  // a) smaller than m_genNStepImprove, then divide present sigma by m_genSigmaMult
  // b) equal, do nothing
  // c) larger than m_genNStepImprove, then multiply the present sigma by m_genSigmaMult
  //
  //If convergence hasn't improvedmore than m_genConvergCriteria in the last
  //  m_genConvergNsteps, then consider minimization complete
  int m_genConvergNsteps;
  int m_genNStepMutate;
  int m_genNStepImprove;
  double m_genSigma;  //How many 'sigma' the entire range of valid values spans
  double m_genSigmaMult;
  double m_genConvergCriteria;

  WtGui *m_gui;

  std::vector<boost::posix_time::time_period> m_timeRanges;

  //The actual fitness funtion to use
  boost::shared_ptr<ModelTestFCN> m_fittnesFunc;

  //Callback functions to update the GUI
  boost::function<bool (void)> m_shouldCntinueFcn;
  boost::function<void (double bestChi2)> m_bestChi2CalbackFcn;
};//struct GenOptData

}//namespace GeneticEvalUtils

#endif  //GENETIC_EVAL_UTILS_HH
