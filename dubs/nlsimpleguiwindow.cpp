#include "nlsimpleguiwindow.h"
#include "ui_nlsimpleguiwindow.h"

#include "ResponseModel.hh"
#include "nlsimple_create.h"


#include "TH1F.h"

NLSimpleGuiWindow::NLSimpleGuiWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::NLSimpleGuiWindow)
{
    ui->setupUi(this);
    init();
}


void NLSimpleGuiWindow::init()
{
  ui->tQtWidget2->cd();
  //connect(tQtWidget2,SIGNAL(RootEventProcessed(TObject *, unsigned int, TCanvas *)),this,SLOT(CanvasEvent(TObject *, unsigned int, TCanvas *)));
  TH1 *hist = new TH1F( "hist", "Test Hist", 10, 0, 10 );
  hist->FillRandom("gaus", 1000);
  hist->Draw();
}

NLSimpleGuiWindow::~NLSimpleGuiWindow()
{
    delete ui;
}
