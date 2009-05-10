#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fstream>

//ROOT includes (using root 5.14)
#include "TSystem.h"
#include "TStyle.h"
#include "TF1.h"
#include "TClonesArray.h"
#include "TTree.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TPaveText.h"
#include "TMath.h"



//Boost includes (developing with boost 1.38)
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include "boost/bind.hpp"
#include "boost/function.hpp"

#include "KineticModels.hh"
#include "ResponseModel.hh"
#include "RungeKuttaIntegrater.hh"
#include "ConsentrationGraph.hh"
#include "ArtificialPancrease.hh"
#include "CgmsDataImport.hh"
#include "ProgramOptions.hh"

TApplication *gTheApp = (TApplication *)NULL;

using namespace std;

//foward declaration
void setStyle();

void testFFT();
void testSmoothing();
void saveMar31ThorughApr7GraphsToDisk();

//these variable get values through ProgramOptions
double PersonConstants::kPersonsWeight  = kFailValue;
double PersonConstants::kBasalGlucConc  = kFailValue;
double ModelDefaults::kDefaultCgmsDelay = kFailValue;
double ModelDefaults::kCgmsIndivReadingUncert = kFailValue;

int main( int argc, char** argv )
{
  using namespace boost;
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  
  setStyle();
  ProgramOptions::decodeOptions( argc, argv );
  
  //void testSmoothing();
  // void testFFT();  
  // saveMar31ThorughApr7GraphsToDisk();
  ConsentrationGraph mmData( "../data/mmCgmsData_march31_April7.dub" );
  ConsentrationGraph insulinGraph( "../data/insulinConcentration_march31_April7.dub" );
  ConsentrationGraph carbAbsortionGraph( "../data/carbAbsorbtion_march31_April7.dub" );
  ConsentrationGraph meterData1( "../data/meterData_march31_April7.dub" );
  ConsentrationGraph bolusGraph( "../data/bolusData_march31_April7.dub" );
  ConsentrationGraph conspumtionGraph( "../data/carbConsumption_march31_April7.dub" );
  
  ptime t0 = conspumtionGraph.getT0();
  
  NLSimple model( "SimpleModel", 0.8, 120.0, t0 );
  model.addBolusData( bolusGraph );
  mmData.bSplineSmoothOrDeriv( false, 30, 6 );
  model.addCgmsData( mmData );
  model.addGlucoseAbsorption( carbAbsortionGraph );
      
  vector<double> parms(NLSimple::NumNLSimplePars, 0.0);
  parms[NLSimple::BGMultiplier] = 0.0;
  parms[NLSimple::CarbAbsorbMultiplier] = 30.0 / 9.0;
  parms[NLSimple::XMultiplier] = 0.040;
  parms[NLSimple::PlasmaInsulinMultiplier] = 0.00015;
  
  model.setModelParameters( parms );
  model.performModelGlucosePrediction( t0, t0+hours(36) );
  double chi2 = model.getModelChi2( 0.0 );
  cout << "chi2=" << chi2 << endl;
  model.draw();
  
  
  //ConsentrationGraph testGraph( t0, 1.0, InsulinGraph );
  //testGraph.add( 0.8 / kMyWieght, 300.0, NovologAbsorbtion );
  double glucose_equiv_carbs = 45.0;
  static const double t_assent = 40.0;
  static const double t_dessent = 6.0;
  static const double v_max = 0.9;
  static const double k_gut_absoption = 0.1;
  static const double timeStep = 1.0;
  
  ConsentrationGraph glucAbs = yatesGlucoseAbsorptionRate( t0, glucose_equiv_carbs, t_assent, t_dessent, v_max, k_gut_absoption, timeStep );
  ConsentrationGraph insConcen = novologConsentrationGraph( t0, 5.0/PersonConstants::kPersonsWeight, timeStep );
  
  // insConcen.draw();
  // glucAbs.draw();
  
  ForcingFunction foodEaten = boost::bind( &ConsentrationGraph::valueUsingOffset, glucAbs, _1 );
  ForcingFunction insulinTaken = boost::bind( &ConsentrationGraph::valueUsingOffset, insConcen, _1 );
  
  double time = 0.0;
  double G_basal = 100.0;  //g/dL
  double I_basal = 100.0; //mU/L
  std::vector<double> GAndX(2, 0.0);
  std::vector<double> G_parameters(2, 0.0);
  std::vector<double> X_parameters(2, 0.0);
  
  G_parameters[0] = 0.0;
  G_parameters[1] = 30.0 / 9.0;
  X_parameters[0] = 0.025;
  X_parameters[1] = 0.0002;
  RK_DerivFuntion dGdTdXdT = boost::bind( dGdT_and_dXdT, _1, _2, 
                                          G_basal, I_basal, 
                                          G_parameters, X_parameters,
                                          foodEaten,
                                          insulinTaken 
                                        );
  vector<double> y0(2, 0.0);

  vector<ConsentrationGraph> answers( 2, ConsentrationGraph( t0, timeStep, GlucoseConsentrationGraph ) );
  
  integrateRungeKutta4( 0, 240, timeStep, y0, dGdTdXdT, answers );

  answers[0].draw();
  answers[1].draw();
  
  GAndX = rungeKutta4( time, GAndX, timeStep, dGdTdXdT );
  
  
  
  //body mass index 26.2 +- 4.9 kg /m^2
  ConsentrationGraph testGraph( t0, 1.0, GlucoseAbsorbtionRateGraph );
  testGraph.add( 100.0, 0.0,  MediumCarbAbsorbtionRate );
  // testGraph.draw();
  
  testGraph.add( 20.0, 120.0,  FastCarbAbsorbtionRate );
  
  testGraph.draw();
}//main(...)





void saveMar31ThorughApr7GraphsToDisk()
{
  
  using namespace boost;
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  
  string startDate = "2009-03-031 00:00:00.000";
  string endDate = "2009-04-01 19:30:00.000";
  ConsentrationGraph mmData = CgmsDataImport::importSpreadsheet( "../MM_march1-Apr7.csv", 
                                              CgmsDataImport::CgmsReading,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  
  
  
  // ConsentrationGraph meterData = CgmsDataImport::importSpreadsheet( "../MM_march1-Apr7.csv", 
                                              // CgmsDataImport::MeterReading,
                                              // time_from_string(startDate),
                                              // time_from_string(endDate) );
  // meterData.saveToFile( "data/meterData_march31_April7.dub" );
  // meterData.draw( "A*" );
  
 
  ConsentrationGraph meterData1 = CgmsDataImport::importSpreadsheet( "../../march30_apr1st_diabetesLog.csv", 
                                              CgmsDataImport::MeterReading,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  ConsentrationGraph bolusGraph = CgmsDataImport::importSpreadsheet( "../../march30_apr1st_diabetesLog.csv", 
                                              CgmsDataImport::BolusTaken,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  ConsentrationGraph conspumtionGraph = CgmsDataImport::importSpreadsheet( "../../march30_apr1st_diabetesLog.csv", 
                                              CgmsDataImport::GlucoseEaten,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  ConsentrationGraph insulinGraph = CgmsDataImport::bolusGraphToInsulinGraph( bolusGraph, PersonConstants::kPersonsWeight );
  ConsentrationGraph carbAbsortionGraph = CgmsDataImport::carbConsumptionToSimpleCarbAbsorbtionGraph( conspumtionGraph );
  
  mmData.saveToFile( "../data/mmCgmsData_march31_April7.dub" );
  meterData1.saveToFile( "../data/meterData_march31_April7.dub" );
  bolusGraph.saveToFile( "../data/bolusData_march31_April7.dub" );
  insulinGraph.saveToFile( "../data/insulinConcentration_march31_April7.dub" );
  conspumtionGraph.saveToFile( "../data/carbConsumption_march31_April7.dub" );
  carbAbsortionGraph.saveToFile( "../data/carbAbsorbtion_march31_April7.dub" );

}//saveMar31ThorughApr7GraphsToDisk



void testSmoothing()
{
  ConsentrationGraph mmData( "../data/mmCgmsData_march31_April7.dub" );
  
  ConsentrationGraph bSlpineDiffData = mmData;
  bSlpineDiffData.bSplineSmoothOrDeriv( true, 30, 6 );
  
  ConsentrationGraph bSplineSmoothThenDiffData = mmData;
  bSplineSmoothThenDiffData.bSplineSmoothOrDeriv( false, 30, 6 );
  bSplineSmoothThenDiffData.differntiate(5);
  
  ConsentrationGraph butterSmoothThenDiffData = mmData;
  butterSmoothThenDiffData.butterWorthFilter( 60, 4 );
  butterSmoothThenDiffData.differntiate(5);
  // new TCanvas();
  mmData.draw("", "", false, 1);
  new TCanvas();
  bSlpineDiffData.draw( "Al", "", false, 1 );
  bSplineSmoothThenDiffData.draw( "l", "", false, 2 );
  butterSmoothThenDiffData.draw( "l", "", true, 4 );
  
  
  
  ConsentrationGraph mmSmoothedData = mmData;
  ConsentrationGraph mmSmoothedFFT = mmData;
  
  
  
  // mmData.draw("", "", false, 1);
  // mmSmoothedData.butterWorthFilter( 60, 4 );
  mmSmoothedFFT.bSplineSmoothOrDeriv( false, 30, 6 );
  mmSmoothedFFT.differntiate(3);
  mmSmoothedFFT.draw("Al", "smoothed", true, 2);
  
  mmSmoothedFFT.fastFourierSmooth( 30 );
  mmSmoothedData.draw("l", "smoothed", false, 2);
  mmSmoothedFFT.draw("l", "smoothed", true, 3);
}//void testSmoothing()


void testFFT()
{
  ConsentrationGraph fftTest( kGenericT0, 5, GlucoseConsentrationGraph );
  
  TF1 fsin("fffFunc", "sin(2.0*TMath::Pi()*x/600.0) + sin(2.0*TMath::Pi()*x/30.0) + cos(2.0*TMath::Pi()*x/20.0)", 0, 600.0 );
  //p = 1/x
  
  for( double min = 0; min <= 600; min += 5.0 )
  {
    fftTest.insert( min, fsin.Eval(min) );
  }
  
  fftTest.draw("", "", false, 1);
  
  ConsentrationGraph smoothed10 = fftTest;
  smoothed10.fastFourierSmooth( 10 );
  new TCanvas("10", "10" );
  smoothed10.draw("", "", false, 2);
  
  ConsentrationGraph smoothed15 = fftTest;
  smoothed15.fastFourierSmooth( 15 );
  new TCanvas("15", "15");
  smoothed15.draw("", "", false, 3);
  
  ConsentrationGraph smoothed20 = fftTest;
  smoothed20.fastFourierSmooth( 20 );
  new TCanvas("20", "20");
  smoothed20.draw("", "", false, 3);
  
  ConsentrationGraph smoothed25 = fftTest;
  smoothed25.fastFourierSmooth( 25 );
  new TCanvas("25", "25");
  smoothed25.draw("", "", false, 3);
  
  ConsentrationGraph smoothed30 = fftTest;
  smoothed30.fastFourierSmooth( 30 );
  new TCanvas("30", "30");
  smoothed30.draw("", "", true, 4);
  
  ConsentrationGraph smoothed35 = fftTest;
  smoothed35.fastFourierSmooth( 35 );
  new TCanvas("35", "35");
  smoothed35.draw("", "", true, 4);
}//void testFFT()




void setStyle()
{
  TStyle *myStyle = gROOT->GetStyle("Plain"); //base style on Plain
   
  myStyle->SetPalette(1,0); // pretty color palette

   // use plain black on white colors
  myStyle->SetFrameBorderMode(0);
  myStyle->SetTitleBorderSize(0);
  myStyle->SetCanvasBorderMode(0);
  myStyle->SetPadBorderMode(0);
  myStyle->SetPadColor(0);
  myStyle->SetFillStyle(0);
  // myStyle->SetNdivisions( 55, "x" );
   
  gROOT->SetStyle("Plain");
  gROOT->ForceStyle();
}//void setStyle()

