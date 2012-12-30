#include <vector>
#include <math.h>
#include <string>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <Wt/WApplication>

//ROOT includes
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TMath.h"
#include "TTree.h"
#include "TStyle.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TPaveText.h"
#include "TApplication.h"
#include "TClonesArray.h"

#include "dubs/WtGui.hh"
#include "dubs/WtUtils.hh"
#include "dubs/DubUser.hh"
#include "dubs/KineticModels.hh"
#include "dubs/ResponseModel.hh"
#include "dubs/CgmsDataImport.hh"
#include "dubs/ProgramOptions.hh"
#include "dubs/DubsApplication.hh"
#include "dubs/ConsentrationGraph.hh"
#include "dubs/ArtificialPancrease.hh"
#include "dubs/RungeKuttaIntegrater.hh"



using namespace Wt;
using namespace std;

//Forward Declarations
void setStyle();
void cgmFilterStudy();

DubUserServer gDubUserServer;  //the single instance of the DubUserServer



WApplication *createApplication(const WEnvironment& env)
{
  DubsApplication *app = new DubsApplication( env, gDubUserServer, "user_information.db" );
  return app;
}

int main( int argc, char *argv[] )
{
  setStyle();
//  cgmFilterStudy();
  ProgramOptions::declareOptions();
  //ProgramOptions::decodeOptions( argc, argv );
  ProgramOptions::decodeOptions( 0, NULL );
  ProgramOptions::ns_defaultModelFileName = "../data/qt_test.dubm";

  return WRun( argc, argv, &createApplication );
}//int main(int argc, char *argv[])


void setStyle()
{
  TStyle *myStyle = gROOT->GetStyle("Plain"); //base style on Plain

  myStyle->SetPalette(1,0); // pretty color palette

  TH1::AddDirectory(kFALSE);

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


#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include "boost/bind.hpp"
#include "boost/function.hpp"
void testFiltering();
void testFFTAA();
extern TApplication *gTheApp;
void cgmFilterStudy()
{

  Int_t dummy_arg = 0;
  gTheApp = gApplication = (TApplication *)new TApplication( "App", &dummy_arg, (char **)NULL );

  using namespace boost;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

//  string startDate = "2009-03-031 00:00:00.000";
//  string endDate = "2009-04-01 19:30:00.000";
//  ConsentrationGraph mmData = CgmsDataImport::importSpreadsheet( "../data/MM_march1-Apr7.csv", CgmsDataImport::CgmsReading, time_from_string(startDate), time_from_string(endDate) );

  string startDate = "2011-02-13 00:00:00.000";
  string endDate = "2011-02-14 00:00:00.000";
  ConsentrationGraph mmData = CgmsDataImport::importSpreadsheet( "../data/dex_data_through_20110216.Export.Export.txt", CgmsDataImport::CgmsReading, time_from_string(startDate), time_from_string(endDate) );


  ConsentrationGraph fft120 = mmData.getSmoothedGraph( 75, FourierSmoothing );
  ConsentrationGraph fftMatt = mmData;
  fftMatt.fastFourierSmooth( 0.1, true );

  ConsentrationGraph butter = mmData.getSmoothedGraph( 240, ButterworthSmoothing );
  ConsentrationGraph bspline = mmData.getSmoothedGraph( 240, BSplineSmoothing );

  ConsentrationGraph sgSmoothed = mmData;
  ConsentrationGraph sgSmoothed220 = mmData;
  ConsentrationGraph sgSmoothed320 = mmData;
  ConsentrationGraph sgSmoothed420 = mmData;
  ConsentrationGraph sgSmoothed520 = mmData;
  ConsentrationGraph sgSmoothed620 = mmData;

  int num_left, num_right, order;
  sgSmoothed = sgSmoothed.getSavitzyGolaySmoothedGraph( num_left=35, num_right=35, order = 5 );

  sgSmoothed220 = sgSmoothed220.getSavitzyGolaySmoothedGraph( num_left=20, num_right=20, order = 2 );
  sgSmoothed320 = sgSmoothed320.getSavitzyGolaySmoothedGraph( num_left=20, num_right=20, order = 3 );
  sgSmoothed420 = sgSmoothed420.getSavitzyGolaySmoothedGraph( num_left=20, num_right=20, order = 4 );
  sgSmoothed520 = sgSmoothed520.getSavitzyGolaySmoothedGraph( num_left=20, num_right=20, order = 5 );
  sgSmoothed620 = sgSmoothed620.getSavitzyGolaySmoothedGraph( num_left=20, num_right=20, order = 6 );



  TLegend *leg = new TLegend( 0.7, 0.7, 0.9, 1.0 );
  TGraph *original = mmData.draw("", "", false, 1 );
  TGraph *fft120G = fft120.draw("SAME", "", false, 11 );
  TGraph *fftMattG = fftMatt.draw("SAME", "", false, 4 );

//  TGraph *butterG = butter.draw("SAME", "", false, 4 );
//  TGraph *bsplineG = bspline.draw("SAME", "", false, 6 );
//  TGraph *sgG = sgSmoothed.draw("SAME", "", false, 46 );

//  TGraph *sgG220 = sgSmoothed220.draw("SAME", "", false, 2 );
//  TGraph *sgG320 = sgSmoothed320.draw("SAME", "", false, 3 );
//  TGraph *sgG420 = sgSmoothed420.draw("SAME", "", false, 4 );
//  TGraph *sgG520 = sgSmoothed520.draw("SAME", "", false, 28 );
//  TGraph *sgG620 = sgSmoothed620.draw("SAME", "", false, 6 );


  leg->AddEntry( original, "Original Dexcom data", "l" );
  leg->AddEntry( fft120G,  "FFT Strict Low Pass #lambda=75 min", "l" );
  leg->AddEntry( fftMattG,  "FFT 10% largest contributing frequ", "l" );
//  leg->AddEntry( butterG,  "4^{th} order Butterworth, #lambda_{crit}=240 min", "l" );
//  leg->AddEntry( bsplineG,  "6^{th} order B-Spline, knot distance 240/2 min", "l" );
//  leg->AddEntry( sgG,  "Savitzy-Golay N_{left}=35, N_{right}=35, 5^{th} Order", "l" );
//  leg->AddEntry( sgG220,  "Savitzy-Golay N_{left}=20, N_{right}=20, 2^{th} Order", "l" );
//  leg->AddEntry( sgG320,  "Savitzy-Golay N_{left}=20, N_{right}=20, 3^{th} Order", "l" );
//  leg->AddEntry( sgG420,  "Savitzy-Golay N_{left}=20, N_{right}=20, 4^{th} Order", "l" );
//  leg->AddEntry( sgG520,  "Savitzy-Golay N_{left}=20, N_{right}=20, 5^{th} Order", "l" );
//  leg->AddEntry( sgG620,  "Savitzy-Golay N_{left}=20, N_{right}=20, 6^{th} Order", "l" );

  leg->SetBorderSize( 0 );
  leg->Draw();
  gApplication->Run(  kTRUE );



/*
  ,      //Strict low-pass filter
  ButterworthSmoothing,  //order 4 low-pass Butterworth filter
  BSplineSmoothing,      //B-Splines, okay smoothing of data (not above link type)
                         //  but excelentfor taking derivative
  NoSmoothing,
  */
}

void testFiltering()
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
}//void void testFiltering()()


void testFFTAA()
{
  ConsentrationGraph fftTest( kGenericT0, 5, GlucoseConsentrationGraph );

  TF1 fsin("fffFunc", "sin(2.0*TMath::Pi()*x/600.0) + sin(2.0*TMath::Pi()*x/30.0) + cos(2.0*TMath::Pi()*x/20.0)", 0, 600.0 );
  //p = 1/x

  for( double min = 0; min <= 600; min += 5.0 )
  {
    fftTest.insert( fftTest.getT0() + toTimeDuration(min), fsin.Eval(min) );
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

