#include "DubsConfig.hh"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <stdio.h>
#include <math.h>  //contains M_PI
#include <stdlib.h>
#include <fstream>
#include <algorithm> //min/max_element
#include <float.h> // for DBL_MAX

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/assign/list_of.hpp> //for 'list_of()'
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/serialization/version.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include <Wt/WApplication>

#include "external_libs/spline/spline.hpp"

#include "dubs/WtGui.hh"
#include "dubs/ResponseModel.hh"
#include "dubs/KineticModels.hh"
#include "dubs/CgmsDataImport.hh"
#include "dubs/GeneticEvalUtils.hh"
#include "dubs/ConsentrationGraph.hh"
#include "dubs/ArtificialPancrease.hh"
#include "dubs/RungeKuttaIntegrater.hh"


extern "C"
{
#include "gaul.h"
}

using namespace std;
using namespace boost;
using namespace boost::posix_time;


//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH





namespace GeneticEvalUtils
{


double perform_optimiation( WtGui *gui,
                          std::vector<TimeRange> timeRanges,
                          boost::function<void (double bestChi2)> genBestCallBackFcn,
                          boost::function<bool (void)> continueFcn
                          )
{
  boost::shared_ptr<NLSimple> model;

  {
    const string error_msg = "Failed to get thread lock at " + SRC_LOCATION;
    NLSimplePtr guismodel( gui, true, error_msg );
    model.reset( new NLSimple(*guismodel) );
  }

  cerr << "Performing genetic optiiztion" << endl;

  timeRanges = model->getNonExcludedTimeRanges( timeRanges );
  const ModelSettings &settings = model->settings();

  const double endPredChi2Weight = settings.m_lastPredictionWeight;

  boost::shared_ptr<ModelTestFCN> fittnesFunc;
  fittnesFunc.reset( new ModelTestFCN( model.get(), endPredChi2Weight, timeRanges ) );
  fittnesFunc->SetErrorDef(10.0);

  GenOptData dataForOpt;
  dataForOpt.m_gui                = gui;
  dataForOpt.m_timeRanges         = timeRanges;
  dataForOpt.m_shouldCntinueFcn   = continueFcn;
  dataForOpt.m_bestChi2CalbackFcn = genBestCallBackFcn;
  dataForOpt.m_originalParameters = model->paramaters();
  dataForOpt.m_genConvergNsteps   = settings.m_genConvergNsteps;
  dataForOpt.m_genNStepMutate     = settings.m_genNStepMutate;
  dataForOpt.m_genNStepImprove    = settings.m_genNStepImprove;
  dataForOpt.m_genSigmaMult       = settings.m_genSigmaMult;
  dataForOpt.m_genConvergCriteria = settings.m_genConvergCriteria;
  dataForOpt.m_fittnesFunc        = fittnesFunc;
  dataForOpt.m_genSigma           = 10.0;


  //Get ready for optimization
  model->paramaters().clear();
  model->paramaterErrorPlus().clear();
  model->paramaterErrorMinus().clear();

  model->resetPredictions();
  model->findSteadyStateBeginings(3);

  const PTimeVec &startSteadyStateTimes = model->startSteadyStateTimes();

  cerr << "There are " << startSteadyStateTimes.size()
       << " Steady state starting times" << endl;

  const ConsentrationGraph &customEvents = model->customEvents();
  const NLSimple::EventDefMap &customEventDefs = model->customEventDefs();


  size_t nParExp = (size_t)NLSimple::NumNLSimplePars;
  foreach( const NLSimple::EventDefPair &et, customEventDefs )
  {
    if( customEvents.hasValueNear( et.first ) )
      nParExp += et.second.getNPoints();
  }//foreach( custom event def )

  int len_chromo = int(NLSimple::NumNLSimplePars);
  size_t parNum = (size_t)NLSimple::NumNLSimplePars;

  foreach( const NLSimple::EventDefPair &et, customEventDefs )
  {
    if( !customEvents.hasValueNear( et.first ) )
      continue;

    const size_t nPoints = et.second.getNPoints();
    for( size_t pointN = 0; pointN < nPoints; ++pointN, ++parNum )
    {
      cout << "Adding paramater " << pointN << " for Custom Event named "
           << et.second.getName() << " for genetic minization" << endl;
      ++len_chromo;
    }//for
  }//foreach( custom event def )

  random_seed(2003);

  const int population_size = settings.m_genPopSize;
  const int num_chromo = 1;
  GAgeneration_hook generation_hook = GeneticEvalUtils::generation_start_hook;
  GAiteration_hook iteration_hook = NULL;
  GAdata_destructor data_destructor = NULL;
  GAdata_ref_incrementor data_ref_incrementor = NULL;
  GAevaluate evaluate = GeneticEvalUtils::eval_ga_struggle_score;
  GAseed seed = GeneticEvalUtils::seed_initial_parameters;
  GAadapt adapt = NULL;
  GAselect_one select_one = ga_select_one_sus; //Selects individual to mutate using Stochastic Universal Sampling selection; fitness values defined as 0.0 is bad, and large positive values are good
  GAselect_two select_two = ga_select_two_sus; //Selects two individuals to mate using Stochastic Universal Sampling selection; fitness values defined as 0.0 is bad, and large positive values are good
  GAmutate mutate = GeneticEvalUtils::mutate_ga_params; //ga_mutate_double_singlepoint_drift; //ga_mutate_double_singlepoint_randomize, ga_mutate_double_multipoint, ga_mutate_double_allpoint
  GAcrossover crossover_type = ga_crossover_double_allele_mixing;  //gives the 'son' and 'daughter' a parameter from either the 'mother' or 'father'
  GAreplace replace = NULL;
  vpointer userdata = (void *)&dataForOpt;

  population *pop = ga_genesis_double( population_size, num_chromo, len_chromo,
                                       generation_hook, iteration_hook,
                                       data_destructor, data_ref_incrementor,
                                       evaluate, seed, adapt, select_one,
                                       select_two, mutate, crossover_type,
                                       replace, userdata );

  const ga_scheme_type scheme = GA_SCHEME_DARWIN;

//http://gaul.sourceforge.net/tutorial/elitism.html:
//  GA_ELITISM_PARENTS_SURVIVE      All parents that rank sufficiently highly will pass to the next generation.
//  GA_ELITISM_ONE_PARENT_SURVIVES	The single fittest parent will pass to the next generation if it ranks sufficiently well.
//  GA_ELITISM_PARENTS_DIE          No parents pass to next generation, regardless of their fitness.
//  GA_ELITISM_RESCORE_PARENTS      All parents are re-evalutated, and those that subsequently rank sufficiently highly will pass to the next generation.
  const ga_elitism_type elitism = GA_ELITISM_ONE_PARENT_SURVIVES;

  const double crossover = 0.9;
  const double mutation  = 0.2;
  const double migration = 0.0;
  ga_population_set_parameters( pop, scheme, elitism, crossover, mutation, migration );

  const int max_generations = 10;
  ga_evolution( pop, max_generations );

//  setenv( GA_NUM_THREADS_ENVVAR_STRING, "2", 1 );
//  ga_evolution_threaded( pop, max_generations );

  entity *best = ga_get_entity_from_rank( pop, 0 );
  double bestchi2 = 1.0 / best->fitness;
  const int len = pop->len_chromosomes;
  double *chromosomes = (double *)best->chromosome[0];

  vector<double> genes( chromosomes, chromosomes + len );

  {
    const string error_msg = "Failed to get thread after optimization at "
                             + SRC_LOCATION;

    NLSimplePtr guismodel( gui, true, error_msg );
    guismodel->setModelParameters( genes );

    cout << "Done with Genetic optimization" << endl;
    cout << "Final paramaters are: ";
    foreach( double d, genes )
      cout << d << "  ";
    cout << endl;

    guismodel->findSteadyStateBeginings( 3.0 );
    guismodel->updateXUsingCgmsInfo();

    timeRanges = guismodel->getNonExcludedTimeRanges();
    ModelTestFCN fitFcn( guismodel.get(), endPredChi2Weight, timeRanges );

    fitFcn.testParamaters( genes, true );
  }

  ga_extinction( pop );

  return bestchi2;
}//double perform_optimiation(...)


// Called at the beginning of each generation
bool generation_start_hook( const int generation, population *pop )
{
  /*
   *From GAUL documentation, this function is called at the beginning of each
   *  generation by all evolutionary functions. If this callback function
   *  returns FALSE the evolution will terminate.
   *From what I can tell, this function is called with generation==0, after
   *  evaluating
   */

  GenOptData *data = (GenOptData *)pop->data;
//  boost::shared_ptr<ModelTestFCN> fitFunc = data->m_fittnesFunc;

  if( data->m_shouldCntinueFcn && !data->m_shouldCntinueFcn() )
  {
    //XXX - should set the best found parameters so far to the model.
    cerr << "User requested minimization stoping" << endl;
    return false;
  }//if( data->m_shouldCntinueFcn && !data->m_shouldCntinueFcn() )


  cerr << "on generation " << generation
       << ", pop size=" << pop->size << endl;

//  if( generation == 0 )
//  {
    /*
    if( data->m_originalParameters.size() )
    {
      cerr << "Assigning original parameters to an individual" << endl;
      const int npop = pop->size;

      if( npop )
      {
        entity *first = pop->entity_array[0];
//      entity *first = ga_get_entity_from_id( pop, 0 );
        assert( first );
        double *chromosome = ((double *)(first->chromosome[0]));
        const size_t len = static_cast<size_t>( pop->len_chromosomes );
        if( len == data->m_originalParameters.size() )
          memcpy(chromosome, &(data->m_originalParameters[0]), len*sizeof(double));
        else
          cerr << "Warning: chormosome length mismatch" << endl;
      }//if( npop )
    }//if( data->m_originalParameters.size() )
    */
//    return true;
//  }//if( generation == 0 )

  const int npop = pop->size;
  entity **entity_iarray = pop->entity_iarray; /* The population sorted by fitness. */
  for( int n = 0; n < npop; ++n )
  {
    entity *dude = entity_iarray[n];
    cerr << "\tEntity " << n << " has fitness " << dude->fitness << endl;
  }//for( int n = 0; n < npop; ++n )

  entity *best = ga_get_entity_from_rank( pop, 0 );

  if( !best )
  {
    cerr << "Didnt have best on generation=" << generation << endl;
    return true;
  }//if( !best )

  cerr << "\tbest->fitness=" << best->fitness << endl;
  cerr << endl;

  const double bestChi2 = 1.0 / best->fitness;
  data->m_generationsBestChi2.push_back( bestChi2 );

  //Update the model now
  {
    const string error_msg = "Failed to get thread after after generation of "
                             "optimization " + SRC_LOCATION;

    double *chromosome = ((double *)(best->chromosome[0]));
    const vector<double> genes( chromosome, chromosome + pop->len_chromosomes );

    NLSimplePtr guismodel( data->m_gui, true, error_msg );
    guismodel->setModelParameters( genes );

    guismodel->findSteadyStateBeginings( 3.0 );
    guismodel->updateXUsingCgmsInfo();

    const ModelSettings &settings = guismodel->settings();
    const double endPredChi2Weight = settings.m_lastPredictionWeight;

    ModelTestFCN fitFcn( guismodel.get(), endPredChi2Weight, data->m_timeRanges );
    fitFcn.testParamaters( genes, true );
  }


  //Push changes to the user
  {
    Wt::WApplication::UpdateLock lock( data->m_gui->app() );

    double ymin = 0.0, ymax = 0.0;
    ymax = *std::max_element( data->m_generationsBestChi2.begin(), data->m_generationsBestChi2.end() );

    const double nChi2 = static_cast<double>( data->m_generationsBestChi2.size() );
    data->m_gui->geneticOptimizationTab()->setChi2XRange( 0.0, nChi2 + 1 );
    data->m_gui->geneticOptimizationTab()->setChi2YRange( ymin, 1.2*ymax );
    data->m_gui->syncDisplayToModel();
    data->m_gui->app()->triggerUpdate();
  }

  if( data->m_bestChi2CalbackFcn )
    data->m_bestChi2CalbackFcn( bestChi2 );


  //Now lets evaulate if we are done with the evaluation
  //We will kinda use similar convergence criteria to the TMVA genetic algorithm
  //  to determine when convergence occurs
  //When the number of improvments within the last m_genNStepMutate
  // a) smaller than m_genNStepImprove, then divide present sigma by m_genSigmaMult
  // b) equal, do nothing
  // c) larger than m_genNStepImprove, then multiply the present sigma by m_genSigmaMult
  //
  //If convergence hasn't improved more than m_genConvergCriteria in the last
  //  m_genConvergNsteps, then consider minimization complete

  const int nstep = data->m_genConvergNsteps;

  vector<double> &chi2s = data->m_generationsBestChi2;

  if( (nstep<=0) || (generation < nstep) || (chi2s.size()<size_t(nstep)) )
  {
    cerr << "generation < data->m_genConvergCriteria" << endl;
    return true;
  }

  vector<double>::const_iterator pos = chi2s.begin() + chi2s.size() - nstep;
  const double minus_n_val = *pos;
  const double improvment = minus_n_val - bestChi2;

  if( improvment < data->m_genConvergCriteria )
  {
    cerr << "Improvment is only " << improvment << " over the last " << nstep
         << " generations, where convergence criteria is "
         << data->m_genConvergCriteria << endl;
    return false;
  }//if( improvment < data->m_genConvergCriteria )

  int num_improvments = -1;
  double prev_chi2 = DBL_MAX;
  for( ; pos != chi2s.end(); ++pos )
  {
    if( (*pos) < prev_chi2 )
      ++num_improvments;
    prev_chi2 = *pos;
  }//for( ; pos != chi2s.end(); ++pos )

  cerr << "Saw " << num_improvments << " improvments over the last " << nstep
       << " generations, changing sigma from " << data->m_genSigma << " to ";

  if( num_improvments < data->m_genNStepImprove )
    data->m_genSigma /= data->m_genSigmaMult;
  else if( num_improvments > data->m_genNStepImprove )
    data->m_genSigma *= data->m_genSigmaMult;

  cerr << data->m_genSigma << "." << endl;

  return true;
}//bool generation_start_hook( const int generation, population *pop )


void mutate_ga_params( population *pop, entity *father, entity *son )
{
  if( !father || !son )
    die("Null pointer to entity structure passed");

  GenOptData *data = (GenOptData *)pop->data;

  // Select mutation locus.
  const int chromo = (int) random_int(pop->num_chromosomes);  //Index of chromosome to mutate  (will allways be 1 for us)
  const int point = (int) random_int(pop->len_chromosomes);   //Index of allele to mutate

  assert( chromo == 0 );

  //Copy unchanged data.
  for( int i = 0; i < pop->num_chromosomes; ++i )
  {
    memcpy(son->chromosome[i], father->chromosome[i], pop->len_chromosomes*sizeof(double));
    if( i!=chromo )
      ga_copy_data(pop, son, father, i);
    else
      ga_copy_data(pop, son, NULL, i);
  }//for( int i = 0; i < pop->num_chromosomes; ++i )

  double lowX, highX;
  NLSimple::parameterRange( point, lowX, highX );

  //Mutate by tweaking a single allele.
  double *chromosome = ((double *)(son->chromosome[chromo]));
//  double *fatherchromo = ((double *)(father->chromosome[chromo]));

  const double equiv_sigma = (highX - lowX) / data->m_genSigma;
  const double amount = equiv_sigma * random_unit_gaussian();

  chromosome[point] += amount;

  if( chromosome[chromo] > highX )
    chromosome[chromo] -= (highX - lowX);
  if( chromosome[chromo] < lowX )
    chromosome[chromo] += (highX - lowX);
}//void mutate_ga_params( population *pop, entity *father, entity *son )



bool seed_initial_parameters( population *pop, entity *adam )
{
  if( !pop )
    die("Null pointer to population structure passed.");
  if( !adam )
    die("Null pointer to entity structure passed.");

  const int numchromo = pop->num_chromosomes;
  const int chromolen = pop->len_chromosomes;

  assert( numchromo == 1 );
  assert( chromolen >= NLSimple::NumNLSimplePars );

  double *chromosome = ((double *)(adam->chromosome[0]));

  for( int par = 0; par < chromolen; ++par )
  {
    double lowX, highX;
    NLSimple::parameterRange( par, lowX, highX );
    chromosome[par] = random_double_range( lowX, highX );
  }//for( loop over NLSimplePars )

  return true;
}//seed_initial_parameters(...)




bool eval_ga_struggle_score( population *pop, entity *dude )
{
  dude->fitness = 0.0;

  try
  {
    GenOptData *data = (GenOptData *)pop->data;
    boost::shared_ptr<ModelTestFCN> fittnesFunc = data->m_fittnesFunc;

    assert( pop->num_chromosomes == 1 );
    const int len = pop->len_chromosomes;
    assert( len >= NLSimple::NumNLSimplePars );
    double *chromos = (double *)dude->chromosome[0];

    vector<double> genes( chromos, chromos + len );

//    cerr << "Starting an eval with: ";
//    foreach( double s, genes )
//      cerr << s << ", ";
//    cerr << endl;

    foreach( double s, genes )
    {
      if( isnan(s) || isinf(s) )
      {
        cerr << SRC_LOCATION << "\n\tINF or NaN double" << endl;
        return false;
//        throw runtime_error( "INF or NaN double" );
      }
    }//foreach( double s, genes )

    dude->fitness = fittnesFunc->operator()( genes );
    dude->fitness = 1.0 / dude->fitness;

/*
    cerr << "Got fitness " << dude->fitness << " for genes:\n";

    for( NLSimple::NLSimplePars par = NLSimple::NLSimplePars(0);
         par < NLSimple::NumNLSimplePars;
         par = NLSimple::NLSimplePars(par+1) )
    {
      switch(par)
      {
        case NLSimple::BGMultiplier:
          cerr << "\tBGMultiplier=" << genes[par] << endl;
        break;
        case NLSimple::CarbAbsorbMultiplier:
          cerr << "\tCarbAbsorbMultiplier=" << genes[par] << endl;
        break;
        case NLSimple::XMultiplier:
          cerr << "\tXMultiplier=" << genes[par] << endl;
        break;
        case NLSimple::PlasmaInsulinMultiplier:
          cerr << "\tPlasmaInsulinMultiplier=" << genes[par] << endl;
        break;
        case NLSimple::NumNLSimplePars: assert(0);
      };//
    }//for( loop over NLSimplePars )

    for( int parn = int(NLSimple::NumNLSimplePars); parn < len; ++parn )
      cerr << "\tCustomEventPar " << (parn - int(NLSimple::NumNLSimplePars))
           << "=" << genes[parn] << endl;
    cerr << endl << endl;
*/

    if( dude->fitness < 0.0 )
    {
      cerr << SRC_LOCATION << "\n\tGot negative fitness" << endl;
      assert( 0 );
    }
  }catch( std::exception &e )
  {
    cerr << SRC_LOCATION << "\n\tCaught exception: " << e.what() << endl;
    return false;
  }//try/catch

  return true;
}//bool eval_ga_struggle_score(population *pop, entity *dude)

}//namespace GeneticEvalUtils
