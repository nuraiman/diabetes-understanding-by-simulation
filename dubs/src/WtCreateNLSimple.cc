#include "DubsConfig.hh"

#include <set>
#include <cmath>
#include <vector>
#include <string>
#include <math.h>
#include <iomanip>
#include <cstdlib>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <iostream>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <Wt/WText>
#include <Wt/WTable>
#include <Wt/WLabel>
#include <Wt/WLabel>
#include <Wt/WDialog>
#include <Wt/WSpinBox>
#include <Wt/WLineEdit>
#include <Wt/WTableView>
#include <Wt/WTabWidget>
#include <Wt/WPushButton>
#include <Wt/WDatePicker>
#include <Wt/WGridLayout>
#include <Wt/WFileUpload>
#include <Wt/WEnvironment>
#include <Wt/WApplication>
#include <Wt/WBorderLayout>
#include <Wt/Dbo/QueryModel>
#include <Wt/WLengthValidator>
#include <Wt/WContainerWidget>
#include <Wt/WAbstractItemModel>
#include <Wt/WStandardItemModel>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Chart/WCartesianChart>

#if(USE_CERNS_ROOT)
#include "TH1.h"
#include "TH2.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TSystem.h"
#include "TRandom3.h"
#endif  //#if(USE_CERNS_ROOT)

#include "dubs/WtUtils.hh"
#include "dubs/ResponseModel.hh"
#include "dubs/CgmsDataImport.hh"
#include "dubs/WtUserManagment.hh"
#include "dubs/WtCreateNLSimple.hh"
#include "dubs/ConsentrationGraph.hh"
#include "dubs/ArtificialPancrease.hh"


using namespace Wt;
using namespace std;

#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH


WtCreateNLSimple::WtCreateNLSimple( Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_model(),
    m_modelName( "InialModel" )
{
  setInline(false);
  setStyleClass( "WtCreateNLSimple" );

  init();
}//WtCreateNLSimple constructor


std::string WtCreateNLSimple::modelName() const
{
  return m_modelName;
}


void WtCreateNLSimple::doEmit( Wt::Signal<> &signal )
{
  signal.emit();
}//void doEmit( Wt::Signal<> &signal )

Wt::Signal<> &WtCreateNLSimple::created()
{
  return m_created;
}


Wt::Signal<> &WtCreateNLSimple::canceled()
{
  return m_canceled;
}


boost::shared_ptr<NLSimple> WtCreateNLSimple::model()
{
  return m_model;
}


void WtCreateNLSimple::init()
{
  clear();
  foreach( ConsentrationGraph *g, m_datas )
    if( g )
      delete g;

  m_datas.resize( kNUM_DataType, NULL ),
  m_fileUploads.resize( kNUM_DataType, NULL );
  m_sourceDescripts.resize( kNUM_DataType, NULL );
  m_userSetTime = false;

  WGridLayout *layout = new WGridLayout();
  layout->setContentsMargins( 0, 0, 0, 0 );
  setLayout( layout );

  m_graph = new Chart::WCartesianChart( Chart::ScatterPlot );
  m_graph->setMinimumSize( 200, 200 );
  m_graphModel = new WStandardItemModel( 0, kNUM_DataType+1, this );
  m_graph->setModel( m_graphModel );
  m_graph->setXSeriesColumn(kNUM_DataType);
  m_graph->setLegendEnabled(true);
  m_graph->setPlotAreaPadding( 200, Wt::Right );
  m_graph->setPlotAreaPadding( 70, Wt::Bottom );
  m_graph->axis(Chart::XAxis).setScale(Chart::DateTimeScale);
  m_graph->axis(Chart::XAxis).setLabelAngle(45.0);
  m_graph->axis(Chart::YAxis).setTitle( "mg/dL" );

  for( DataType dt = DataType(0); dt < kNUM_DataType; dt = DataType(dt+1) )
  {
    switch( dt )
    {
      case kCGMS_ENTRY:
        m_graph->addSeries( Chart::WDataSeries( dt, Chart::LineSeries ) );
      break;
      case kBOLUS_ENTRY: case kCARB_ENTRY: case kMETER_ENTRY:
            //kCustom_ENTRY, kEXCERSIZE_ENTRY, kALL_ENTRY,
        m_graph->addSeries( Chart::WDataSeries( dt, Chart::PointSeries ) );
      break;
      case kNUM_DataType: assert(0);
    };//switch( det )

    switch( dt )
    {
      case kCGMS_ENTRY:  m_graphModel->setHeaderData( dt, WString("CGMS Data")  ); break;
      case kBOLUS_ENTRY: m_graphModel->setHeaderData( dt, WString("Bolus Data") ); break;
      case kCARB_ENTRY:  m_graphModel->setHeaderData( dt, WString("Meal Data")  ); break;
      case kMETER_ENTRY: m_graphModel->setHeaderData( dt, WString("Meter Data") ); break;
      //kCustom_ENTRY, kEXCERSIZE_ENTRY, kALL_ENTRY,
      case kNUM_DataType: assert(0);
    };//switch( det )
  }//for( loop over DataType's


  WText *introText = new WText( "You must upload at least CGMS data, Bolus data, "
                                "and Meal data in order to create a new model, do"
                                " this below:" );
  introText->setAttributeValue( "style", "border-bottom: solid 1px black;" );
  int row = 0;
  layout->addWidget( introText, row++, 0, 1, kNUM_DataType+1 );


  layout->addWidget( m_graph, row++, 0, 1, kNUM_DataType+1 );
  layout->setRowStretch( row-1, 5 );
  layout->setColumnStretch( kNUM_DataType, 5 );


  Div *timeRangeDiv = new Div( "WtCreateNLSimple_timeRangeDiv" );
  m_startTime = new DateTimeSelect( "Begin Time",
                                    WDateTime::fromPosixTime(kGenericT0),
                                    timeRangeDiv );
  m_endTime   = new DateTimeSelect( "End Time",
                                    WDateTime::fromPosixTime(kGenericT0),
                                    timeRangeDiv );

  m_endTime->setMargin( 25, Wt::Left );
  m_startTime->setPadding( 20, Wt::Right );
  layout->addWidget( timeRangeDiv, row++, 0, 1, kNUM_DataType, AlignLeft );

  m_endTime->changed().connect( this, &WtCreateNLSimple::handleTimeLimitButton );
  m_startTime->changed().connect( this, &WtCreateNLSimple::handleTimeLimitButton );


  Div *fileInputDiv = new Div( "WtCreateNLSimple_fileInputLayout" );

  WGridLayout *fileInputLayout = new WGridLayout();
  fileInputDiv->setLayout( fileInputLayout );

  for( DataType dt = DataType(0); dt < kNUM_DataType; dt = DataType(dt+1) )
  {
    string title = "";
    switch( dt )
    {
      case kCGMS_ENTRY:  title = "<b>CGMS Data:</b>";   break;
      case kBOLUS_ENTRY: title = "<b>Bolus Data:</b>";  break;
      case kCARB_ENTRY:  title = "<b>Meal Data:</b>";   break;
      case kMETER_ENTRY: title = "<b>Meter Data (non-cal):</b>";  break;
            //kCustom_ENTRY, kEXCERSIZE_ENTRY, kALL_ENTRY,
      case kNUM_DataType: assert(0);
    };//switch( det )

    fileInputLayout->addWidget( new WText( title, XHTMLUnsafeText ), 0, dt, 1, 1, AlignLeft );

    Div *fileInputDiv = new Div( "WtCreateNLSimple_fileInputDiv" );
    WFileUpload *upload = m_fileUploads[dt] = new WFileUpload( fileInputDiv );
    upload->changed().connect( upload, &WFileUpload::upload);
    upload->uploaded().connect( boost::bind( &WtCreateNLSimple::addData, this, dt ) );
    upload->fileTooLarge().connect( this, &WtCreateNLSimple::failedUpload );

    fileInputLayout->addWidget( fileInputDiv, 1, dt, 1, 1, Wt::AlignCenter );
    fileInputLayout->setColumnStretch( dt, 1 );
    m_sourceDescripts[dt] = new WText();
    fileInputLayout->addWidget( m_sourceDescripts[dt], 2, dt, 1, 1, AlignLeft );
    fileInputLayout->setRowStretch( 2, 3 );
  }//for( loop over DataType's


  layout->addWidget( fileInputDiv, row++, 0, 1, kNUM_DataType, AlignCenter );

  Div *basalDiv = new Div();
  WLabel *label = new WLabel( "Basal Insulin: ", basalDiv );
  m_basalInsulin = new WDoubleSpinBox( basalDiv );
  label->setBuddy( m_basalInsulin );
  m_basalInsulin->setValue( 1.0 );
  m_basalInsulin->setMinimum( 0.0 );
  m_basalInsulin->setMaximum( 4.0 );
  m_basalInsulin->setSingleStep( 0.05 );
  m_basalInsulin->setTextSize(4);
  m_basalInsulin->setMaxLength(4);
  m_basalInsulin->setStyleClass( "WtCreateNLSimple_m_basalInsulin" );
  new WLabel( " u", basalDiv );

  label = new WLabel( " Your weight: ", basalDiv );
  label->setMargin( 25, Wt::Left );
  m_weightInput = new WDoubleSpinBox( basalDiv );
  m_weightInput->setSingleStep( 1.0 );
  m_weightInput->setValue( ProgramOptions::kPersonsWeight );
  m_weightInput->setMinimum( 0.0 );
  m_weightInput->setTextSize(3);
  m_weightInput->setMaxLength(3);
  label->setBuddy( m_weightInput );
  m_weightInput->setStyleClass( "WtCreateNLSimple_m_weightInput" );
  new WLabel( " kg", basalDiv );

  label = new WLabel( " Model Name: ", basalDiv );
  m_modelNameEdit = new WLineEdit( m_modelName, basalDiv );
  label->setBuddy( m_modelNameEdit );

  layout->addWidget( basalDiv, row++, 0, 1, kNUM_DataType, AlignLeft );


  Div *buttonDiv = new Div( "WtCreateNLSimple_buttonDiv" );
  m_createButton = new WPushButton( "Create" , buttonDiv );
  m_cancelButton = new WPushButton( "Cancel" , buttonDiv );
  layout->addWidget( buttonDiv, row, 0, 1, 1, AlignLeft );

  m_createButton->setDisabled(true);
  m_createButton->clicked().connect( boost::bind( &WtCreateNLSimple::doEmit, this, boost::ref(m_created) ) );
  m_createButton->clicked().connect( this, &WtCreateNLSimple::constructModel );
  m_cancelButton->clicked().connect( boost::bind( &WtCreateNLSimple::doEmit, this, boost::ref(m_canceled) ) );
}//void init()


WtCreateNLSimple::~WtCreateNLSimple()
{
  //m_model  is not owned by this objcet
  foreach( ConsentrationGraph *g, m_datas )
    if( g )
      delete g;
}//~WtCreateNLSimple()


void WtCreateNLSimple::findTimeLimits()
{
  //This functions logic salvaged from old GUI implementations
  //
  //Don't let user control time span before loading any graphs
  if( m_userSetTime )
  {
    m_userSetTime = false;
    for( DataType i = DataType(0); i < kNUM_DataType; i = DataType(i+1) )
      m_userSetTime = (m_userSetTime || (m_datas[i]));
  }//if( m_userSetTime )

  const PosixTime currStartTime = m_startTime->dateTime().toPosixTime();
  const PosixTime currEndTime = m_endTime->dateTime().toPosixTime();

  if( (currEndTime == kGenericT0) && (currStartTime == kGenericT0))
    m_userSetTime = false;

  if( m_userSetTime ) return;

  PosixTime endTime = kGenericT0;
  PosixTime startTime = kGenericT0;

  if( m_datas[kCGMS_ENTRY] )
  {
    endTime = m_datas[kCGMS_ENTRY]->getEndTime();
    startTime = m_datas[kCGMS_ENTRY]->getStartTime();
  }//if( m_cgmsData )

  if( m_datas[kBOLUS_ENTRY] )
  {
    PosixTime end = m_datas[kBOLUS_ENTRY]->getEndTime() + TimeDuration( 24, 0, 0, 0 );
    PosixTime start = m_datas[kBOLUS_ENTRY]->getStartTime() - TimeDuration( 24, 0, 0, 0 );
    startTime = std::max( startTime, start );
    endTime = (endTime == kGenericT0) ? end : std::min( endTime, end );
  }//if( m_bolusData )

  if( m_datas[kCARB_ENTRY] )
  {
    PosixTime end = m_datas[kCARB_ENTRY]->getEndTime()
                   + TimeDuration( 24, 0, 0, 0 );
    PosixTime start = m_datas[kCARB_ENTRY]->getStartTime()
                      - TimeDuration( 24, 0, 0, 0 );
    startTime = std::max( startTime, start );
    endTime = (endTime == kGenericT0) ? end : std::min( endTime, end );
  }//if( m_carbConsumptionData )


  //if( (endTime == kGenericT0) && (startTime  == kGenericT0) && m_datas[kCustom_ENTRY] )
  //{
  //  endTime = m_datas[kCustom_ENTRY]->getEndTime() + TimeDuration( 24, 0, 0, 0 );
  //  startTime = m_datas[kCustom_ENTRY]->getStartTime() - TimeDuration( 24, 0, 0, 0 );
  //}//if( m_carbConsumptionData )


  if( (endTime != kGenericT0) && (startTime != kGenericT0) )
  {
    m_startTime->setTop( WDateTime::fromPosixTime(endTime) );
    m_endTime->setTop( WDateTime::fromPosixTime(endTime) );
    m_startTime->setBottom( WDateTime::fromPosixTime(startTime) );
    m_endTime->setBottom( WDateTime::fromPosixTime(startTime) );
    m_startTime->set( WDateTime::fromPosixTime(startTime) );
    m_endTime->set( WDateTime::fromPosixTime(endTime) );
  }//if( we have time limits )

  m_graph->axis(Chart::XAxis).setAutoLimits( Chart::MinimumValue | Chart::MaximumValue );

  if( endTime != kGenericT0 )
  {
    const double endTimeAsDouble = asNumber( WDateTime::fromPosixTime(endTime) );
    m_graph->axis(Chart::XAxis).setMaximum( endTimeAsDouble );
  }//if( endTime != kGenericT0 )

  if( startTime != kGenericT0 )
  {
    const double startTimeAsDouble = asNumber( WDateTime::fromPosixTime(startTime) );
    m_graph->axis(Chart::XAxis).setMinimum( startTimeAsDouble );
  }//if( startTime != kGenericT0 )
}//void WtCreateNLSimple::findTimeLimits()

void WtCreateNLSimple::drawPreview()
{
  typedef ConsentrationGraph::value_type cg_type;

  int nNeededRow = 0;

  for( DataType i = DataType(0); i < kNUM_DataType; i = DataType(i+1) )
    if( m_datas[i] ) nNeededRow += m_datas[i]->size();

  m_graphModel->removeRows( 0, m_graphModel->rowCount() );
  m_graphModel->insertRows( 0, nNeededRow );

  int row = 0;
  for( DataType i = DataType(0); i < kNUM_DataType; i = DataType(i+1) )
  {
    ConsentrationGraph *graph = m_datas[i];
    if( !graph ) continue;

    foreach( const cg_type &element, (*graph) )
    {
      WDateTime x;
      x.setPosixTime( element.m_time );
      m_graphModel->setData( row, kNUM_DataType, x );
      m_graphModel->setData( row++, i, element.m_value );
    }//foreach element in the graph
  }//for( each ConsentrationGraph we have )

  findTimeLimits();
}//void WtCreateNLSimple::drawPreview()


void WtCreateNLSimple::addData( WtCreateNLSimple::DataType type )
{
  using CgmsDataImport::importSpreadsheet;

  CgmsDataImport::InfoType infoType = CgmsDataImport::InfoType(0);

  switch( type )
  {
    case kCGMS_ENTRY:  infoType = CgmsDataImport::CgmsReading;  break;
    case kBOLUS_ENTRY: infoType = CgmsDataImport::BolusTaken;   break;
    case kCARB_ENTRY:  infoType = CgmsDataImport::GlucoseEaten; break;
    case kMETER_ENTRY: infoType = CgmsDataImport::MeterReading; break;
    //kCustom_ENTRY, kEXCERSIZE_ENTRY, kALL_ENTRY,
    case kNUM_DataType: assert(0);
  };//switch( det )

  const string fileName = m_fileUploads.at(type)->spoolFileName();

  try
  {
    ConsentrationGraph importedData = importSpreadsheet( fileName, infoType );

    if( m_datas[type]  ) m_datas[type]->addNewDataPoints( importedData );
    else                 m_datas[type] = new ConsentrationGraph( importedData );

  }catch( exception &e )
  {
    string msg = "Warning: failed try/catch in WtCreateNLSimple::"
                 "addData( WtCreateNLSimple::DataType type ):\n";
    msg += e.what();
    wApp->doJavaScript( "alert( '" + msg + "' );", false );
    cerr << msg << endl;
    return;
  }catch(...)
  {
    const string msg = "Warning: failed try/catch in WtCreateNLSimple::"
                       "addData( WtCreateNLSimple::DataType type )";
    wApp->doJavaScript( "alert( '" + msg + "' );", false );
    cerr << msg << endl;
    return;
  }//try / catch

  //may need to reset here m_fileUploads

  const WString realFileName = m_fileUploads.at(type)->clientFileName();
  WString descript = m_sourceDescripts[type]->text();
  if( descript.empty() ) descript = realFileName;
  else                   descript += (", " + realFileName);
  m_sourceDescripts[type]->setText( descript );

  drawPreview();
  enableCreateButton();
}//void addData( DataType type )


void WtCreateNLSimple::constructModel()
{
  assert( m_datas[kCGMS_ENTRY] );
  assert( m_datas[kCARB_ENTRY] );
  // m_datas[kMETER_ENTRY];
  assert( m_datas[kBOLUS_ENTRY] );

  m_modelName = m_modelNameEdit->valueText().narrow();

  double insPerHour = m_basalInsulin->value();
  double weight = m_weightInput->value(); //kg s

  PosixTime startTime = m_startTime->dateTime().toPosixTime();
  PosixTime endTime = m_endTime->dateTime().toPosixTime();

  for( DataType i = DataType(0); i < kNUM_DataType; i = DataType(i+1) )
  {
    ConsentrationGraph *graph = m_datas[i];
    if( graph ) graph->trim( startTime, endTime, (i==kCGMS_ENTRY) );
  }//foreach potential graph

  m_model.reset( new NLSimple( "SimpleModel",
                          insPerHour,
                          ProgramOptions::kBasalGlucConc,
                          m_datas[kBOLUS_ENTRY]->getStartTime() ) );

  m_model->m_settings.m_personsWeight = weight;

  m_model->addBolusData( *(m_datas[kBOLUS_ENTRY]) );

  cerr << "Added " << m_datas[kBOLUS_ENTRY]->size() << " bolusses to give "
      << m_model->m_freePlasmaInsulin.size() << " insulin infos" << endl;

  m_model->addCgmsData( *(m_datas[kCGMS_ENTRY]) );
  m_model->addGlucoseAbsorption( *(m_datas[kCARB_ENTRY]) );
  if( m_datas[kMETER_ENTRY] ) m_model->addNonCalFingerStickData( *(m_datas[kMETER_ENTRY]) );
  //if( m_datas[kCustom_ENTRY] ) m_model->addCustomEvents( *(m_datas[kCustom_ENTRY]) );

}//void WtCreateNLSimple::constructModel()


void WtCreateNLSimple::handleTimeLimitButton()
{
  m_userSetTime = true;
  const PosixTime startTime = m_startTime->dateTime().toPosixTime();
  const PosixTime endTime = m_endTime->dateTime().toPosixTime();

  if( endTime < startTime )
  {
    m_userSetTime = false;
    findTimeLimits();
    return;
  }//if( endTime < startTime )

  //drawPreview();

  m_graph->axis(Chart::XAxis).setAutoLimits( Chart::MinimumValue | Chart::MaximumValue );

  if( endTime != kGenericT0 )
  {
    const double endTimeAsDouble = asNumber( WDateTime::fromPosixTime(endTime) );
    m_graph->axis(Chart::XAxis).setMaximum( endTimeAsDouble );
  }//if( endTime != kGenericT0 )

  if( startTime != kGenericT0 )
  {
    const double startTimeAsDouble = asNumber( WDateTime::fromPosixTime(startTime) );
    m_graph->axis(Chart::XAxis).setMinimum( startTimeAsDouble );
  }//if( startTime != kGenericT0 )

  m_graph->update();
}//void handleTimeLimitButton()

void WtCreateNLSimple::enableCreateButton()
{
  if( (m_datas[kBOLUS_ENTRY])
      && (m_datas[kCGMS_ENTRY])
      && (m_datas[kCARB_ENTRY])
      && (m_basalInsulin->value()> 0.0) )
    m_createButton->setEnabled(true);
}//void enableCreateButton()

void WtCreateNLSimple::checkDisplayTimeLimitsConsistency()
{
  const PosixTime startTime = m_startTime->dateTime().toPosixTime();
  const PosixTime endTime = m_endTime->dateTime().toPosixTime();

  if( startTime <= endTime )    return;
  if( kGenericT0 == endTime )   return;
  if( kGenericT0 == startTime ) return;

  cerr << "Warning in WtCreateNLSimple::checkDisplayTimeLimitsConsistency(), inconsitency!" << endl;
  m_userSetTime = false;
  findTimeLimits();
}//void checkDisplayTimeLimitsConsistency()


void WtCreateNLSimple::failedUpload( ::int64_t type )
{
  wApp->doJavaScript( "alert( \"CreateNLSimple::failedUpload(...) failed to "
                      "upload file of type "
                      + boost::lexical_cast<string>((int)type) + "\" )", false );
  cerr << "CreateNLSimple::failedUpload(...) failed to upload file of type "
       << type << endl;
}//void failedUpload( DataType type )
