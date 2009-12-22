#include "nlsimple_create.h"
#include "ui_nlsimple_create.h"



#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <string>
#include <stdio.h>
#include <math.h>    //contains M_PI
#include <stdlib.h>
#include <fstream>
#include <algorithm> //min/max_element
#include <float.h>   // for DBL_MAX

#include "TList.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TGraph.h"
#include "TH1F.h"

#include "NLSimpleGui.hh"
#include "ResponseModel.hh"
#include "CgmsDataImport.hh"
#include "ProgramOptions.hh"
#include "ArtificialPancrease.hh"
#include "RungeKuttaIntegrater.hh"
#include "ConsentrationGraph.hh"
#include "ConsentrationGraphGui.hh"

#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"


#include <QDir>
#include <QFileDialog>
#include <QScrollBar>

#include "MiscGuiUtils.hh"

using namespace std;


NlSimpleCreate::NlSimpleCreate( NLSimple *&model, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::NlSimpleCreate),
    m_model(model),
    m_userSetTime(false),
    m_bolusData(NULL),
    m_insulinData(NULL), //created from m_bolusData
    m_carbConsumptionData(NULL),
    m_meterData(NULL),
    m_customData(NULL),
    //ConsentrationGraph *m_ExcersizeData;
    m_minutesGraphPerPage(-1)

{
    m_ui->setupUi(this);
    init();
}//NlSimpleCreate constructor

void NlSimpleCreate::init()
{
    m_ui->m_createButton->setEnabled(false);
    m_ui->tabWidget->setTabText( kCGMS_PAD, "CGMS" );
    m_ui->tabWidget->setTabText( kBOLUS_PAD, "Bolus" );
    m_ui->tabWidget->setTabText( kCARB_PAD, "Meals" );
    m_ui->tabWidget->setTabText( kMETER_PAD, "Meter" );
    m_ui->tabWidget->setTabText( kCustom_PAD, "Custom" );
    m_ui->tabWidget->setCurrentIndex(kCGMS_PAD);
    m_ui->m_createButton->setEnabled(false);
    m_ui->m_endDateEntry->setFocusPolicy( Qt::StrongFocus );
    m_ui->m_startDateEntry->setFocusPolicy( Qt::StrongFocus );
    m_ui->m_basalInsulinAmount->setFocusPolicy( Qt::StrongFocus );
    m_ui->m_startDateEntry->setDateTime( posixTimeToQTime(kGenericT0) );
    m_ui->m_endDateEntry->setDateTime( posixTimeToQTime(kGenericT0) );

    double weightInlbs = 2.20462262 * PersonConstants::kPersonsWeight;
    m_ui->m_weightInput->setRange(1,500);
    m_ui->m_weightInput->setValue( (int)weightInlbs );
    m_ui->m_unitButton->setText("lbs");
    m_useKgs = false;
}//void NlSimpleCreate::init()



NlSimpleCreate::~NlSimpleCreate()
{
  if( m_cgmsData )            delete m_cgmsData;
  if( m_insulinData )         delete m_insulinData;
  if( m_carbConsumptionData ) delete m_carbConsumptionData;
  if( m_meterData )           delete m_meterData;
  if( m_bolusData )           delete m_bolusData;

  delete m_ui;
}//NlSimpleCreate::~NlSimpleCreate()

void NlSimpleCreate::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}//void NlSimpleCreate::changeEvent(QEvent *e)




void NlSimpleCreate::findTimeLimits()
{
     //Don't let user control time span before loading any graphs
  if( m_userSetTime )
  {
    m_userSetTime = ( m_cgmsData || m_insulinData || m_bolusData
                      || m_carbConsumptionData
                      || m_meterData || m_customData );
  }//if( m_userSetTime )

  PosixTime currStartTime = qtimeToPosixTime( m_ui->m_startDateEntry->dateTime() );
  PosixTime currEndTime = qtimeToPosixTime( m_ui->m_endDateEntry->dateTime() );

  if( (currEndTime == kGenericT0) && (currEndTime == kGenericT0)) m_userSetTime = false;

  if( m_userSetTime ) return;

  PosixTime endTime = kGenericT0;
  PosixTime startTime = kGenericT0;

  if( m_cgmsData )
  {
    endTime = m_cgmsData->getEndTime();
    startTime = m_cgmsData->getStartTime();
  }//if( m_cgmsData )

  if( m_bolusData )
  {
    PosixTime end = m_bolusData->getEndTime() + TimeDuration( 24, 0, 0, 0 );
    PosixTime start = m_bolusData->getStartTime() - TimeDuration( 24, 0, 0, 0 );
    startTime = std::max( startTime, start );
    endTime = (endTime == kGenericT0) ? end : std::min( endTime, end );
  }//if( m_bolusData )

  if( m_carbConsumptionData )
  {
    PosixTime end = m_carbConsumptionData->getEndTime() + TimeDuration( 24, 0, 0, 0 );
    PosixTime start = m_carbConsumptionData->getStartTime() - TimeDuration( 24, 0, 0, 0 );
    startTime = std::max( startTime, start );
    endTime = (endTime == kGenericT0) ? end : std::min( endTime, end );
  }//if( m_carbConsumptionData )


  if( (endTime == kGenericT0) && (startTime  == kGenericT0) && m_customData )
  {
    endTime = m_customData->getEndTime() + TimeDuration( 24, 0, 0, 0 );
    startTime = m_customData->getStartTime() - TimeDuration( 24, 0, 0, 0 );
  }//if( m_carbConsumptionData )


  if( (endTime != kGenericT0) && (startTime != kGenericT0) )
  {
    QDateTime minTimeDate = posixTimeToQTime( startTime );
    QDateTime maxTimeDate = posixTimeToQTime( endTime );

    m_ui->m_startDateEntry->setDateTimeRange(minTimeDate, maxTimeDate);
    m_ui->m_endDateEntry->setDateTimeRange(minTimeDate, maxTimeDate);
    m_ui->m_startDateEntry->setDateTime(minTimeDate);
    m_ui->m_endDateEntry->setDateTime(maxTimeDate);
  }//if( we have time limits )
}//void NlSimpleCreate::findTimeLimits()


void NlSimpleCreate::drawPreview( GraphPad pad )
{
  //TQtWidget *qtWidget = dynamic_cast<TQtWidget *>(m_ui->tabWidget->widget(pad));
  QObjectList qlist = m_ui->tabWidget->widget(pad)->children();

  TQtWidget *qtWidget = m_ui->tabWidget->widget(pad)->findChild<TQtWidget *>();
  if( !qtWidget ) qtWidget = dynamic_cast<TQtWidget *>(m_ui->tabWidget->widget(pad));

  if( !qtWidget ) cout << "Error finding TQtWidget for pad " << pad << endl;
  if( !qtWidget ) return;

  TCanvas *can = qtWidget->GetCanvas();
  can->cd();
  can->SetEditable( kTRUE );

  TObject *obj;
  TList *list = can->GetListOfPrimitives();
  for( TIter nextObj(list); (obj = nextObj()); )
  {
    string className = obj->ClassName();
    if( className != "TFrame" ) delete obj;
  }//for(...)
  can->Update();

  PosixTime startTime = qtimeToPosixTime( m_ui->m_startDateEntry->dateTime() );
  PosixTime endTime = qtimeToPosixTime( m_ui->m_endDateEntry->dateTime() );

  vector<TGraph *>graphs;
  if( !m_cgmsData ) endTime = startTime = kGenericT0;

  switch( pad )
  {
    case kCGMS_PAD:
      if(m_cgmsData) graphs.push_back( m_cgmsData->getTGraph(startTime, endTime) );
    break;

    case kCARB_PAD:
      if( m_carbConsumptionData )
      {
        graphs.push_back( m_carbConsumptionData->getTGraph(startTime, endTime) );
      }//if both defined
    break;

    case kMETER_PAD:
      if(m_meterData) graphs.push_back( m_meterData->getTGraph(startTime, endTime) );
    break;

    case kBOLUS_PAD:
      if(m_bolusData) graphs.push_back( m_bolusData->getTGraph(startTime, endTime) );
      // m_insulinData
    break;

    case kCustom_PAD:
      if(m_customData) graphs.push_back( m_customData->getTGraph(startTime, endTime) );
    break;

    case kEXCERSIZE_PAD:
    case kALL_PAD:
    case kNUM_PAD:
    break;
  };//switch( pad )

  for( size_t i = 0; i < graphs.size(); ++i )
  {
    if( graphs[i]->GetN() < 2 )
    {
      std::cout << "Refusing to display " << graphs[i]->GetN() << " points" << std::endl;
      continue;
    }//if( graphs[i]->GetN() < 2 )

    graphs[i]->SetLineColor( i+1 );

    string drawOptions;
    if( i==0 ) drawOptions += 'A';
    if( pad == kCGMS_PAD ) drawOptions += 'l';
    else                   drawOptions += '*';

    graphs[i]->Draw( drawOptions.c_str() );
  }//for( size_t i = 0; i < graphs.size(); ++i )
  can->Update();
  updateModelGraphSize();
}//void NlSimpleCreate::drawPreview( GraphPad pad )

void NlSimpleCreate::updateModelGraphSize()
{
    updateModelGraphSize(kCGMS_PAD);
    updateModelGraphSize(kBOLUS_PAD);
    updateModelGraphSize(kCARB_PAD);
    updateModelGraphSize(kMETER_PAD);
    updateModelGraphSize(kCustom_PAD);
}//void NlSimpleCreate::updateModelGraphSize()

void NlSimpleCreate::updateModelGraphSize(int tabNumber)
{
  if( !m_ui->tabWidget->widget(tabNumber) ) return;
  // assert( !m_ui->tabWidget->widget(tabNumber) );
  TQtWidget *qtWidget = m_ui->tabWidget->widget(tabNumber)->findChild<TQtWidget *>();
  if( !qtWidget ) qtWidget = dynamic_cast<TQtWidget *>(m_ui->tabWidget->widget(tabNumber));
  if( !qtWidget ) cout << "Error finding TQtWidget in void "
                       << "NlSimpleCreate::updateModelGraphSize(" << tabNumber << ")" <<  endl;
  QScrollArea *scrollArea = m_ui->tabWidget->widget(tabNumber)->findChild<QScrollArea *>();

  if( !qtWidget ) return;
  if( !scrollArea ) return;

  TCanvas *can = qtWidget->GetCanvas();
  can->SetEditable( kTRUE );
  can->Update(); //need this or else TCanvas won't have updated axis range

  double xmin, xmax, ymin, ymax;
  can->GetRangeAxis( xmin, ymin, xmax, ymax );
  const double nMinutes = xmax - xmin;

  if( nMinutes > 1 )
  {
    //see if have made it this far before
    if( m_minutesGraphPerPage < 0 ) m_minutesGraphPerPage = nMinutes;
    //always fill up screen
    if( nMinutes < m_minutesGraphPerPage ) m_minutesGraphPerPage = nMinutes;

    //for memmories sake, allow max of 5 pages wide
    double nPages = std::min(5.0, nMinutes / m_minutesGraphPerPage);
    if( nPages >= 5.0 )  m_minutesGraphPerPage = nMinutes / 5.0;

    if( m_minutesGraphPerPage == nMinutes )
       scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    else scrollArea->setHorizontalScrollBarPolicy ( Qt::ScrollBarAsNeeded );

    const int newWidth = nPages * scrollArea->width();
    qtWidget->setMinimumSize( newWidth, scrollArea->height() - scrollArea->verticalScrollBar()->height() );
    qtWidget->setMaximumSize( newWidth, scrollArea->height() - scrollArea->verticalScrollBar()->height() );
  }//if( nMinutes > 1 )

  can->Update();
  can->SetEditable( kFALSE );
}//void NlSimpleCreate::updateModelGraphSize(int tabNumber)


void NlSimpleCreate::addCgmsData()
{
    ConsentrationGraph *newData = openConsentrationGraph( this, CgmsDataImport::CgmsReading );
    if( m_cgmsData && newData )
    {
      m_cgmsData->addNewDataPoints( *newData );
      delete newData;
    }else if( newData )
    {
        m_cgmsData = newData;
        m_ui->m_cgmsButton->setText( "More CGMS Data" );
    }

   findTimeLimits();
   drawPreview(kCGMS_PAD);
   m_ui->tabWidget->setCurrentIndex(kCGMS_PAD);
   enableCreateButton();
}//void NlSimpleCreate::addCgmsData()

void NlSimpleCreate::addBolusData()
{
    ConsentrationGraph *newData = openConsentrationGraph( this, CgmsDataImport::BolusTaken );

    if( m_bolusData && newData )
    {
      m_bolusData->addNewDataPoints( *newData );
      delete newData;
    }else if( newData )
    {
        m_bolusData = newData;
        m_ui->m_bolusButton->setText(  "More Bolus Data" );
    }

    findTimeLimits();
    drawPreview(kBOLUS_PAD);
    m_ui->tabWidget->setCurrentIndex(kBOLUS_PAD);
    //ConsentrationGraph *m_insulinData; //created from m_bolusData
    enableCreateButton();
}//void NlSimpleCreate::addBolusData()

void NlSimpleCreate::addCarbData()
{
  ConsentrationGraph *newData = openConsentrationGraph( this, CgmsDataImport::GlucoseEaten );

  if( m_carbConsumptionData && newData )
  {
    m_carbConsumptionData->addNewDataPoints( *newData );
    delete newData;
  }else if( newData )
  {
    m_carbConsumptionData = newData;
    m_ui->m_carbButton->setText( "More Meal Data" );
  }

  findTimeLimits();
  drawPreview(kCARB_PAD);
  m_ui->tabWidget->setCurrentIndex(kCARB_PAD);
  enableCreateButton();
}//void NlSimpleCreate::addCarbData()

void NlSimpleCreate::addMeterData()
{
    ConsentrationGraph *newData = openConsentrationGraph( this, CgmsDataImport::MeterReading );
    if( m_meterData && newData )
    {
      m_meterData->addNewDataPoints( *newData );
      delete newData;
    }else if( newData )
    {
        m_meterData = newData;
        m_ui->m_meterButton->setText( "More Meter Data" );
    }

  findTimeLimits();
  drawPreview(kMETER_PAD);
  m_ui->tabWidget->setCurrentIndex(kMETER_PAD);
  enableCreateButton();
}//void NlSimpleCreate::addMeterData()


void NlSimpleCreate::addCustomData()
{
    ConsentrationGraph *newData = openConsentrationGraph( this, CgmsDataImport::GenericEvent );
    if( m_customData && newData )
    {
      m_customData->addNewDataPoints( *newData );
      delete newData;
    }else if( newData )
    {
        m_customData = newData;
        m_ui->m_cutomData->setText( "More Custom Data" );
    }
   findTimeLimits();
   drawPreview(kCustom_PAD);
   m_ui->tabWidget->setCurrentIndex(kCustom_PAD);
   enableCreateButton();
}//void NlSimpleCreate::addCustomData()


void NlSimpleCreate::constructModel()
{
  assert( m_cgmsData );
  // m_insulinData;
  assert( m_carbConsumptionData );
  // m_meterData;
  assert( m_bolusData );

  double insPerHour = m_ui->m_basalInsulinAmount->value();
  if( insPerHour <= 0.0 )
  {
      cout << "Error in your program, for some reason basal insulin has a rate of "
           << insPerHour << endl;
      m_ui->m_createButton->setEnabled(false);
      return;
  }//if( insPerHour <= 0.0 )

  double weight = m_ui->m_weightInput->value();
  if( !m_useKgs ) weight /= 0.45359237;
  PersonConstants::kPersonsWeight = weight;
  insPerHour /= weight;
  //assert( insPerHour > 0.0 );


  PosixTime startTime = qtimeToPosixTime( m_ui->m_startDateEntry->dateTime() );
  PosixTime endTime = qtimeToPosixTime( m_ui->m_endDateEntry->dateTime() );

  m_bolusData->trim( startTime, endTime, false );
  m_cgmsData->trim( startTime, endTime );
  m_carbConsumptionData->trim( startTime, endTime, false );
  if(m_meterData) m_meterData->trim( startTime, endTime, false );
  if(m_customData) m_customData->trim( startTime, endTime, false );

  m_model = new NLSimple( "SimpleModel", insPerHour, PersonConstants::kBasalGlucConc, m_bolusData->getStartTime() );
  m_model->addBolusData( *m_bolusData );
  m_model->addCgmsData( *m_cgmsData );
  m_model->addGlucoseAbsorption( *m_carbConsumptionData );
  if( m_meterData ) m_model->addFingerStickData( *m_meterData );
  if(m_customData) m_model->addCustomEvents( *m_customData );

  close();
}//void NlSimpleCreate::constructModel()

void NlSimpleCreate::openDefinedModel()
{
  m_model = openNLSimpleModelFile( this );

  if( m_model ) this->close();
}//void NlSimpleCreate::openDefinedModel()

void NlSimpleCreate::cancel()
{
    m_model = NULL;
    close();
}//void NlSimpleCreate::cancel()


void NlSimpleCreate::handleTimeLimitButton()
{
    m_userSetTime = true;
    QDateTime startTime = m_ui->m_startDateEntry->dateTime();
    QDateTime endTime = m_ui->m_endDateEntry->dateTime();

    if( endTime < startTime )
    {
        m_userSetTime = false;
        findTimeLimits();
    }//if( endTime < startTime )

    int curPad = m_ui->tabWidget->currentIndex();
    drawPreview( kCGMS_PAD );
    drawPreview( kBOLUS_PAD );
    drawPreview( kCARB_PAD );
    drawPreview( kMETER_PAD );
    drawPreview( kCustom_PAD );
    m_ui->tabWidget->setCurrentIndex(curPad);
}//void NlSimpleCreate::handleTimeLimitButton()


void NlSimpleCreate::enableCreateButton()
{
  double insalinValue = m_ui->m_basalInsulinAmount->value();

  if( m_cgmsData && (m_insulinData || m_bolusData)
           && ( m_carbConsumptionData )
           && (insalinValue > 0.0) )
   m_ui->m_createButton->setEnabled(true);
}//void NlSimpleCreate::enableCreateButton()

void NlSimpleCreate::zoomInX()
{
    m_minutesGraphPerPage = 0.9 * m_minutesGraphPerPage;
    updateModelGraphSize();
}//void NlSimpleCreate::zoomInX()


void NlSimpleCreate::zoomOutX()
{
    m_minutesGraphPerPage = 1.1 * m_minutesGraphPerPage;
    updateModelGraphSize();
}//void NlSimpleCreate::zoomOutX()

void NlSimpleCreate::changeMassUnits()
{
  if( m_useKgs )
  {
    double kgs = (double) m_ui->m_weightInput->value();
    m_ui->m_weightInput->setValue(kgs * 2.20462262 + 2); //the 2 is cause of integer truncations
    m_ui->m_unitButton->setText("lbs");
  }else
  {
    double lbs = (double) m_ui->m_weightInput->value();
    m_ui->m_weightInput->setValue(lbs * 0.45359237 );
    m_ui->m_unitButton->setText("kg");
  }

  m_useKgs = !m_useKgs;
}//void NlSimpleCreate::changeMassUnits()

