#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

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
#include <Wt/WHBoxLayout>
#include <Wt/Chart/WChartPalette>

#include "TH1.h"
#include "TH1F.h"
#include "TH2.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TRandom3.h"
#include "TSystem.h"

#include "WtGui.hh"
#include "WtUtils.hh"
#include "WtUserManagment.hh"
#include "WtCreateNLSimple.hh"
#include "ResponseModel.hh"
#include "ProgramOptions.hh"

using namespace Wt;
using namespace std;


#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH



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
  WGridLayout *layout = new WGridLayout();
  root()->setLayout( layout );
  DubsLogin *login = new DubsLogin( m_dbSession );
  layout->addWidget( login, 0, 0, 1, 1, AlignCenter );
  layout->setColumnStretch(0, 5);
  layout->addWidget( new Div(), 1, 0, 1, 1, AlignCenter );
  layout->setRowStretch( 1, 5 );

  login->loginSuccessful().connect( boost::bind( &WtGui::init, this, _1 ) );
  login->loginSuccessful().connect( boost::bind( &DubsLogin::insertCookie, _1, 3024000, boost::ref(m_dbSession) ) );
}//void WtGui::requireLogin()


void WtGui::logout()
{  
  delete m_model;
  m_model = NULL;
  DubsLogin::insertCookie( m_userDbPtr->name, 0, m_dbSession ); //deltes the cookie
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
  {
    Dbo::Transaction transaction(m_dbSession);
    Dbo::ptr<UsersModel> model = m_userDbPtr.modify()->models.find().where("fileName = ?").bind( modelname );
    if( model ) model.remove();
    transaction.commit();
  }

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



  size_t nModels = 0;
  if( m_userDbPtr )
  {
    Dbo::Transaction transaction(m_dbSession);
    nModels = m_userDbPtr->models.size();
    transaction.commit();
  }//if( m_userDbPtr )

  if( nModels == 0 )
  {
    newModel();
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
      return;
    }//if( selected.empty() )

    try
    {
      const int selectedRow = selected.begin()->row();
      setModelFileName( boost::any_cast<string>( viewmodel->data( selectedRow, 0 ) ) );
      resetGui();
    }catch(...)
    { cerr << endl << "Any Cast Failed in openModelDialog()" << endl; }

    return;
  }//if( code == WDialog::Accepted )

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
    setModelFileName( "" );
    init( m_userDbPtr->name );
    return;
  }

  m_upperEqnDiv = new Div( "m_upperEqnDiv" );
  m_actionMenuPopup = new WPopupMenu();
  WPushButton *actionMenuButton = new WPushButton( "Actions", m_upperEqnDiv );
  actionMenuButton->setStyleClass( "actionMenuButton" );
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
  item->triggered().connect( boost::bind( &WPopupMenu::setHidden, m_actionMenuPopup, true ) );

  layout->addWidget( m_upperEqnDiv, WBorderLayout::North );

  WPushButton *logout = new WPushButton( "logout", m_upperEqnDiv );
  logout->setStyleClass( "logoutButton" );
  logout->clicked().connect( boost::bind( &WtGui::logout, this) );


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
  m_bsGraph->axis(Chart::Y2Axis).setTitle( "Consumed Carbs" );
  const WPen &y2Pen = m_bsGraph->palette()->strokePen(kMealData-2);
  m_bsGraph->axis(Chart::Y2Axis).setPen( y2Pen );
  m_bsGraph->axis(Chart::YAxis).setTitle( "mg/dL" );


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
  WPushButton *zoomOut = new WPushButton( "Full Date Range", datePickingDiv );
  zoomOut->clicked().connect( this, &WtGui::zoomToFullDateRange );
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

  DubEventEntry *dataEntry = new DubEventEntry( this );
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


void WtGui::zoomToFullDateRange()
{
  m_bsEndTimePicker->set( m_bsEndTimePicker->top() );
  m_bsBeginTimePicker->set( m_bsBeginTimePicker->top() );
  updateDisplayedDateRange();
}//void zoomToFullDateRange();


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

  updateDisplayedDateRange();
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

  updateDataRange();
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

  WPushButton *ok = new WPushButton( "Yes", dialog.contents() );
  WPushButton *cancel = new WPushButton( "No", dialog.contents() );
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

  NLSimple *new_model = NULL;
  WDialog dialog( "Upload Data To Create A New Model From:" );
  dialog.resize( WLength(95.0, WLength::Percentage), WLength(95.0, WLength::Percentage) );
  //dialog.setAttributeValue(  "style", "border: solid 3px black;");
  WtCreateNLSimple *creater = new WtCreateNLSimple( new_model, dialog.contents() );
  creater->created().connect(       &dialog, &WDialog::accept );
  creater->canceled().connect(   &dialog, &WDialog::reject );
  WDialog::DialogCode code = dialog.exec();

  if( (code==WDialog::Accepted) && new_model)
  {
    delete m_model;
    m_model = new_model;
    setModelFileName( "" );
    init( m_userDbPtr->name );
  }//if( accepted )
}//void newModel()


void WtGui::saveModel( const std::string &fileName )
{
  if( fileName == "" )
  {
    saveModelAsDialog();
    return;
  }//if( fileName.empty() )

  m_model->saveToFile( formFileSystemName(fileName) );

  {
    Dbo::Transaction transaction(m_dbSession);
    const DubUser::UsersModels &models = m_userDbPtr->models;
    Dbo::ptr<UsersModel> model = models.find().where( "fileName = ?" ).bind( fileName );
    transaction.commit();
    if( model ) return;
  }

  Dbo::Transaction transaction(m_dbSession);
  UsersModel *newModel = new UsersModel();
  newModel->user     = m_userDbPtr;
  newModel->fileName = fileName;
  newModel->created  = WDateTime::fromPosixTime( boost::posix_time::second_clock::local_time() );
  newModel->modified = newModel->created;
  Dbo::ptr<UsersModel> newModelPtr = m_dbSession.add( newModel );
  if( newModelPtr )
    cerr << "Added new model:\n  "
         << "newModel->user->name='" << newModel->user->name
         << "', newModel->fileName='" << newModel->fileName
         << "', newModel->created='" << newModel->created.toPosixTime() << endl;

  transaction.commit();

  setModelFileName( fileName );
}//void saveModel( const std::string &fileName )


void WtGui::deleteModelFile( const std::string &fileName )
{
  if( fileName.empty() ) return;

  Dbo::ptr<UsersModel> model;

  {
    Dbo::Transaction transaction(m_dbSession);
    const DubUser::UsersModels &models = m_userDbPtr->models;
    model = models.find().where( "fileName = ?" ).bind( fileName );
    if( model ) model.remove();
    transaction.commit();
  }

  if( m_userDbPtr->currentFileName == fileName )
  {
    setModelFileName( "" );
    openModelDialog();
  }//if( we need to open another model file )
}//void deleteModelFile( const std::string &fileName );



void WtGui::setModel( const std::string &fileName )
{
  try{
    NLSimple *original = m_model;
    m_model = new NLSimple( formFileSystemName(fileName) );
    setModelFileName( fileName );
    if( original ) delete original;
  }catch(...){
    cerr << "Failed to open NLSimple named " << fileName << endl;
    wApp->doJavaScript( "alert( \"Failed to open NLSimple named " + fileName + "\" )", true );
    deleteModelFile( fileName );
  }//try/catch
}//void setModel( const std::string &fileName )


void WtGui::setModelFileName( const std::string &fileName )
{
  if( m_userDbPtr->currentFileName == fileName ) return;

  Dbo::Transaction transaction(m_dbSession);
  m_userDbPtr.modify()->currentFileName = fileName;
  if( !transaction.commit() ) cerr << "setModelFileName( const string & ) failed commit" << endl;
}//void setModelFileName( const std::string &fileName )








DubEventEntry::DubEventEntry( WtGui *wtguiparent, WContainerWidget *parent )
  : WContainerWidget( parent ),
  m_time( new DateTimeSelect( "&nbsp;&nbsp;Date/Time:&nbsp;",
                              WDateTime::fromPosixTime(boost::posix_time::second_clock::local_time())
                            ) ),
  m_type( new WComboBox() ),
  m_value( new WLineEdit() ),
  m_units( new WText() ),
  m_button( new WPushButton("submit") ),
  m_wtgui( wtguiparent )
{
  setStyleClass( "DubEventEntry" );
  setInline(false);

  addWidget( new WText( "<b>Enter New Event:</b>&nbsp;&nbsp;" ) );

  Div *timeSelectDiv = new Div( "DubEventEntry_timeSelectDiv", this );
  timeSelectDiv->setInline(true);
  addWidget( timeSelectDiv );

  timeSelectDiv->addWidget( m_time );
  WPushButton *nowB = new WPushButton( "now", timeSelectDiv );
  WPushButton *lastTimeB = new WPushButton( "data end", timeSelectDiv );
  nowB->setStyleClass( "dubEventEntryTimeButton" );
  lastTimeB->setStyleClass( "dubEventEntryTimeButton" );
  nowB->clicked().connect( this, &DubEventEntry::setTimeToNow );
  lastTimeB->clicked().connect( this, &DubEventEntry::setTimeToLastData );

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
  WDateTime now = WDateTime::fromPosixTime(second_clock::local_time());
  if( (m_time->dateTime().addSecs(30*60)) < now ) m_time->set( now );
  m_type->setCurrentIndex( WtGui::kNotSelected );
  m_units->setText("");
  m_value->setText( "" );
  m_value->setEnabled(false);
  m_button->setEnabled(false);
}//void DubEventEntry::reset()


void DubEventEntry::setTimeToNow()
{
  m_time->set( WDateTime::fromPosixTime(boost::posix_time::second_clock::local_time()) );
}//void setTimeToNow()


void DubEventEntry::setTimeToLastData()
{
  if( !m_wtgui ) return;
  m_time->set( m_wtgui->m_bsEndTimePicker->top() );
}//void setTimeToLastData()


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


WtModelSettingsGui::WtModelSettingsGui( ModelSettings *modelSettings,
                                        WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_settings( modelSettings )
{
  init();
}

WtModelSettingsGui::~WtModelSettingsGui() {}
Signal<> &WtModelSettingsGui::changed() { return m_changed; }
Signal<> &WtModelSettingsGui::predictionChanged() { return m_predictionChanged; }
void WtModelSettingsGui::emitChanged() { m_changed.emit(); }
void WtModelSettingsGui::emitPredictionChanged() { m_predictionChanged.emit(); }

void WtModelSettingsGui::init()
{
  clear();
  if( !m_settings ) return;

  WGridLayout *layout = new WGridLayout();

  const int maxRows = 8;
  int row = 0, column = 0;


  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

/*
  IntSpinBox;
  DoubleSpinBox;
  TimeDurationSpinBox;

  double m_personsWeight;          //ProgramOptions::kPersonsWeight
  double m_cgmsIndivReadingUncert; //ProgramOptions::kCgmsIndivReadingUncert

  TimeDuration m_defaultCgmsDelay; //ProgramOptions::kDefaultCgmsDelay
  TimeDuration m_cgmsDelay;        //what is actually used for the delay
  TimeDuration m_predictAhead;     //ProgramOptions::kPredictAhead
  TimeDuration m_dt;               //ProgramOptions::kIntegrationDt

  PosixTime m_endTrainingTime;
  PosixTime m_startTrainingTime;


  double m_lastPredictionWeight;   //ProgramOptions::kLastPredictionWeight

  double m_targetBG;               //ProgramOptions::kTargetBG
  double m_bgLowSigma;             //ProgramOptions::kBGLowSigma
  double m_bgHighSigma;            //ProgramOptions::kBGHighSigma

//Genetic minimization paramaters
  int m_genPopSize;                //ProgramOptions::kGenPopSize
  int m_genConvergNsteps;          //ProgramOptions::kGenConvergNsteps
  int m_genNStepMutate;            //ProgramOptions::kGenNStepMutate
  int m_genNStepImprove;           //ProgramOptions::kGenNStepImprove
  double m_genSigmaMult;           //ProgramOptions::kGenSigmaMult
  double m_genConvergCriteria;     //ProgramOptions::kGenConvergCriteria
  */
}//void init()
