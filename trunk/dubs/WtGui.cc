#include <set>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fstream>
#include <cmath>

#include <sqlite3.h>

#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/algorithm/string.hpp"

#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WContainerWidget>
#include <Wt/Chart/WCartesianChart>
#include <Wt/WBorderLayout>
#include <Wt/WTabWidget>
#include <Wt/WStandardItemModel>
#include <Wt/WTableView>
#include <Wt/WDatePicker>
#include <Wt/WLabel>
#include <Wt/WSpinBox>
#include <Wt/WDatePicker>
#include <Wt/WPopupMenu>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WTable>
#include <Wt/WPainter>
#include <Wt/Chart/WDataSeries>
#include <Wt/WTime>
#include <Wt/WDate>
#include <Wt/WDateTime>
#include <Wt/WComboBox>
#include <Wt/WIntValidator>
#include <Wt/WDoubleValidator>
#include <Wt/WDialog>
#include <Wt/WLengthValidator>
#include <Wt/Dbo/QueryModel>
#include <Wt/WTableView>
#include <Wt/WItemDelegate>
#include <Wt/WLineF>
#include <Wt/WPen>
#include <Wt/Chart/WChartPalette>

#include "WtGui.hh"

#include "TH1.h"
#include "TH1F.h"
#include "TH2.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TRandom3.h"
#include "TSystem.h"

#include "ResponseModel.hh"
#include "ProgramOptions.hh"

using namespace Wt;
using namespace std;


#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

const std::string DubsLogin::cookie_name = "dubsuserid";

void temp(){cerr <<"Working" << endl;}

WtGui::WtGui( const Wt::WEnvironment& env )
  : WApplication( env ),
  m_model( NULL ),
  m_dbBackend( "user_information.db" ),
  m_dbSession(),
  m_upperEqnDiv( NULL ),
  m_actionMenuPopup( NULL ),
  m_tabs( NULL ),
  m_bsModel( NULL ),
  m_bsGraph( NULL ),
  m_bsBeginTimePicker( NULL ),
  m_bsEndTimePicker( NULL ),
  m_errorGridModel( NULL ),
  m_errorGridGraph( NULL ),
  m_customEventsView( NULL ),
  m_customEventsModel( NULL ),
  m_customEventGraphs( 0 )
{
  setTitle( "dubs" );
  useStyleSheet( "/local_resources/dubs.css" );

  m_dbSession.setConnection( m_dbBackend );
  m_dbBackend.setProperty("show-queries", "true");
  m_dbSession.mapClass<DubUser>("DubUser");
  m_dbSession.mapClass<UsersModel>("UsersModel");
  try{ m_dbSession.createTables(); }catch(...){}

  requireLogin();
}//WtGui constructor



void WtGui::requireLogin()
{
  const string logged_in_username = DubsLogin::isLoggedIn( m_dbSession );

  if( logged_in_username != "" )
  {
    init( logged_in_username );
    return;
  }//if( isLoggedIn() )

  root()->clear();
  DubsLogin *login = new DubsLogin( m_dbSession, root() );
  login->loginSuccessful().connect( boost::bind( &WtGui::init, this, _1 ) );
  login->loginSuccessful().connect( boost::bind( &DubsLogin::insertCookie, _1, 3024000, boost::ref(m_dbSession) ) );
}//void WtGui::requireLogin()


void WtGui::logout()
{  
  delete m_model;
  m_model = NULL;
  DubsLogin::insertCookie( DubsLogin::cookie_name, 0, m_dbSession ); //deltes the cookie
  requireLogin();
}//void logout()


void WtGui::resetGui()
{
  delete m_model;
  m_model = NULL;
  init( m_userDbPtr->name );
}//void resetGui()


void WtGui::deleteModel( const string &modelname )
{
  Dbo::Transaction transaction(m_dbSession);
  Dbo::ptr<UsersModel> model = m_userDbPtr.modify()->models.find().where("fileName = ?").bind( modelname );
  if( model ) m_userDbPtr.modify()->models.erase( model );
  transaction.commit();

  if( m_userDbPtr->currentFileName == modelname )
  {
    setModelFileName( "" );
    resetGui();
  }//if( m_userDbPtr->currentFileName == modelname )
}//void deleteModel( const string model )

void WtGui::enableOpenModelDialogOkayButton( WPushButton *button, WTableView *view )
{
  std::set<WModelIndex> selected = view->selectedIndexes();
  if( selected.empty() ) button->disable();
  else button->enable();
}//void enableOpenModelDialogOkayButton( WPushButton *button, WTableView *view )


void WtGui::openModelDialog()
{
  saveModelConfirmation();

  Dbo::Transaction transaction(m_dbSession);

  if( !m_userDbPtr || !(m_userDbPtr->models.size()) )
  {
    newModel();
    transaction.commit();
    return;
  }//if( no models owned by user )

  WDialog dialog( "Open Model" );
  new WText("Select model to open: ", dialog.contents() );
  new WBreak( dialog.contents() );
  Dbo::QueryModel<Dbo::ptr<UsersModel> > *viewmodel = new Dbo::QueryModel<Dbo::ptr<UsersModel> >( dialog.contents() );
  viewmodel->setQuery( m_userDbPtr->models.find() );
  viewmodel->addColumn( "fileName", "File Name" );
  viewmodel->addColumn( "created", "Created" );
  viewmodel->addColumn( "modified", "Modified" );

  // cerr << "Found " << m_userDbPtr->models.find().resultList().size()
  //      << " potential models to open" << endl;

  Div *viewDiv = new Div( "viewDiv", dialog.contents() );
  WTableView *view = new WTableView( viewDiv );
  view->setModel( viewmodel );
  view->setSortingEnabled(true);
  view->setColumnResizeEnabled(true);
  view->setAlternatingRowColors(true);
  view->setSelectionBehavior( SelectRows );
  view->setSelectionMode( SingleSelection );
  view->setRowHeight(22);
  view->setColumnWidth( 0, 150 );
  view->setColumnWidth( 1, 150 );
  view->setColumnWidth( 2, 150 );
  view->resize( 50+3*150, min( 500, 20 + 22*viewmodel->rowCount() ) );

  new WBreak( dialog.contents() );
  WPushButton *ok = new WPushButton( "Open", dialog.contents() );
  ok->setEnabled( false );
  WPushButton *cancel   = new WPushButton( "Cancel", dialog.contents() );
  WPushButton *newModelB = new WPushButton( "Create New Model", dialog.contents() );

  ok->clicked().connect(       &dialog, &WDialog::accept );
  newModelB->clicked().connect( &dialog, &WDialog::accept );
  newModelB->clicked().connect( boost::bind( &WTableView::setSelectedIndexes, view, WModelIndexSet() ) );
  cancel->clicked().connect(   &dialog, &WDialog::reject );
  view->selectionChanged().connect(  boost::bind( &WtGui::enableOpenModelDialogOkayButton, this, ok, view ) );
  enableOpenModelDialogOkayButton( ok, view );

  WDialog::DialogCode code = dialog.exec();

  if( code == WDialog::Accepted )
  {
    std::set<WModelIndex> selected = view->selectedIndexes();

    if( selected.empty() )
    {
      //user hit 'Create New Model' button
      newModel();
      transaction.commit();
      return;
    }//if( selected.empty() )

    try
    {
      const int selectedRow = selected.begin()->row();
      setModelFileName( boost::any_cast<string>( viewmodel->data( selectedRow, 0 ) ) );
      resetGui();
    }catch(...)
    { cerr << endl << "Any Cast Failed in openModelDialog()" << endl; }

    transaction.commit();
    return;
  }//if( code == WDialog::Accepted )

  transaction.commit();
  newModel();
}//void openModelDialog()


void WtGui::init( const string username )
{
  {
    Dbo::Transaction transaction(m_dbSession);
    m_userDbPtr = m_dbSession.find<DubUser>().where("name = ?").bind( username );
    transaction.commit();
  }

  if( !m_userDbPtr )
  {
    cerr << username << " doesn't have a currentFile." << endl;
    requireLogin();
    return;
  }//if( !m_userDbPtr )

  root()->clear();
  root()->setStyleClass( "root" );
  WBorderLayout *layout = new WBorderLayout();
  root()->setLayout( layout );

  if( m_userDbPtr->currentFileName.empty() && !m_model )
  {
    openModelDialog();
    return;
  }else if( !m_userDbPtr->currentFileName.empty() )
  {
    assert( !m_model );
    setModel( m_userDbPtr->currentFileName );
  }

  if( m_model == NULL )
  {
    init( m_userDbPtr->name );
    return;
  }

  m_upperEqnDiv = new Div( "m_upperEqnDiv" );
  m_actionMenuPopup = new WPopupMenu();
  WPushButton *actionMenuButton = new WPushButton( "Actions", m_upperEqnDiv );
  actionMenuButton->clicked().connect( boost::bind( &WPopupMenu::exec, m_actionMenuPopup, actionMenuButton, Wt::Vertical) );
  actionMenuButton->clicked().connect( boost::bind( &WPushButton::disable, actionMenuButton ) );

  m_actionMenuPopup->aboutToHide().connect( boost::bind( &WPushButton::enable, actionMenuButton ) );
  WPopupMenuItem *item = m_actionMenuPopup->addItem( "Save Model" );
  item->triggered().connect( boost::bind( &WtGui::saveModel, this, boost::ref(m_userDbPtr->currentFileName) ) );
  item = m_actionMenuPopup->addItem( "Save As" );
  item->triggered().connect( boost::bind( &WtGui::saveModelAsDialog, this ) );
  item = m_actionMenuPopup->addItem( "Open Model" );
  item->triggered().connect( boost::bind( &WtGui::openModelDialog, this ) );
  item = m_actionMenuPopup->addItem( "New Model" );
  item->triggered().connect( boost::bind( &WtGui::newModel, this ) );

  layout->addWidget( m_upperEqnDiv, WBorderLayout::North );

  m_tabs = new WTabWidget();
  m_tabs->setStyleClass( "m_tabs" );
  layout->addWidget( m_tabs, WBorderLayout::Center );

  m_bsModel = new WStandardItemModel( this );
  m_bsModel->insertColumns( m_bsModel->columnCount(), NumDataSources );
  m_bsModel->setHeaderData( kTimeData,              WString("Time"));
  m_bsModel->setHeaderData( kCgmsData,              WString("CGMS Readings") );
  m_bsModel->setHeaderData( kFreePlasmaInsulin,     WString("Free Plasma Insulin (pred.)") );
  m_bsModel->setHeaderData( kGlucoseAbsRate,        WString("Glucose Abs. Rate (pred.)") );
  m_bsModel->setHeaderData( kMealData,              WString("Consumed Carbohydrates") );
  m_bsModel->setHeaderData( kFingerStickData,       WString("Finger Stick Readings") );
  m_bsModel->setHeaderData( kCustomEventData,       WString("User Defined Events") );
  m_bsModel->setHeaderData( kPredictedBloodGlucose, WString("Predicted Blood Glucose") );
  m_bsModel->setHeaderData( kPredictedInsulinX,     WString("Insulin X (pred.)") );
  
  m_bsGraph = new Chart::WCartesianChart(Chart::ScatterPlot);
  m_bsGraph->setModel( m_bsModel );
  m_bsGraph->setXSeriesColumn(0);
  m_bsGraph->setLegendEnabled(true);
  m_bsGraph->setPlotAreaPadding( 200, Wt::Right );
  m_bsGraph->setPlotAreaPadding( 70, Wt::Bottom );
  m_bsGraph->axis(Chart::XAxis).setScale(Chart::DateTimeScale);
  m_bsGraph->axis(Chart::XAxis).setLabelAngle(45.0);
  m_bsGraph->axis(Chart::Y2Axis).setVisible(true);

  Chart::WDataSeries cgmsSeries(kCgmsData, Chart::LineSeries);
  cgmsSeries.setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
  m_bsGraph->addSeries( cgmsSeries );

  Chart::WDataSeries fingerSeries( kFingerStickData, Chart::PointSeries );
  m_bsGraph->addSeries( fingerSeries );

  Chart::WDataSeries mealSeries( kMealData, Chart::PointSeries, Chart::Y2Axis );
  m_bsGraph->addSeries( mealSeries );
  Chart::WDataSeries predictSeries( kPredictedBloodGlucose, Chart::LineSeries );
  m_bsGraph->addSeries( predictSeries );



  m_errorGridModel = new WStandardItemModel(  this );
  m_errorGridModel->insertColumns( m_errorGridModel->columnCount(), NumErrorRegions );

  m_errorGridGraph = new ClarkErrorGridGraph( Chart::ScatterPlot );
  m_errorGridLegend = new Div( "m_errorGridLegend" );
  m_errorGridGraph->setModel( m_errorGridModel );
  m_errorGridGraph->setXSeriesColumn(0);
  //m_errorGridGraph->setLegendEnabled(true);
  m_errorGridGraph->setPlotAreaPadding( 50, Wt::Left );
  m_errorGridGraph->setPlotAreaPadding( 20, Wt::Right );
  m_errorGridGraph->setPlotAreaPadding( 50, Wt::Bottom );
  m_errorGridGraph->axis(Chart::XAxis).setRange( 0.0, 400.0 );
  m_errorGridGraph->axis(Chart::YAxis).setRange( 0.0, 400.0 );

  for( ErrorRegions reg(kRegionA); reg < NumErrorRegions; reg = ErrorRegions(reg+1) )
  {
    Chart::WDataSeries series(reg, Chart::PointSeries);
    m_errorGridGraph->addSeries( series );
  }
  updateClarkAnalysis( m_model->m_fingerMeterData, m_model->m_cgmsData, true );

  m_customEventsView = new WTableView();
  m_customEventsModel = new WStandardItemModel( this );
  m_customEventsView->setModel( m_customEventsModel );
  m_customEventGraphs.clear();


  /*
  foreach( NLSimple::EventDefMap::value_type &t, m_model->m_customEventDefs )
  {
  }
  */

  Div *datePickingDiv  = new Div( "datePickingDiv" );
  WDateTime now( WDate(2010,1,3), WTime(2,30) );
  m_bsBeginTimePicker  = new DateTimeSelect( "Start Date/Time:&nbsp;",
                                             now, datePickingDiv );
  m_bsEndTimePicker    = new DateTimeSelect( "&nbsp;&nbsp;&nbsp;&nbsp;End Date/Time:&nbsp;",
                                             now, datePickingDiv );
  m_bsBeginTimePicker->changed().connect( boost::bind( &WtGui::updateDisplayedDateRange, this ) );
  m_bsEndTimePicker->changed().connect( boost::bind( &WtGui::updateDisplayedDateRange, this ) );

  updateDataRange();
  WDateTime start, end;
  end.setPosixTime( m_model->m_cgmsData.getEndTime() );
  start.setPosixTime( m_model->m_cgmsData.getStartTime() );
  m_bsEndTimePicker->set( end );
  m_bsBeginTimePicker->set( start );


  Div *bsTabDiv = new Div( "bsTabDiv" );
  WBorderLayout *bsTabLayout = new WBorderLayout();
  bsTabLayout->setSpacing( 0 );
  bsTabLayout->setContentsMargins( 0, 0, 0, 0 );
  bsTabDiv->setLayout( bsTabLayout );
  bsTabLayout->addWidget( m_bsGraph, WBorderLayout::Center );
  bsTabLayout->addWidget( datePickingDiv, WBorderLayout::South );
  m_tabs->addTab( bsTabDiv, "Display" );

  /*Div *optionsTabDiv = new Div( "optionsTabDiv" );
  m_tabs->addTab( optionsTabDiv, "Options" );*/

  Div *errorGridTabDiv = new Div( "errorGridTabDiv" );
  WBorderLayout *errorGridTabLayout = new WBorderLayout();
  errorGridTabLayout->setSpacing( 0 );
  errorGridTabLayout->setContentsMargins( 0, 0, 0, 0 );
  errorGridTabDiv->setLayout( errorGridTabLayout );
  errorGridTabLayout->addWidget( m_errorGridLegend, WBorderLayout::West );
  errorGridTabLayout->addWidget( m_errorGridGraph, WBorderLayout::Center );
  m_errorGridGraph->setMinimumSize( 400, 400 );
  m_tabs->addTab( errorGridTabDiv, "Error Grid" );

  syncDisplayToModel();

  DubEventEntry *dataEntry = new DubEventEntry();
  layout->addWidget( dataEntry, WBorderLayout::South );
  dataEntry->entered().connect( boost::bind(&WtGui::addData, this, _1) );

}//WtGui::init()


void WtGui::saveModelAsDialog()
{
  WDialog *dialog = new WDialog( "Save As" );
  //dialog->setModal( false );

  new WText("Enter name to save as: ", dialog->contents());
  WLineEdit *edit = new WLineEdit( dialog->contents() );
  edit->setText( m_userDbPtr->currentFileName );
  edit->setValidator( new WLengthValidator( 1, 45 ) );
  new Wt::WBreak( dialog->contents() );
  WPushButton *ok = new WPushButton( "Ok", dialog->contents() );
  WPushButton *cancel = new WPushButton( "Cancel", dialog->contents() );

  edit->enterPressed().connect( dialog, &WDialog::accept );
  ok->clicked().connect( dialog, &WDialog::accept );
  cancel->clicked().connect( dialog, &WDialog::reject );
  edit->escapePressed().connect( dialog, &WDialog::reject );
  edit->setFocus();
  dialog->exec();

  const string newFileName = edit->text().narrow();
  if( dialog->result() == WDialog::Accepted ) saveModel( newFileName );

  //delete dialog;  //crashes for some reason even when non modal
}//void saveModelAsDialog()



void WtGui::updateDataRange()
{
  if( !m_model ) return;

  PosixTime ptimeStart = boost::posix_time::second_clock::local_time();
  PosixTime ptimeEnd   = boost::posix_time::second_clock::local_time();
  if( !m_model->m_cgmsData.empty() ) ptimeStart = m_model->m_cgmsData.getStartTime();
  if( !m_model->m_cgmsData.empty() ) ptimeEnd = m_model->m_cgmsData.getEndTime();

  if( !m_model->m_fingerMeterData.empty() )
    ptimeStart = min( ptimeStart, m_model->m_fingerMeterData.getStartTime() );
  if( !m_model->m_fingerMeterData.empty() )
    ptimeEnd = max( ptimeEnd, m_model->m_fingerMeterData.getEndTime() );
  if( !m_model->m_mealData.empty() )
    ptimeStart = min( ptimeStart, m_model->m_mealData.getStartTime() );
  if( !m_model->m_mealData.empty() )
    ptimeEnd = max( ptimeEnd, m_model->m_mealData.getEndTime() );
  if( !m_model->m_predictedBloodGlucose.empty() )
    ptimeStart = min( ptimeStart, m_model->m_predictedBloodGlucose.getStartTime() );
  if( !m_model->m_predictedBloodGlucose.empty() )
    ptimeEnd = max( ptimeEnd, m_model->m_predictedBloodGlucose.getEndTime() );
  if( !m_model->m_customEvents.empty() )
    ptimeStart = min( ptimeStart, m_model->m_customEvents.getStartTime() );
  if( !m_model->m_customEvents.empty() )
    ptimeEnd = max( ptimeEnd, m_model->m_customEvents.getEndTime() );
  if( !m_model->m_predictedInsulinX.empty() )
    ptimeStart = min( ptimeStart, m_model->m_predictedInsulinX.getStartTime() );
  if( !m_model->m_predictedInsulinX.empty() )
    ptimeEnd = max( ptimeEnd, m_model->m_predictedInsulinX.getEndTime() );

  WDateTime start, end;
  end.setPosixTime( ptimeEnd );
  start.setPosixTime( ptimeStart );

  const bool isTopped = (m_bsEndTimePicker->dateTime() == m_bsEndTimePicker->bottom());
  const bool isBottomed = (m_bsBeginTimePicker->dateTime() == m_bsBeginTimePicker->bottom());

  m_bsBeginTimePicker->setBottom( start );
  m_bsBeginTimePicker->setTop( end );
  m_bsEndTimePicker->setBottom( start );
  m_bsEndTimePicker->setTop( end );

  if( isTopped ) m_bsEndTimePicker->set( end );
  if( isBottomed ) m_bsBeginTimePicker->set( start );
}//void WtGui::updateDataRange()


void WtGui::updateDisplayedDateRange()
{
  const WDateTime end = m_bsEndTimePicker->dateTime();
  const WDateTime start = m_bsBeginTimePicker->dateTime();
  m_bsGraph->axis(Chart::XAxis).setRange( Wt::asNumber(start), Wt::asNumber(end) );
}//void updateDisplayedDateRange()


void WtGui::addData( WtGui::EventInformation info )
{
  switch( info.type )
  {
    case WtGui::kNotSelected: break;
    case WtGui::kCgmsReading:
      m_model->addCgmsData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kMeterReading:
      m_model->addFingerStickData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kMeterCalibration:
      m_model->addFingerStickData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kGlucoseEaten:
      m_model->addConsumedGlucose( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kBolusTaken:
      m_model->addBolusData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kGenericEvent:
      //m_model->addCustomEvent( const PosixTime &time, int eventType );
    break;
    case WtGui::kNumEntryType: break;
  };//switch( et )

  syncDisplayToModel(); //taking the lazy way out and just reloading
                        //all data to the model
}//void addData( EventInformation info );


void WtGui::syncDisplayToModel()
{
  typedef ConsentrationGraph::value_type cg_type;

  const int nNeededRow = m_model->m_cgmsData.size()
                         + m_model->m_fingerMeterData.size()
                         + m_model->m_mealData.size()
                         + m_model->m_predictedBloodGlucose.size()
                         + m_model->m_glucoseAbsorbtionRate.size()
                         + m_model->m_customEvents.size()
                         + m_model->m_predictedInsulinX.size();

  m_bsModel->removeRows( 0, m_bsModel->rowCount() );
  m_bsModel->insertRows( 0, nNeededRow );

  int row = 0;
  foreach( const cg_type &element, m_model->m_cgmsData )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kCgmsData, element.m_value );
  }//

  foreach( const cg_type &element, m_model->m_fingerMeterData )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kFingerStickData, element.m_value );
  }//

  foreach( const cg_type &element, m_model->m_mealData )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kMealData, element.m_value );
  }//

  foreach( const cg_type &element, m_model->m_predictedBloodGlucose )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kPredictedBloodGlucose, element.m_value );
  }//

  foreach( const cg_type &element, m_model->m_glucoseAbsorbtionRate )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kGlucoseAbsRate, element.m_value );
  }//

  foreach( const cg_type &element, m_model->m_customEvents )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kCustomEventData, element.m_value );
  }//

  foreach( const cg_type &element, m_model->m_predictedInsulinX )
  {
    WDateTime x;
    x.setPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kPredictedInsulinX, element.m_value );
  }//

}//void syncDisplayToModel()





void WtGui::updateClarkAnalysis( const ConsentrationGraph &xGraph,
                                 const ConsentrationGraph &yGraph,
                                 bool isCgmsVMeter )
{
  typedef ConsentrationGraph::value_type cg_type;

  m_errorGridModel->removeRows( 0, m_errorGridModel->rowCount() );
  m_errorGridModel->insertRows( 0, xGraph.size() );

  TimeDuration cmgsDelay(0,0,0,0);

  string delayStr = "", uncertStr = "";
  if( isCgmsVMeter )
  {
    cmgsDelay = m_model->findCgmsDelayFromFingerStick();
    double sigma = 1000.0 * m_model->findCgmsErrorFromFingerStick(cmgsDelay);
    sigma = static_cast<int>(sigma + 0.5) / 10.0; //nearest tenth of a percent

    delayStr = "Delay=";
    delayStr += boost::posix_time::to_simple_string(cmgsDelay).substr(3,5);
    delayStr += "   ";
    ostringstream uncertDescript;
    uncertDescript << "&sigma;<sub>cgms</sub><sup>finger</sup>=" << sigma << "%";
    uncertStr = uncertDescript.str();
  }//if( isCgmsVMeter )

  vector<int> regionTotals(NumErrorRegions, 0);
  int row = 0;
  foreach( const cg_type &el, xGraph )
  {
    const PosixTime &time = el.m_time;
    const double meterValue = el.m_value;
    const double cgmsValue = yGraph.value(time + cmgsDelay);

    ErrorRegions region = NumErrorRegions;

    if( (cgmsValue <= 70.0 && meterValue <= 70.0) || (cgmsValue <= 1.2*meterValue && cgmsValue >= 0.8*meterValue) )
    {
       region = kRegionA;
    }else
    {
      if( ((meterValue >= 180.0) && (cgmsValue <= 70.0)) || ((meterValue <= 70.0) && cgmsValue >= 180.0 ) )
      {
        region = kRegionE;
      }else
      {
        if( ((meterValue >= 70.0 && meterValue <= 290.0) && (cgmsValue >= meterValue + 110.0) ) || ((meterValue >= 130.0 && meterValue <= 180.0)&& (cgmsValue <= (7.0/5.0)*meterValue - 182.0)) )
        {
           region = kRegionC;
        }else
        {
          if( ((meterValue >= 240.0) && ((cgmsValue >= 70.0) && (cgmsValue <= 180.0))) || (meterValue <= 175.0/3.0 && (cgmsValue <= 180.0) && (cgmsValue >= 70.0)) || ((meterValue >= 175.0/3.0 && meterValue <= 70.0) && (cgmsValue >= (6.0/5.0)*meterValue)) )
          {
            region = kRegionD;
          }else
          {
             region = kRegionB;
          }//if(zone D) else(zoneB)
        }//if(zone C) / else
      }//if(zone E) / else
   }//if(zone A) / else

   assert(region != NumErrorRegions) ;

   if( cgmsValue > 10.0 )
   {
     ++(regionTotals[region]);
     m_errorGridModel->setData( row, 0, meterValue );
     m_errorGridModel->setData( row++, region, cgmsValue );
   }//if( all is a okay to display this point )

 }//foreach( const GraphElement &el, meterGraph )

  double nTotal = 0.0;
  foreach( const int &i, regionTotals ) nTotal += i;

  for( ErrorRegions reg(kRegionA); reg < NumErrorRegions; reg = ErrorRegions(reg+1) )
  {
    ostringstream percentA;
    percentA << "Region " << char(int('A')+reg-kRegionA) << " " << setw(4)
             << setiosflags(ios::fixed | ios::right)
             << setprecision(1) << 100.0 * regionTotals[reg] / nTotal << "%";
    m_errorGridModel->setHeaderData( reg, WString(percentA.str()) );
  }//for( loop over regions to make theree legend titles)


  if( isCgmsVMeter )
  {
    m_errorGridGraph->axis(Chart::XAxis).setTitle( "Finger-Prick Value(mg/dl)" );
    m_errorGridGraph->axis(Chart::YAxis).setTitle(  "CGMS Value(mg/dl)" );
  }else
  {
    m_errorGridGraph->axis(Chart::XAxis).setTitle( "CGMS Value(mg/dl)" );
    m_errorGridGraph->axis(Chart::YAxis).setTitle( "Predicted Value(mg/dl)" );
  }//if( !isCgmsVsMeter )

  m_errorGridLegend->clear();

  new WText( delayStr, XHTMLUnsafeText, m_errorGridLegend );
  new WBreak( m_errorGridLegend );
  new WText( uncertStr, XHTMLUnsafeText, m_errorGridLegend );
  new WBreak( m_errorGridLegend );
  new WBreak( m_errorGridLegend );

  for( ErrorRegions reg(kRegionA); reg < NumErrorRegions; reg = ErrorRegions(reg+1) )
  {
    WWidget *item = m_errorGridGraph->createLegendItemWidget(reg);
    m_errorGridLegend->addWidget( item );
  }//for( loop over and put the regions into their area )
}// void WtGui::updateClarkAnalysis()







std::string WtGui::formFileSystemName( const std::string &internalName )
{
  return  "../data/" + m_userDbPtr->name + "_" + internalName + ".dubm";
}//string formFileSystemName( const string &internalName )


void WtGui::saveModelConfirmation()
{
  if( m_userDbPtr->currentFileName == "" ) return;

  WDialog dialog( "Confirm" );
  new WText("Would you like to save the current model first?", dialog.contents() );
  new WBreak( dialog.contents() );

  WPushButton *ok = new WPushButton( "Ok", dialog.contents() );
  WPushButton *cancel = new WPushButton( "Cancel", dialog.contents() );
  ok->enterPressed().connect( &dialog, &WDialog::accept );
  cancel->enterPressed().connect( &dialog, &WDialog::reject );
  ok->clicked().connect( &dialog, &WDialog::accept );
  cancel->clicked().connect( &dialog, &WDialog::reject );

  const WDialog::DialogCode code = dialog.exec();
  if( code == WDialog::Accepted ) saveModel( m_userDbPtr->currentFileName );
}//void saveModelConfirmation()


void WtGui::newModel()
{
  saveModelConfirmation();
  delete m_model;
  setModelFileName( "" );
  m_model = new NLSimple( m_userDbPtr->name + " Model", 1.0 );
  //saveModel( "" );
  init( m_userDbPtr->name );
}//void newModel()


void WtGui::saveModel( const std::string &fileName )
{
  if( fileName.empty() )
  {
    saveModelAsDialog();
    return;
  }//if( fileName.empty() )

  m_model->saveToFile( formFileSystemName(fileName) );

  Dbo::Transaction transaction(m_dbSession);
  const DubUser::UsersModels &models = m_userDbPtr->models;
  Dbo::ptr<UsersModel> model = models.find().where( "fileName = ?" ).bind( fileName );
  if( !model )
  {
    UsersModel *newModel = new UsersModel();
    newModel->user     = m_userDbPtr;
    newModel->fileName = fileName;
    newModel->created  = WDateTime::fromPosixTime( boost::posix_time::second_clock::local_time() );
    newModel->modified = newModel->created;
    m_dbSession.add( newModel );
  }//if( !model )

  transaction.commit();

  setModelFileName( fileName );
}//void saveModel( const std::string &fileName )


void WtGui::setModel( const std::string &fileName )
{
  try{
    NLSimple *original = m_model;
    m_model = new NLSimple( formFileSystemName(fileName) );
    setModelFileName( fileName );
    if( original ) delete original;
  }catch(...){
    cerr << "Failed to open NLSimple named " << fileName << endl;
    wApp->doJavaScript( "Failed to open NLSimple named " + fileName, true );
  }//try/catch
}//void setModel( const std::string &fileName )


void WtGui::setModelFileName( const std::string &fileName )
{
  Dbo::Transaction transaction(m_dbSession);
  m_userDbPtr.modify()->currentFileName = fileName;
  transaction.commit();
}//void setModelFileName( const std::string &fileName )


Div::Div( const std::string &styleClass, Wt::WContainerWidget *parent )
  : Wt::WContainerWidget( parent )
{
  setInline( false );
  if( styleClass != "" ) WContainerWidget::setStyleClass( styleClass );
}//Div Constructor



DubsLogin::DubsLogin(  Wt::Dbo::Session &dbSession, WContainerWidget *parent ):
   WContainerWidget( parent ),
   m_dbSession( dbSession ),
   m_introText( NULL ),
   m_username( NULL ),
   m_password( NULL )
{
   setStyleClass( "login_box" );

   //if( WServer::httpPort() != .... ) then don't serve the page, redirect to a static error page

   WText *title = new WText("Login", this);
   title->decorationStyle().font().setSize(WFont::XLarge);

   m_introText = new WText("<p>Welcome to Diabetes Understanding by Simulation</p>", this);

   WTable *layout = new WTable(this);
   WLabel *usernameLabel = new WLabel("User name: ", layout->elementAt(0, 0));
   layout->elementAt(0, 0)->resize(WLength(14, WLength::FontEx), WLength::Auto);
   m_username = new WLineEdit( layout->elementAt(0, 1) );
   usernameLabel->setBuddy(m_username);

   WLabel *passwordLabel = new WLabel("Password: ", layout->elementAt(1, 0));
   m_password = new WLineEdit(layout->elementAt(1, 1));
   m_password->setEchoMode(WLineEdit::Password);
   passwordLabel->setBuddy(m_password);

   new WBreak(this);

   WPushButton *loginButton = new WPushButton("Login", this);
   WPushButton *addUserButton = new WPushButton("New User", this);
   m_username->setFocus();

   WApplication::instance()->globalEnterPressed().connect( SLOT( this, DubsLogin::checkCredentials) );
   m_username->enterPressed().connect( SLOT( this, DubsLogin::checkCredentials) );
   m_password->enterPressed().connect( SLOT( this, DubsLogin::checkCredentials) );
   loginButton->clicked().connect(SLOT( this, DubsLogin::checkCredentials) );
   addUserButton->clicked().connect( this, &DubsLogin::addUser );
}//DubsLogin( initializer )



std::string DubsLogin::isLoggedIn( Wt::Dbo::Session &dbSession )
{
  string value = "";
  try{ value = wApp->environment().getCookie( cookie_name ); }
  catch(...) { return ""; }

  const size_t pos = value.find( '_' );
  if( pos == string::npos ) return "";

  const string name = value.substr( 0, pos );
  const string hash = value.substr( pos+1, string::npos );

  string found_hash = "";

  Dbo::Transaction transaction(dbSession);
  Dbo::ptr<DubUser> user = dbSession.find<DubUser>().where("name = ?").bind( name );
  if( user ) found_hash = user->cookieHash;
  transaction.commit();

  if( (found_hash==hash) && (hash!="") ) return name;
  return "";
}//bool DubsLogin::checkForCookie()


void DubsLogin::insertCookie( const std::string &uname,
                              const int lifetime,
                              Wt::Dbo::Session &dbSession )
{
  static TRandom3 tr3(0);
  const string random_str = boost::lexical_cast<string>( tr3.Rndm() );
  TMD5 md5obj;
  md5obj.Update( (UChar_t *)random_str.c_str(), random_str.length() );
  md5obj.Final();
  string hash = md5obj.AsString();

  const string cookie_value = (lifetime > 0) ? (uname + "_" + hash) : string("");

  Dbo::Transaction transaction(dbSession);
  Dbo::ptr<DubUser> user = dbSession.find<DubUser>().where("name = ?").bind(uname);

  if( !user )
  {
    wApp->doJavaScript( "alert( \"Couldn\'t find user" + uname + " in database to set cookie for\" )", true );
  }else
  {
    user.modify()->cookieHash = hash;
    wApp->setCookie( cookie_name, cookie_value, lifetime ); //keep cookie for 25 days
  }//if( found the user ) / else
  transaction.commit();
}//void DubsLogin::insertCookie()



void DubsLogin::addUser()
{
  WDialog *dialog = new WDialog( "Add User" );
  //dialog->setModal( false );
  new WText( "User Name: ", dialog->contents() );
  WLineEdit *userNameEdit = new WLineEdit( dialog->contents() );
  userNameEdit->setText( "" );
  WLengthValidator *val = new WLengthValidator( 3, 50 );
  userNameEdit->setValidator( val );
  new WText( " Password: ", dialog->contents() );
  WLineEdit *passwordEdit = new WLineEdit( dialog->contents() );
  passwordEdit->setText( "" );
  val = new WLengthValidator( 3, 50 );
  passwordEdit->setValidator( val );

  new Wt::WBreak( dialog->contents() );
  WPushButton *ok = new WPushButton( "Ok", dialog->contents() );
  WPushButton *cancel = new WPushButton( "Cancel", dialog->contents() );

  userNameEdit->enterPressed().connect( dialog, &WDialog::accept );
  passwordEdit->enterPressed().connect( dialog, &WDialog::accept );
  ok->clicked().connect( dialog, &WDialog::accept );
  cancel->clicked().connect( dialog, &WDialog::reject );
  userNameEdit->escapePressed().connect( dialog, &WDialog::reject );
  passwordEdit->escapePressed().connect( dialog, &WDialog::reject );
  userNameEdit->setFocus();
  dialog->exec();

  if( dialog->result() == WDialog::Rejected )
  {
    //delete dialog;
    return;
  }//if user canceled

  const string name = userNameEdit->text().narrow();
  const string pword = passwordEdit->text().narrow();

  Dbo::ptr<DubUser> user;
  {
    Dbo::Transaction transaction(m_dbSession);
    user = m_dbSession.find<DubUser>().where("name = ?").bind( name );
    transaction.commit();
  }

  if( user || name.empty() || pword.empty() )
  {
    wApp->doJavaScript( "alert( \"User '" + name + "'' exists or invalid\" )", true );
    //delete dialog;
    //dialog = NULL;
    addUser();
  }else
  {
    TMD5 md5obj;
    md5obj.Update( (UChar_t *)pword.c_str(), pword.length() );
    md5obj.Final();
    const string pwordhash = md5obj.AsString();

    Dbo::Transaction transaction(m_dbSession);
    DubUser *user = new DubUser();
    user->name = name;
    user->password = pwordhash;
    user->currentFileName = "example";//ProgramOptions::ns_defaultModelFileName;
    const string example_file_name = "../data/" + name + "_example.dubm";
    gSystem->CopyFile( "../data/example.dubm", example_file_name.c_str(), kFALSE );
    user->cookieHash = "";
    using boost::algorithm::ends_with;
    if( ends_with( name, "guest" ) ) user->role = DubUser::Visitor;
    else                             user->role = DubUser::FullUser;

    Dbo::ptr<DubUser> userptr = m_dbSession.add( user );
    UsersModel *defaultModel  = new UsersModel();
    defaultModel->user        = userptr;
    defaultModel->fileName    = user->currentFileName;
    defaultModel->created     = WDateTime::fromPosixTime( kGenericT0 );
    defaultModel->modified    = WDateTime::fromPosixTime( kGenericT0 );
    Dbo::ptr<UsersModel> usermodelptr = m_dbSession.add( defaultModel );
    const bool wasCommitted = transaction.commit();

    if( !wasCommitted ) cerr << "\nDid not commit adding the user\n" << endl;
  }//if( user exists ) / else

  //delete dialog;
  m_username->setText( name );
}//void addUser()


void DubsLogin::checkCredentials()
{
   const std::string user = m_username->text().narrow();
   const std::string pass = m_password->text().narrow();

   if( validLogin( user, pass ) )
   {
     insertCookie( user, 25*24*3600, m_dbSession );
     m_loginSuccessfulSignal.emit( user );
   } else
   {
      m_introText->setText("<p>You entered an invalid password or username."
                           "</p><p>Please try again.</p>" );
      m_introText->decorationStyle().setForegroundColor( Wt::WColor( 186, 17, 17 ) );
      m_username->setText("");
      m_password->setText("");
      WWidget *p = m_introText->parent();
      if( p ) p->decorationStyle().setBackgroundColor( Wt::WColor( 242, 206, 206 ) );
   }//if( username / password ) / else failure
}//void DubsLogin::checkCredentials()



bool DubsLogin::validLogin( const std::string &user, std::string pass )
{
  TMD5 md5obj;
  md5obj.Update( (UChar_t *)pass.c_str(), pass.length() );
  md5obj.Final();
  pass = md5obj.AsString();

  Dbo::Transaction transaction(m_dbSession);
  Dbo::ptr<DubUser> userptr = m_dbSession.find<DubUser>().where("name = ?").bind( user );
  transaction.commit();

  return (userptr && (userptr->password == pass));
}//bool validLogin( const std::string &user, std::string &pass );


Wt::Signal<std::string> &DubsLogin::loginSuccessful()
{
  return m_loginSuccessfulSignal;
}//loginSuccessful()


DateTimeSelect::DateTimeSelect( const std::string &labelText,
                                const Wt::WDateTime &initialTime,
                                Wt::WContainerWidget *parent )
  : WContainerWidget(parent),
  m_datePicker( new WDatePicker() ),
  m_hourSelect( new WSpinBox() ),
  m_minuteSelect( new WSpinBox() ),
  m_top( WDate(3000,1,1), WTime(0,0,0) ),
  m_bottom( WDate(1901,1,1), WTime(0,0,0) )
{
  setInline(true);
  setStyleClass( "DateTimeSelect" );

  m_hourSelect->setSingleStep( 1.0 );
  m_hourSelect->setMinimum(0);
  m_hourSelect->setMaximum(23);
  m_hourSelect->setMaxLength(2);
  m_hourSelect->setTextSize(2);

  m_minuteSelect->setSingleStep( 1.0 );
  m_minuteSelect->setMinimum(0);
  m_minuteSelect->setMaximum(59);
  m_minuteSelect->setMaxLength(2);
  m_minuteSelect->setTextSize(2);

  m_datePicker->setGlobalPopup( true );
  m_datePicker->changed().connect(  boost::bind( &WDatePicker::setPopupVisible, m_datePicker, false ) );
  m_datePicker->changed().connect(   boost::bind( &DateTimeSelect::validate, this, true ) );
  m_hourSelect->changed().connect(   boost::bind( &DateTimeSelect::validate, this, true ) );
  m_minuteSelect->changed().connect( boost::bind( &DateTimeSelect::validate, this, true ) );

  if( labelText != "" )
  {
    WLabel *label = new WLabel( labelText, this );
    label->setBuddy( m_datePicker->lineEdit() );
  }//if( labelText != "" )

  addWidget( m_datePicker );
  addWidget( new WText("&nbsp;&nbsp;") );
  addWidget( m_hourSelect );
  addWidget( new WText(":") );
  addWidget( m_minuteSelect );

  set( initialTime );
}//DateTimeSelect constructor

DateTimeSelect::~DateTimeSelect(){}

void DateTimeSelect::set( const Wt::WDateTime &dt )
{
  if( !dt.isValid() ) return;
  if( (dt > m_top) || (dt < m_bottom) ) return;
  m_datePicker->setDate( dt.date() );

  const int hour = dt.time().hour();
  const int minute = dt.time().minute();
  m_hourSelect->setText( boost::lexical_cast<string>(hour) );
  m_minuteSelect->setText( boost::lexical_cast<string>(minute) );
}//void set( const Wt::WDateTime *dateTime )


Wt::WDateTime DateTimeSelect::dateTime() const
{
  const int hour = floor( m_hourSelect->value() + 0.5 );
  const int minute = floor( m_minuteSelect->value() + 0.5 );
  return WDateTime( m_datePicker->date(), WTime( hour, minute ) );
}//Wt::WDateTime DateTimeSelect::dateTime() const

void DateTimeSelect::setTop( const Wt::WDateTime &top )
{
  m_top = top;
  validate();
}//setTop( const Wt::WDateTime &top )

void DateTimeSelect::setBottom( const Wt::WDateTime &bottom )
{
  m_bottom = bottom;
  validate();
}//setBottom( const Wt::WDateTime &bottom )

void DateTimeSelect::validate( bool emitChanged )
{
  const WDateTime currentDT = dateTime();
  if( currentDT < m_bottom )   set( m_bottom );
  else if( currentDT > m_top ) set( m_top );

  if( emitChanged ) m_changed.emit();
}//void notify()

Wt::Signal<> &DateTimeSelect::changed()
{
  return m_changed;
}//Wt::Signal<> changed()




const Wt::WDateTime &DateTimeSelect::top() const
{
  return m_top;
}

const Wt::WDateTime &DateTimeSelect::bottom() const
{
  return m_bottom;
}


DubEventEntry::DubEventEntry( WContainerWidget *parent )
  : WContainerWidget( parent ),
  m_time( new DateTimeSelect( "Date/Time&nbsp;",
                              WDateTime::fromPosixTime(boost::posix_time::second_clock::local_time())
                            ) ),
  m_type( new WComboBox() ),
  m_value( new WLineEdit() ),
  m_units( new WText() ),
  m_button( new WPushButton("submit") )
{
  setStyleClass( "DubEventEntry" );
  setInline(false);

  addWidget( new WText( "<b>Enter New Event</b>&nbsp;:&nbsp;" ) );
  addWidget( m_time );
  addWidget( m_type );
  addWidget( m_value );
  addWidget( m_units );
  addWidget( m_button );

  for( WtGui::EntryType et = WtGui::EntryType(0);
       et < WtGui::kNumEntryType;
       et = WtGui::EntryType( et+1 ) )
  {
    switch( et )
    {
      case WtGui::kNotSelected:      m_type->addItem( "Event Type" );  break;
      case WtGui::kCgmsReading:      m_type->addItem( "CGMS Reading" ); break;
      case WtGui::kMeterReading:     m_type->addItem( "Fingerstick" );  break;
      case WtGui::kMeterCalibration: m_type->addItem( "CGMS Cal." );    break;
      case WtGui::kGlucoseEaten:     m_type->addItem( "Carbs" );        break;
      case WtGui::kBolusTaken:       m_type->addItem( "Insulin" );      break;
      case WtGui::kGenericEvent:     m_type->addItem( "Generic" );      break;
      case WtGui::kNumEntryType:     /*m_type->addItem( "" );*/             break;
    };//switch( et )
  };//enum EntryType
  m_type->setCurrentIndex( WtGui::kNotSelected );

  m_value->setTextSize(4);
  m_value->enterPressed().connect( boost::bind( &DubEventEntry::emitEntered, this ) );
  m_type->changed().connect( boost::bind( &DubEventEntry::typeChanged, this ) );
  m_button->clicked().connect( boost::bind( &DubEventEntry::emitEntered, this ) );

  typeChanged();
}//DubEventEntry(...)

DubEventEntry::~DubEventEntry(){}

void DubEventEntry::emitEntered()
{
  if( m_type->currentIndex() == WtGui::kNotSelected
      || m_type->currentIndex() == WtGui::kCustomEventData ) return;

  WtGui::EventInformation info;
  info.dateTime = m_time->dateTime();
  info.type = WtGui::EntryType( m_type->currentIndex() );

  try
  {
    info.value = boost::lexical_cast<double>( m_value->text().narrow() );
  }catch(...)
  {
    cerr << "Warning failed lexical_cast<double> in emitEntered" << endl;
    return;
  }//try/catch

  reset();

  m_signal.emit(info);
}//void emitEntered()

void DubEventEntry::reset()
{
  using namespace boost::posix_time;
  m_time->set( WDateTime::fromPosixTime(second_clock::local_time()) );
  m_type->setCurrentIndex( WtGui::kNotSelected );
  m_units->setText("");
  m_value->setText( "" );
  m_value->setEnabled(false);
  m_button->setEnabled(false);
}//void DubEventEntry::reset()

void DubEventEntry::typeChanged()
{
  m_value->setEnabled(true);
  m_button->setEnabled(true);

  switch( m_type->currentIndex() )
  {
    case WtGui::kNotSelected:
      m_units->setText("");
      m_value->setText( "" );
      m_value->setEnabled(false);
      m_button->setEnabled(false);
    break;
    case WtGui::kCgmsReading:
      m_units->setText("mg/dL");
      m_value->setValidator( new WIntValidator(0,500) );
    break;
    case WtGui::kMeterReading:
      m_units->setText("mg/dL");
      m_value->setValidator( new WIntValidator(0,500) );
    break;
    case WtGui::kMeterCalibration:
      m_units->setText("mg/dL");
      m_value->setValidator( new WIntValidator(0,500) );
    break;
    case WtGui::kGlucoseEaten:
      m_units->setText("grams");
      m_value->setValidator( new WIntValidator(0,150) );
    break;
    case WtGui::kBolusTaken:
      m_units->setText("units");
      m_value->setValidator( new WDoubleValidator(0,50) );
    break;
    case WtGui::kGenericEvent:
      m_units->setText("");
      m_value->setText( "NA" );
      m_value->setEnabled(false);
      m_button->setEnabled(false);
    break;
    case WtGui::kNumEntryType:
      m_type->addItem( "" );
    break;
  };//switch( index[0] )

}//void typeChanged()

Wt::Signal<WtGui::EventInformation> &DubEventEntry::entered()
{
  return m_signal;
}


ClarkErrorGridGraph::ClarkErrorGridGraph( Wt::Chart::ChartType type,
                                          Wt::WContainerWidget *parent ) :
      Wt::Chart::WCartesianChart( type, parent )
{
}//ClarkErrorGridGraph constructor

WLineF ClarkErrorGridGraph::line( const double &x1, const double &y1,
                                     const double &x2, const double &y2 ) const
{
  return WLineF( mapToDevice(x1,y1), mapToDevice(x2,y2) );
}//WLineF ClarkErrorGridGraph::line()

WRectF ClarkErrorGridGraph::rect( const double &x1, const double &y1 ) const
{
  const WPointF ul = mapToDevice(x1,y1);
  return WRectF( ul.x(), ul.y(), 15.0, 15.0 );
}//WLineF ClarkErrorGridGraph::line()


void ClarkErrorGridGraph::paint( WPainter &painter,
                                 const WRectF &rectangle ) const
{
  WCartesianChart::paint( painter, rectangle );
  const WPen origPen = painter.pen();

  WPen pen( DotLine/*DashLine*/ );

  painter.setPen( pen );
  painter.drawLine( line( 0.0, 0.0, 400.0, 400.0 ) );

  pen.setStyle( SolidLine );
  painter.setPen( pen );
  painter.drawLine( line( 0.0,       70.0,      175.0/3.0, 70.0 ) );
  painter.drawLine( line( 175.0/3.0, 70.0,      320,       400 ) );
  painter.drawLine( line( 70,        84,        70,        400) );
  painter.drawLine( line( 0,         180,       70,        180) );
  painter.drawLine( line( 70,        180,       290,       400) );
  painter.drawLine( line( 70,        0,         70,        175.0/3.0) );
  painter.drawLine( line( 70,        175.0/3.0, 400,       320) );
  painter.drawLine( line( 180,       0,         180,       70) );
  painter.drawLine( line( 180,       70,        400,       70) );
  painter.drawLine( line( 240,       70,        240,       180) );
  painter.drawLine( line( 240,       180,       400,       180) );
  painter.drawLine( line( 130,       0,         180,       70) );

  WFont font;
  font.setSize( WFont::XLarge );
  font.setWeight( WFont::Bolder );
  painter.setFont( font );
  pen.setColor( palette()->strokePen(WtGui::kRegionA-1).color() );
  painter.setPen( pen );
  painter.drawText( rect(30,  30), AlignLeft, "A");
  pen.setColor( palette()->strokePen(WtGui::kRegionB-1).color() );
  painter.setPen( pen );
  painter.drawText( rect(380, 220), AlignLeft,  "B");
  painter.drawText( rect(240, 340), AlignLeft,  "B");
  pen.setColor( palette()->strokePen(WtGui::kRegionC-1).color() );
  painter.setPen( pen );
  painter.drawText( rect(150, 380), AlignLeft,  "C");
  painter.drawText( rect(160, 30),  AlignLeft,  "C");
  pen.setColor( palette()->strokePen(WtGui::kRegionD-1).color() );
  painter.setPen( pen );
  painter.drawText( rect(30,  150), AlignLeft,  "D");
  painter.drawText( rect(380, 120), AlignLeft,  "D");
  pen.setColor( palette()->strokePen(WtGui::kRegionE-1).color() );
  painter.setPen( pen );
  painter.drawText( rect(380, 30),  AlignLeft,  "E");
  painter.drawText( rect(30,  380), AlignLeft,  "E");

  painter.setPen( origPen );
}//void paintEvent(Wt::WPaintDevice *paintDevice)




