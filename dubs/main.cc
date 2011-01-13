#include <Wt/WApplication>

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
#include "TH1F.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TPaveText.h"
#include "TMath.h"


#include "KineticModels.hh"
#include "ResponseModel.hh"
#include "RungeKuttaIntegrater.hh"
#include "ConsentrationGraph.hh"
#include "ArtificialPancrease.hh"
#include "CgmsDataImport.hh"
#include "ProgramOptions.hh"

#include "WtGui.hh"
#include "WtUtils.hh"

using namespace Wt;
using namespace std;

//Forward Declarations
void setStyle();

DubUserServer gDubUserServer;  //the single instance of the DubUserServer



WApplication *createApplication(const WEnvironment& env)
{
  WtGui *app = new WtGui(env, gDubUserServer);
  return app;
}

int main(int argc, char *argv[])
{
  setStyle();
  ProgramOptions::declareOptions();
  //ProgramOptions::decodeOptions( argc, argv );
  ProgramOptions::decodeOptions( 0, NULL );
  ProgramOptions::ns_defaultModelFileName = "../data/qt_test.dubm";

  return WRun(argc, argv, &createApplication);
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
