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

TApplication *gTheApp = (TApplication *)NULL;

using namespace std;

//foward declaration
void setStyle();

void saveMar31ThorughApr7GraphsToDisk();


int main( int argc, char** argv )
{
  using namespace boost;
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  
  setStyle();
  const double kMyWieght = 78.0; //kg
  
  // saveMar31ThorughApr7GraphsToDisk();
  
  ConsentrationGraph mmData( "data/mmCgmsData_march31_April7.txt" );
  ConsentrationGraph insulinGraph( "data/insulinConcentration_march31_April7.txt" );
  ConsentrationGraph carbAbsortionGraph( "data/carbAbsorbtion_march31_April7.txt" );
  ConsentrationGraph meterData1( "data/meterData_march31_April7.txt" );
  ConsentrationGraph bolusGraph( "data/bolusData_march31_April7.txt" );
  ConsentrationGraph conspumtionGraph( "data/carbConsumption_march31_April7.txt" );
  
  ptime t0 = conspumtionGraph.getT0();
  
  NLSimple model( "SimpleModel", 0.8, 120.0, t0 );
  model.addBolusData( bolusGraph );
  model.addCgmsData( mmData );
  model.addGlucoseAbsorption( carbAbsortionGraph );
      
  vector<double> parms(NLSimple::NumNLSimplePars, 0.0);
  parms[NLSimple::BGMultiplier] = 0.0;
  parms[NLSimple::CarbAbsorbMultiplier] = 30.0 / 9.0;
  parms[NLSimple::XMultiplier] = 0.035;
  parms[NLSimple::PlasmaInsulinMultiplier] = 0.00015;
  
  model.setModelParameters( parms );
  model.performModelGlucosePrediction( t0, t0+hours(36) );
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
  ConsentrationGraph insConcen = novologConsentrationGraph( t0, 5.0/kMyWieght, timeStep );
  
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
  // meterData.saveToFile( "data/meterData_march31_April7.txt" );
  // meterData.draw( "A*" );
  
 
  ConsentrationGraph meterData1 = CgmsDataImport::importSpreadsheet( "../march30_apr1st_diabetesLog.csv", 
                                              CgmsDataImport::MeterReading,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  ConsentrationGraph bolusGraph = CgmsDataImport::importSpreadsheet( "../march30_apr1st_diabetesLog.csv", 
                                              CgmsDataImport::BolusTaken,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  ConsentrationGraph conspumtionGraph = CgmsDataImport::importSpreadsheet( "../march30_apr1st_diabetesLog.csv", 
                                              CgmsDataImport::GlucoseEaten,
                                              time_from_string(startDate),
                                              time_from_string(endDate) );
  ConsentrationGraph insulinGraph = CgmsDataImport::bolusGraphToInsulinGraph( bolusGraph, PersonConstants::kPersonsWeight );
  ConsentrationGraph carbAbsortionGraph = CgmsDataImport::carbConsumptionToSimpleCarbAbsorbtionGraph( conspumtionGraph );
  
  mmData.saveToFile( "data/mmCgmsData_march31_April7.txt" );
  meterData1.saveToFile( "data/meterData_march31_April7.txt" );
  bolusGraph.saveToFile( "data/bolusData_march31_April7.txt" );
  insulinGraph.saveToFile( "data/insulinConcentration_march31_April7.txt" );
  conspumtionGraph.saveToFile( "data/carbConsumption_march31_April7.txt" );
  carbAbsortionGraph.saveToFile( "data/carbAbsorbtion_march31_April7.txt" );

}//saveMar31ThorughApr7GraphsToDisk


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

