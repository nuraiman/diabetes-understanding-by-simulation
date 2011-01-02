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
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

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
#include <Wt/WFileUpload>
#include <Wt/WRadioButton>
#include <Wt/WButtonGroup>
#include <Wt/WGroupBox>

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
#include "CgmsDataImport.hh"

using namespace Wt;
using namespace std;


#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

WtGui *m_parent;
boost::shared_ptr<boost::recursive_mutex::scoped_lock> m_lock;

boost::recursive_mutex NLSimplePtr::sm_nLockMapMutex;
std::map<WtGui *, int> NLSimplePtr::sm_nLocksMap;


NLSimplePtr::NLSimplePtr( WtGui *gui, const bool waite, const std::string &failureMessage  )
  : NLSimpleShrdPtr(  ((gui!=NULL) ? gui->m_model : NLSimpleShrdPtr()) ),
  m_parent( gui ),
  m_lock()
{
  if( m_parent )
  {
    if( waite ) m_lock.reset( new RecursiveScopedLock( m_parent->m_modelMutex ) );
    else m_lock.reset( new RecursiveScopedLock( m_parent->m_modelMutex, boost::try_to_lock ) );
  }//if( gui )

  if( !m_parent || !m_lock->owns_lock() )
  {
    const string msg = "Warning; failed to get the NLSimplePtr thread lock\n"
                       + failureMessage;
    cerr << msg << endl;
    if( gui && (failureMessage!="") )
      gui->doJavaScript( "alert( \"" + msg + "\" )" );
    (*static_cast<NLSimpleShrdPtr *>(this)) = NLSimpleShrdPtr();
    return;
  }//if( couldn't get the lock )

  RecursiveScopedLock lock( sm_nLockMapMutex );
  if( sm_nLocksMap.find( m_parent ) == sm_nLocksMap.end() )
    sm_nLocksMap[m_parent] = 1;
  else
    sm_nLocksMap[m_parent] = sm_nLocksMap[m_parent] + 1;
}//NLSimplePtr constructor


bool NLSimplePtr::has_lock() const
{
  return ( m_lock.get() && m_lock->owns_lock() );
}//bool NLSimplePtr::has_lock() const

bool NLSimplePtr::operator!() const{ return !has_lock(); }


int NLSimplePtr::count( WtGui *gui )
{
  RecursiveScopedLock lock( sm_nLockMapMutex );

  if( sm_nLocksMap.find( gui ) == sm_nLocksMap.end() ) return 0;
  return sm_nLocksMap[gui];
}//count

void NLSimplePtr::resetCount( WtGui *gui )
{
  RecursiveScopedLock lock( sm_nLockMapMutex );
  sm_nLocksMap.erase( gui );
}//resetCount



WtGui::WtGui( const Wt::WEnvironment& env )
  : WApplication( env ),
  m_model(),
  m_dbBackend( "user_information.db" ),
  m_dbSession(),
  m_upperEqnDiv( NULL ),
  m_fileMenuPopup( NULL ),
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
  enableUpdates(true);
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
  boost::recursive_mutex::scoped_lock lock( m_modelMutex );
  m_model = NLSimpleShrdPtr();
  DubsLogin::insertCookie( m_userDbPtr->name, 0, m_dbSession ); //deltes the cookie
  requireLogin();
}//void logout()


void WtGui::resetGui()
{
  boost::recursive_mutex::scoped_lock lock( m_modelMutex, boost::try_to_lock );

  if( !lock.owns_lock() )
  {
    const string msg = "Another operation (thread) is currently working, sorry I cant do this operation of resetGui()"
                       + string(SRC_LOCATION);
    wApp->doJavaScript( "alert( \"" + msg + "\" )", true );
    cerr << msg << endl;
    return;
  }//if( couldn't get the lock )

  m_model = NLSimpleShrdPtr();
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

  if( currentFileName() == "" ) newModel();
}//void openModelDialog()


void WtGui::init( const string username )
{
  boost::recursive_mutex::scoped_lock lock( m_modelMutex );

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
  //layout->setSpacing(0);
  layout->setContentsMargins( 0, 0, 0, 0 );
  root()->setLayout( layout );

  if( m_userDbPtr->currentFileName.empty() && !m_model.get() )
  {
    openModelDialog();
    return;
  }else if( !m_userDbPtr->currentFileName.empty() )
  {
    assert( !m_model.get() );
    setModel( m_userDbPtr->currentFileName );
  }

  if( m_model.get() == NULL )
  {
    setModelFileName( "" );
    init( m_userDbPtr->name );
    return;
  }

  m_upperEqnDiv = new Div( "m_upperEqnDiv" );
  m_fileMenuPopup = new WPopupMenu();
  WPushButton *fileMenuButton = new WPushButton( "File", m_upperEqnDiv );
  fileMenuButton->setStyleClass( "fileMenuButton" );
  fileMenuButton->clicked().connect( boost::bind( &WPopupMenu::exec, m_fileMenuPopup, fileMenuButton, Wt::Vertical) );
  fileMenuButton->clicked().connect( boost::bind( &WPushButton::disable, fileMenuButton ) );

  m_fileMenuPopup->aboutToHide().connect( boost::bind( &WPushButton::enable, fileMenuButton ) );
  WPopupMenuItem *item = m_fileMenuPopup->addItem( "Save Model" );
  item->triggered().connect( boost::bind( &WtGui::saveModel, this, boost::ref(m_userDbPtr->currentFileName) ) );
  item = m_fileMenuPopup->addItem( "Save As" );
  item->triggered().connect( boost::bind( &WtGui::saveModelAsDialog, this ) );
  item = m_fileMenuPopup->addItem( "Open Model" );
  item->triggered().connect( boost::bind( &WtGui::openModelDialog, this ) );
  item = m_fileMenuPopup->addItem( "New Model" );
  item->triggered().connect( boost::bind( &WtGui::newModel, this ) );
  item->triggered().connect( boost::bind( &WPopupMenu::setHidden, m_fileMenuPopup, true ) );


  WPopupMenu *dataMenuPopup = new WPopupMenu();
  WPushButton *dataMenuButton = new WPushButton( "Data", m_upperEqnDiv );
  dataMenuButton->setStyleClass( "dataMenuButton" );
  dataMenuButton->clicked().connect( boost::bind( &WPopupMenu::exec, dataMenuPopup, dataMenuButton, Wt::Vertical) );
  dataMenuButton->clicked().connect( boost::bind( &WPushButton::disable, dataMenuButton ) );
  dataMenuPopup->aboutToHide().connect( boost::bind( &WPushButton::enable, dataMenuButton ) );

  item = dataMenuPopup->addItem( "Add Data" );
  item->triggered().connect( boost::bind( &WtGui::addDataDialog, this ) );
  item->triggered().connect( boost::bind( &WPopupMenu::setHidden, dataMenuPopup, true ) );



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
  Chart::WDataSeries insulinSeries( kFreePlasmaInsulin, Chart::LineSeries );
  m_bsGraph->addSeries( insulinSeries );




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

  updateClarkAnalysis();

  m_customEventsView = new WTableView();
  m_customEventsModel = new WStandardItemModel( this );
  m_customEventsView->setModel( m_customEventsModel );
  m_customEventGraphs.clear();


  NLSimplePtr modelPtr( this );

  /*
  foreach( NLSimple::EventDefMap::value_type &t, modelPtr->m_customEventDefs )
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
  end.setPosixTime( modelPtr->m_cgmsData.getEndTime() );
  start.setPosixTime( modelPtr->m_cgmsData.getStartTime() );
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

  WtGeneticallyOptimize *optimizationTab = new WtGeneticallyOptimize( this );
  m_tabs->addTab( optimizationTab, "Optimize" );


  Div *errorGridTabDiv = new Div( "errorGridTabDiv" );
  WBorderLayout *errorGridTabLayout = new WBorderLayout();
  errorGridTabLayout->setSpacing( 0 );
  errorGridTabLayout->setContentsMargins( 0, 0, 0, 0 );
  errorGridTabDiv->setLayout( errorGridTabLayout );
  errorGridTabLayout->addWidget( m_errorGridLegend, WBorderLayout::West );
  errorGridTabLayout->addWidget( m_errorGridGraph, WBorderLayout::Center );
  m_errorGridGraph->setMinimumSize( 400, 400 );
  m_tabs->addTab( errorGridTabDiv, "Error Grid" );


  WtModelSettingsGui *settings = new WtModelSettingsGui( &(modelPtr->m_settings) );
  m_tabs->addTab( settings, "Settings" );



  syncDisplayToModel();


  Div *cgmsDataTableDiv = new Div( "cgmsDataTableDiv" );
  WBorderLayout *cgmsDataTableLayout = new WBorderLayout();
  cgmsDataTableDiv->setLayout( cgmsDataTableLayout );
  WTableView *view = new WTableView();
  cgmsDataTableLayout->addWidget( view, WBorderLayout::Center );
  view->setModel( m_bsModel );
  view->setSortingEnabled(true);
  view->setColumnResizeEnabled(true);
  view->setAlternatingRowColors(true);
  //view->setSelectionBehavior( SelectRows );
  //view->setSelectionMode( SingleSelection );
  view->setRowHeight(22);
  view->setColumnWidth( 0, 150 );
  view->setColumnWidth( 1, 150 );
  view->setColumnWidth( 2, 150 );
  view->setMinimumSize( 200, 200 );

/*
 //As of Wt 3.1.7a calling the setColumnHidden(...) function before the
 //  WTableView widget renders causes the program to crash
  view->refresh(); //without this statment the setColumnHidden(...) statments below will cause the app to crash

  for( int i = 0; i < m_bsModel->columnCount(); ++i )
    view->setColumnHidden( i, true );

  view->setColumnHidden( kTimeData, false );
  view->setColumnHidden( kCgmsData, false );
  view->setColumnHidden( kMealData, false );
  view->setColumnHidden( kFingerStickData, false );
  view->setColumnHidden( kCustomEventData, false );


  // view->setColumnHidden( kFreePlasmaInsulin, true );
  // view->setColumnHidden( kGlucoseAbsRate, true );
  // view->setColumnHidden( kPredictedBloodGlucose, true );
  // view->setColumnHidden( kPredictedInsulinX, true );
*/


  Div *bottomRawDataDiv = new Div();
  cgmsDataTableLayout->addWidget( bottomRawDataDiv, WBorderLayout::South );

  WPushButton *addDataButton = new WPushButton( "Add Data", bottomRawDataDiv );
  addDataButton->clicked().connect( this, &WtGui::addDataDialog );


  m_tabs->addTab( cgmsDataTableDiv, "Raw Data" );


  DubEventEntry *dataEntry = new DubEventEntry( this );
  layout->addWidget( dataEntry, WBorderLayout::South );
  dataEntry->entered().connect( boost::bind(&WtGui::addData, this, _1) );

  NLSimplePtr::resetCount( this );
}//WtGui::init()



void WtGui::addDataDialog()
{
  WDialog dialog( "Upload data from file:" );

  Wt::WGroupBox *container = new Wt::WGroupBox("Select the type of data you would like to upload", dialog.contents() );

  Wt::WButtonGroup *group = new Wt::WButtonGroup( dialog.contents() );

  Wt::WRadioButton *button;
  button = new Wt::WRadioButton("Bolus", container);
  new Wt::WBreak(container);
  group->addButton(button, kBolusTaken);

  button = new Wt::WRadioButton("Meal", container);
  new Wt::WBreak(container);
  group->addButton(button, kGlucoseEaten);

  button = new Wt::WRadioButton("Fingerstick", container);
  new Wt::WBreak(container);
  group->addButton(button, kMeterReading);

  button = new Wt::WRadioButton("Calibration Fingerstick", container);
  new Wt::WBreak(container);
  group->addButton(button, kMeterCalibration);

  button = new Wt::WRadioButton("CGMS data", container);
  new Wt::WBreak(container);
  group->addButton(button, kCgmsReading);

  //button = new Wt::WRadioButton("I didn't vote", container);
  //new Wt::WBreak(container);
  //group->addButton(button, kGenericEvent);

  group->setCheckedButton(group->button(kBolusTaken));
  new WBreak( dialog.contents() );
  WFileUpload *upload = new WFileUpload( dialog.contents() );
  new WBreak( dialog.contents() );
  Div *buttonDiv = new Div( "buttonDivCentered", dialog.contents() );
  buttonDiv->setMargin( WLength::Auto, Wt::Left | Wt::Right );
  WPushButton *ok = new WPushButton( "Add Data", buttonDiv );
  ok->disable();
  WPushButton *cancel = new WPushButton( "Cancel", buttonDiv );

  upload->changed().connect( upload, &WFileUpload::upload);
  upload->uploaded().connect( boost::bind( &WPushButton::enable, ok ) );
  upload->fileTooLarge().connect( boost::bind( &WApplication::doJavaScript,
                                  wApp,
                                  string("Sorry, the file was too large, please email wcjohnson@ucdavis.edu to fix this."),
                                  false ) );
  ok->clicked().connect( &dialog, &WDialog::accept );
  cancel->clicked().connect( &dialog, &WDialog::reject );

  const WDialog::DialogCode return_code = dialog.exec();

  if( return_code == WDialog::Accepted )
  {
    const EntryType dataType = EntryType( group->checkedId() );
    addData( dataType, upload );
    zoomToFullDateRange();
  }//if( return_code == WDialog::Accepted )

}//void addDataDialog()



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
  m_bsBeginTimePicker->set( m_bsBeginTimePicker->bottom() );
  updateDisplayedDateRange();
}//void zoomToFullDateRange();


void WtGui::updateDataRange()
{
  NLSimplePtr modelPtr( this );
  if( !modelPtr ) return;

  PosixTime ptimeStart = boost::posix_time::second_clock::local_time();
  PosixTime ptimeEnd   = boost::posix_time::second_clock::local_time();
  if( !modelPtr->m_cgmsData.empty() ) ptimeStart = modelPtr->m_cgmsData.getStartTime();
  if( !modelPtr->m_cgmsData.empty() ) ptimeEnd = modelPtr->m_cgmsData.getEndTime();

  if( !modelPtr->m_fingerMeterData.empty() )
    ptimeStart = min( ptimeStart, modelPtr->m_fingerMeterData.getStartTime() );
  if( !modelPtr->m_fingerMeterData.empty() )
    ptimeEnd = max( ptimeEnd, modelPtr->m_fingerMeterData.getEndTime() );
  if( !modelPtr->m_mealData.empty() )
    ptimeStart = min( ptimeStart, modelPtr->m_mealData.getStartTime() );
  if( !modelPtr->m_mealData.empty() )
    ptimeEnd = max( ptimeEnd, modelPtr->m_mealData.getEndTime() );
  if( !modelPtr->m_predictedBloodGlucose.empty() )
    ptimeStart = min( ptimeStart, modelPtr->m_predictedBloodGlucose.getStartTime() );
  if( !modelPtr->m_predictedBloodGlucose.empty() )
    ptimeEnd = max( ptimeEnd, modelPtr->m_predictedBloodGlucose.getEndTime() );
  if( !modelPtr->m_customEvents.empty() )
    ptimeStart = min( ptimeStart, modelPtr->m_customEvents.getStartTime() );
  if( !modelPtr->m_customEvents.empty() )
    ptimeEnd = max( ptimeEnd, modelPtr->m_customEvents.getEndTime() );
  if( !modelPtr->m_predictedInsulinX.empty() )
    ptimeStart = min( ptimeStart, modelPtr->m_predictedInsulinX.getStartTime() );
  if( !modelPtr->m_predictedInsulinX.empty() )
    ptimeEnd = max( ptimeEnd, modelPtr->m_predictedInsulinX.getEndTime() );

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
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;

  switch( info.type )
  {
    case WtGui::kNotSelected: break;
    case WtGui::kCgmsReading:
      modelPtr->addCgmsData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kMeterReading:
      modelPtr->addFingerStickData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kMeterCalibration:
      modelPtr->addFingerStickData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kGlucoseEaten:
      modelPtr->addConsumedGlucose( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kBolusTaken:
      modelPtr->addBolusData( info.dateTime.toPosixTime(), info.value );
    break;
    case WtGui::kGenericEvent:
      //modelPtr->addCustomEvent( const PosixTime &time, int eventType );
    break;
    case WtGui::kNumEntryType: break;
  };//switch( et )

  syncDisplayToModel(); //taking the lazy way out and just reloading
                        //all data to the model
}//void addData( EventInformation info );




void WtGui::addData( WtGui::EntryType type, Wt::WFileUpload *fileUpload )
{
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;

  const string fileName = fileUpload->spoolFileName();
  const string clientFileName = fileUpload->clientFileName().narrow();

  CgmsDataImport::InfoType infoType = CgmsDataImport::ISig;

  switch( type )
  {
    case kCgmsReading:      infoType = CgmsDataImport::CgmsReading;     break;
    case kBolusTaken:       infoType = CgmsDataImport::BolusTaken;      break;
    case kGlucoseEaten:     infoType = CgmsDataImport::GlucoseEaten;    break;
    case kMeterReading:     infoType = CgmsDataImport::MeterReading;    break;
    case kMeterCalibration: infoType = CgmsDataImport::MeterCalibration;break;
    case kGenericEvent:     break;
    case kNotSelected:      break;
    case kNumEntryType:     break;
  };//switch( det )

  assert( infoType != CgmsDataImport::ISig );

  typedef boost::shared_ptr<ConsentrationGraph> ShrdGraphPtr;

  ShrdGraphPtr newData;

  try
  {
    ConsentrationGraph importedData = importSpreadsheet( fileName, infoType );
    newData = ShrdGraphPtr( new ConsentrationGraph(importedData) );
  }catch( exception &e )
  {
    string msg = "Warning: failed file decoding in WtGui::"
                 "addData( EntryType, WFileUpload * ):\n";
    msg += e.what();
    msg += "\nFile name=" + clientFileName;
    msg += "\nEntryType=" + boost::lexical_cast<string>( int(type) );
    wApp->doJavaScript( "alert( \"" + msg + "\" )", false );
    cerr << msg << endl;
    return;
  }catch(...)
  {
    string msg = "Warning: failed file decoding in WtGui::"
                 "( EntryType, WFileUpload * )";
    msg += "\nFile name=" + clientFileName;
    msg += "\nEntryType=" + boost::lexical_cast<string>( int(type) );
    wApp->doJavaScript( "alert( \"" + msg + "\" )", false );
    cerr << msg << endl;
    return;
  }//try / catch

  assert( newData.get() );

  const bool findNewSteadyState = false;

  switch( type )
  {
    case WtGui::kCgmsReading:
      modelPtr->addCgmsData( *newData, findNewSteadyState );
    break;
    case WtGui::kMeterReading:
      modelPtr->addFingerStickData( *newData );
    break;
    case WtGui::kMeterCalibration:
      modelPtr->addFingerStickData( *newData );
    break;
    case WtGui::kGlucoseEaten:
      modelPtr->addGlucoseAbsorption( *newData );
    break;
    case WtGui::kBolusTaken:
      modelPtr->addBolusData( *newData, findNewSteadyState );
    break;
    case WtGui::kGenericEvent:
      //modelPtr->addCustomEvent( *newData, findNewSteadyState );
    break;
    case WtGui::kNumEntryType: break;
    case WtGui::kNotSelected: break;
  };//switch( et )

  syncDisplayToModel(); //taking the lazy way out and just reloading
                        //all data to the model
}//void addData( EntryType type, const string file )


void WtGui::syncDisplayToModel()
{
  const TimeDuration plasmaInsulinDt( 0, 5, 0 );  //5 minutes
  typedef ConsentrationGraph::value_type cg_type;

  NLSimplePtr modelPtr( this, false, string(SRC_LOCATION) );
  if( !modelPtr ) return;


  int nNeededRow = modelPtr->m_cgmsData.size()
                         + modelPtr->m_fingerMeterData.size()
                         + modelPtr->m_mealData.size()
                         + modelPtr->m_predictedBloodGlucose.size()
                         + modelPtr->m_glucoseAbsorbtionRate.size()
                         + modelPtr->m_customEvents.size()
                         + modelPtr->m_predictedInsulinX.size();

  //m_freePlasmaInsulin has a ton of points, so we'll only cound every 5
  //  minutes of them, make sure this is consisten with actually putting
  //  the data into the model
  if( modelPtr->m_freePlasmaInsulin.size() )
  {
    PosixTime lastInsulin = modelPtr->m_freePlasmaInsulin.getStartTime();
    foreach( const cg_type &element, modelPtr->m_freePlasmaInsulin )
    {
      if( (element.m_time-lastInsulin) >= plasmaInsulinDt )
      {
        lastInsulin = element.m_time;
        ++nNeededRow;
      }//if( its been long enough, count this point )
    }//foreach point
  }//if( modelPtr->m_freePlasmaInsulin.size() )


  m_bsModel->removeRows( 0, m_bsModel->rowCount() );
  m_bsModel->insertRows( 0, nNeededRow );

  int row = 0;
  foreach( const cg_type &element, modelPtr->m_cgmsData )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kCgmsData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_fingerMeterData )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kFingerStickData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_mealData )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kMealData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_predictedBloodGlucose )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kPredictedBloodGlucose, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_glucoseAbsorbtionRate )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kGlucoseAbsRate, element.m_value );
  }//


  if( modelPtr->m_freePlasmaInsulin.size() )
  {
    PosixTime lastInsulin = modelPtr->m_freePlasmaInsulin.getStartTime();
    foreach( const cg_type &element, modelPtr->m_freePlasmaInsulin )
    {
      if( (element.m_time-lastInsulin) < plasmaInsulinDt ) continue;
      lastInsulin = element.m_time;
      const WDateTime x = WDateTime::fromPosixTime( element.m_time );
      m_bsModel->setData( row, kTimeData, x );
      m_bsModel->setData( row++, kFreePlasmaInsulin, element.m_value );
    }//
  }//if( modelPtr->m_freePlasmaInsulin.size() )


  foreach( const cg_type &element, modelPtr->m_customEvents )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kCustomEventData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_predictedInsulinX )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    m_bsModel->setData( row, kTimeData, x );
    m_bsModel->setData( row++, kPredictedInsulinX, element.m_value );
  }//

  updateDataRange();
}//void syncDisplayToModel()



void WtGui::updateClarkAnalysis()
{
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;

  updateClarkAnalysis( modelPtr->m_fingerMeterData, modelPtr->m_cgmsData, true );
}//void updateClarkAnalysis()

void WtGui::updateClarkAnalysis( const ConsentrationGraph &xGraph,
                                 const ConsentrationGraph &yGraph,
                                 bool isCgmsVMeter )
{
  typedef ConsentrationGraph::value_type cg_type;

  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;


  m_errorGridModel->removeRows( 0, m_errorGridModel->rowCount() );
  m_errorGridModel->insertRows( 0, xGraph.size() );

  TimeDuration cmgsDelay(0,0,0,0);

  string delayStr = "", uncertStr = "";
  if( isCgmsVMeter )
  {
    cmgsDelay = modelPtr->findCgmsDelayFromFingerStick();
    double sigma = 1000.0 * modelPtr->findCgmsErrorFromFingerStick(cmgsDelay);
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
  if( NLSimplePtr::count(this) == 0 ) return;

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

  NLSimplePtr modelPtr( this, false, SRC_LOCATION );  //just for the mutex
  if( !modelPtr ) return;

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
    m_model = NLSimpleShrdPtr( new_model );
    setModelFileName( "" );
    init( m_userDbPtr->name );
  }//if( accepted )
}//void newModel()

void WtGui::saveCurrentModel()
{
  saveModel( m_userDbPtr->currentFileName );
}//void WtGui::saveCurrentModel()

const std::string &WtGui::currentFileName()
{
  return m_userDbPtr->currentFileName;
}//const std::string &currentFileName()


void WtGui::saveModel( const std::string &fileName )
{
  if( fileName == "" )
  {
    saveModelAsDialog();
    return;
  }//if( fileName.empty() )

  {
    NLSimplePtr modelPtr( this, false, SRC_LOCATION );
    if( !modelPtr ) return;

    modelPtr->saveToFile( formFileSystemName(fileName) );
    NLSimplePtr::resetCount( this );
  }


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

  Dbo::ptr<UsersModel> usrmodel;

  {
    Dbo::Transaction transaction(m_dbSession);
    const DubUser::UsersModels &models = m_userDbPtr->models;
    usrmodel = models.find().where( "fileName = ?" ).bind( fileName );
    if( usrmodel ) usrmodel.remove();
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
  boost::recursive_mutex::scoped_lock lock( m_modelMutex, boost::try_to_lock );

  if( !lock.owns_lock() )
  {
    const string msg = "Another operation (thread) is currently working, sorry I cant do this operation of setModel( string ): "
                       + string( SRC_LOCATION );
    wApp->doJavaScript( "alert( \"" + msg + "\" )", true );
    cerr << msg << endl;
    return;
  }//if( couldn't get the lock )

  try{
    m_model = NLSimpleShrdPtr( new NLSimple( formFileSystemName(fileName) ) );
    setModelFileName( fileName );
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
  m_time->set( m_wtgui->getEndTimePicker()->top() );

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
  setStyleClass( "WtModelSettingsGui" );
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
  setLayout( layout );

  const int maxRows = 8;
  int row = 0, column = 0;

  MemVariableSpinBox *sb = new DoubleSpinBox( &(m_settings->m_personsWeight), "", "kg", 0, 250 );
  layout->addWidget( new WText("Your Weight"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_cgmsIndivReadingUncert), "", "", 0, 1 );
  layout->addWidget( new WText("Ind. CGMS Uncert."), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_defaultCgmsDelay), "", -15, 35 );
  layout->addWidget( new WText("Default CGMS Delay"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_cgmsDelay), "", -15, 35 );
  layout->addWidget( new WText("Actual CGMS Delay"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_predictAhead), "", 0, 180 );
  layout->addWidget( new WText("Amount to Predict Ahead"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_dt), "", 1.0/60.0, 30 );
  layout->addWidget( new WText("Integration delta"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_lastPredictionWeight), "", "", 0, 1 );
  layout->addWidget( new WText("Last Prediction Weight"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_targetBG), "", "mg/dL", 20, 250 );
  layout->addWidget( new WText("Target Blood Glucose"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_bgLowSigma), "", "mg/dL", 0, 50 );
  layout->addWidget( new WText("Low BG 1&sigma; Tolerance", XHTMLUnsafeText), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_bgHighSigma), "", "mg/dL", 0, 100 );
  layout->addWidget( new WText("High BG 1&sigma; Tolerance", XHTMLUnsafeText), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genPopSize), "", "Indivuduals", 2, 5000 );
  layout->addWidget( new WText("Gen. Pop. Size"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genConvergNsteps), "", "steps", 1, 200 );
  layout->addWidget( new WText("Gen. Conv. N-steps"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genNStepMutate), "", "steps", 1, 50 );
  layout->addWidget( new WText("Gen. N-step Mutate"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genNStepImprove), "", "steps", 1, 50 );
  layout->addWidget( new WText("Gen. N-step Improve"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;


  sb = new DoubleSpinBox( &(m_settings->m_genSigmaMult), "", "", 0.0, 20 );
  layout->addWidget( new WText("Gen. Mutate Sigma"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;


  sb = new DoubleSpinBox( &(m_settings->m_genConvergCriteria), "", "", 0.0, 20 );
  layout->addWidget( new WText("Gen. Conv. Criteria"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &WtModelSettingsGui::emitChanged );
  //sb->valueChanged().connect( this, &WtModelSettingsGui::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

/*
  PosixTime m_endTrainingTime;
  PosixTime m_startTrainingTime;
*/
}//void init()


WtGeneticallyOptimize::WtGeneticallyOptimize( WtGui *wtGuiParent, Wt::WContainerWidget *parent )
  : WContainerWidget( parent ), m_parentWtGui( wtGuiParent )
{
  setInline(false);
  setStyleClass( "WtGeneticallyOptimize" );

  m_layout = new WBorderLayout();
  setLayout( m_layout );

  m_graphModel = new WStandardItemModel( this );
  m_graphModel->insertColumns( m_graphModel->columnCount(), WtGui::NumDataSources );
  m_graphModel->setHeaderData( WtGui::kTimeData,              WString("Time"));
  m_graphModel->setHeaderData( WtGui::kCgmsData,              WString("CGMS Readings") );
  m_graphModel->setHeaderData( WtGui::kFreePlasmaInsulin,     WString("Free Plasma Insulin (pred.)") );
  m_graphModel->setHeaderData( WtGui::kGlucoseAbsRate,        WString("Glucose Abs. Rate (pred.)") );
  m_graphModel->setHeaderData( WtGui::kMealData,              WString("Consumed Carbohydrates") );
  m_graphModel->setHeaderData( WtGui::kFingerStickData,       WString("Finger Stick Readings") );
  m_graphModel->setHeaderData( WtGui::kCustomEventData,       WString("User Defined Events") );
  m_graphModel->setHeaderData( WtGui::kPredictedBloodGlucose, WString("Predicted Blood Glucose") );
  m_graphModel->setHeaderData( WtGui::kPredictedInsulinX,     WString("Insulin X (pred.)") );

  m_graph = new Chart::WCartesianChart(Chart::ScatterPlot);
  m_graph->setModel( m_graphModel );
  m_graph->setXSeriesColumn(WtGui::kTimeData);
  m_graph->setLegendEnabled(true);
  m_graph->setPlotAreaPadding( 200, Wt::Right );
  m_graph->setPlotAreaPadding( 70, Wt::Bottom );
  m_graph->axis(Chart::XAxis).setScale(Chart::DateTimeScale);
  m_graph->axis(Chart::XAxis).setLabelAngle(45.0);
  m_graph->axis(Chart::YAxis).setTitle( "mg/dL" );
  m_graph->setMinimumSize( 200, 200 );


  Chart::WDataSeries cgmsSeries(WtGui::kCgmsData, Chart::LineSeries);
  cgmsSeries.setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
  m_graph->addSeries( cgmsSeries );

  //Chart::WDataSeries fingerSeries( WtGui::kFingerStickData, Chart::PointSeries );
  //m_graph->addSeries( fingerSeries );

  Chart::WDataSeries mealSeries( WtGui::kMealData, Chart::PointSeries );
  m_graph->addSeries( mealSeries );

  Chart::WDataSeries predictSeries( WtGui::kPredictedBloodGlucose, Chart::LineSeries );
  m_graph->addSeries( predictSeries );

  Chart::WDataSeries freeInsSeries( WtGui::kFreePlasmaInsulin, Chart::LineSeries );
  m_graph->addSeries( freeInsSeries );

  Chart::WDataSeries predXSeries( WtGui::kPredictedInsulinX, Chart::LineSeries );
  m_graph->addSeries( predXSeries );

  Chart::WDataSeries customSeries( WtGui::kCustomEventData, Chart::LineSeries );
  m_graph->addSeries( customSeries );

  m_chi2Model = new WStandardItemModel( this );
  m_chi2Model->insertColumns( m_chi2Model->columnCount(), 2 );
  m_chi2Model->setHeaderData( 0, WString("Generation Number") );
  m_chi2Model->setHeaderData( 1, WString("Best &chi;<sup>2</sup>") );

  m_chi2Graph = new Chart::WCartesianChart(Chart::ScatterPlot);
  m_chi2Graph->setModel( m_chi2Model );
  m_chi2Graph->setXSeriesColumn(0);
  m_chi2Graph->setLegendEnabled(false);
  m_chi2Graph->setPlotAreaPadding( 70, Wt::Bottom );
  m_chi2Graph->axis(Chart::YAxis).setTitle( "Best &chi;2" );
  m_chi2Graph->setMinimumSize( 200, 200 );
  m_chi2Graph->axis(Chart::XAxis).setTitle( "Generation Number" );
  m_chi2Graph->addSeries( Chart::WDataSeries( 1, Chart::LineSeries ) );


  Div *graphsDiv = new Div();
  WGridLayout *graphLayout = new WGridLayout();
  graphsDiv->setLayout( graphLayout );
  graphLayout->addWidget( m_graph, 0, 0 );
  graphLayout->addWidget( m_chi2Graph, 1, 0 );
  graphLayout->setRowStretch( 0, 2 );
  graphLayout->setRowStretch( 1, 2 );

  m_layout->addWidget( graphsDiv, WBorderLayout::Center );



  Div *southernDiv = new Div();
  m_layout->addWidget( southernDiv, WBorderLayout::South );
  Div *timeSelectDiv = new Div( "", southernDiv );
  //timeSelectDiv->setInline(true);


  const WDateTime &top = m_parentWtGui->getBeginTimePicker()->top();
  const WDateTime &bottom = m_parentWtGui->getBeginTimePicker()->bottom();
  new WLabel( "Train using data ", timeSelectDiv );

  NLSimplePtr modelPtr( m_parentWtGui );
  PosixTime *trainStartPtr = &(modelPtr->m_settings.m_startTrainingTime);
  PosixTime *trainEndPtr = &(modelPtr->m_settings.m_endTrainingTime);

  *trainStartPtr = bottom.toPosixTime();
  *trainEndPtr = top.toPosixTime();

  m_startTrainingTimeSelect = new MemGuiTimeDate( trainStartPtr,
                                                  "from ",
                                                  bottom.toPosixTime(),
                                                  top.toPosixTime(),
                                                  timeSelectDiv );
  m_endTrainingTimeSelect = new MemGuiTimeDate( trainEndPtr,
                                                " &nbsp;&nbsp;to ",
                                                bottom.toPosixTime(),
                                                top.toPosixTime(),
                                                timeSelectDiv );
  m_parentWtGui->getBeginTimePicker()->topBottomUpdated().connect( this, &WtGeneticallyOptimize::updateDateSelectLimits );

  m_startTrainingTimeSelect->changed().connect( this, &WtGeneticallyOptimize::syncGraphDataToNLSimple );
  m_endTrainingTimeSelect->changed().connect( this, &WtGeneticallyOptimize::syncGraphDataToNLSimple );

  new WText( "&nbsp;&nbsp;&nbsp;&nbsp;", timeSelectDiv );
  m_saveAfterEachGeneration = new WCheckBox( "Save model after each generation", timeSelectDiv );
  m_saveAfterEachGeneration->setChecked();

  new WText( "Model Parameter training will only use the data"
             " in the selected range.  To modify the settings"
             " that determine how the training will be done, please"
             " see the \"Settings\" tab.  Also please note that training"
             " may take days of time, or longer if a large date range of data"
             " is used (I would recomned about 2 weeks)", XHTMLText, southernDiv );

  Div *buttonDiv = new Div( "buttonDivCentered", southernDiv );
  m_startOptimization = new WPushButton( "Start Genetic Optimization", buttonDiv );
  m_startOptimization->clicked().connect( this, &WtGeneticallyOptimize::startOptimization );

  m_stopOptimization = new WPushButton( "Stop Genetic Optimization", buttonDiv );
  m_stopOptimization->hide();
  m_stopOptimization->clicked().connect( boost::bind( &WtGeneticallyOptimize::setContinueOptimizing, this, false ) );

  m_minuit2Optimization = new WPushButton( "Minuit2 Fine Tune", buttonDiv );
  m_minuit2Optimization->clicked().connect( this, &WtGeneticallyOptimize::startMinuit2Optimization );

  syncGraphDataToNLSimple();
}//WtGeneticallyOptimize


WtGeneticallyOptimize::~WtGeneticallyOptimize() {};


void WtGeneticallyOptimize::updateDateSelectLimits()
{
  const WDateTime &top = m_parentWtGui->getBeginTimePicker()->top();
  const WDateTime &bottom = m_parentWtGui->getBeginTimePicker()->bottom();

  m_startTrainingTimeSelect->setTop( top );
  m_startTrainingTimeSelect->setBottom( bottom );
  m_endTrainingTimeSelect->setTop( top );
  m_endTrainingTimeSelect->setBottom( bottom );
}//void updateDateSelectLimits()

void WtGeneticallyOptimize::startMinuit2Optimization()
{
  new boost::thread( boost::bind( &WtGeneticallyOptimize::doMinuit2Optimization, this ) );
}//void WtGeneticallyOptimize::startMinuit2Optimization()


void WtGeneticallyOptimize::doMinuit2Optimization()
{
  //m_parentWtGui->attachThread();

  NLSimplePtr model( m_parentWtGui, false,
                     "Failed to get thread lock for Minuit2 minimization. "
                     "Are you trying to optimize the same model twice at the "
                     "same time?"
                     + string(SRC_LOCATION) );
  if( !model ) return;

  WText *text = NULL;

  {
    WApplication::UpdateLock appLock( m_parentWtGui );
    m_startOptimization->hide();
    text = new WText( "<font color=\"blue\"><b>Currently Optimizing</b></font>", XHTMLUnsafeText );
    m_layout->addWidget( text, WBorderLayout::North );
    if( appLock ) m_parentWtGui->triggerUpdate();
  }//



  model->fitModelToDataViaMinuit2( model->m_settings.m_lastPredictionWeight );


  {
    WApplication::UpdateLock appLock( m_parentWtGui );
    m_startOptimization->show();
    m_layout->removeWidget( text );
    if( appLock ) m_parentWtGui->triggerUpdate();
  }//

}//void WtGeneticallyOptimize::doMinuit2Optimization()


void WtGeneticallyOptimize::startOptimization()
{
  new boost::thread( boost::bind( &WtGeneticallyOptimize::doGeneticOptimization, this ) );
}//void WtGeneticallyOptimize::startOptimization()


void WtGeneticallyOptimize::doGeneticOptimization()
{
  //std::vector<Wt::WWidget *> m_disableWhenBusyItems;
  //foreach( WWidget *w, m_disableWhenBusyItems ) w->setDisabled(true);
  //m_parentWtGui->attachThread();

  setContinueOptimizing( true );

  NLSimplePtr model( m_parentWtGui, false,
                     "Failed to get thread lock for genetic minimization. "
                     "Are you trying to optimize the same model twice at the "
                     "same time?"
                     + string( SRC_LOCATION ) );
  if( !model ) return;

  WText *text = NULL;

  {
    WApplication::UpdateLock appLock( m_parentWtGui );
    //if( appLock )
    m_stopOptimization->show();  //TODO: figure out if we didn't get appLock, will the changes be propogated to user eventually?
    m_startOptimization->hide();
    text = new WText( "<font color=\"blue\"><b>Currently Optimizing</b></font>", XHTMLUnsafeText );
    m_layout->addWidget( text, WBorderLayout::North );
    if( appLock ) m_parentWtGui->triggerUpdate();
  }//

  m_bestChi2.clear();

  NLSimple::Chi2CalbackFcn chi2Calback = boost::bind( &WtGeneticallyOptimize::optimizationUpdateFcn, this, _1);
  NLSimple::ContinueFcn continueFcn = boost::bind( &WtGeneticallyOptimize::continueOptimizing, this);


  cerr << "about to do the workptr" << endl;
  try
  {
    model->geneticallyOptimizeModel( model->m_settings.m_lastPredictionWeight,
                                     TimeRangeVec(), chi2Calback, continueFcn );
  }catch( exception &e )
  {
    string msg = "Warning: Optimization failed:\n";
    msg += e.what();
    m_parentWtGui->doJavaScript( "alert( \"" + msg + "\" )", false );
    cerr << msg << endl;
  }catch(...)
  {
    string msg = "Warning: Optimization failed";
    m_parentWtGui->doJavaScript( "alert( \"" + msg + "\" )", false );
    cerr << msg << endl;
  }//try / catch

  m_parentWtGui->syncDisplayToModel();
  m_parentWtGui->updateClarkAnalysis();

  //just to make sure we don't loose all this work
  const std::string &fileName = m_parentWtGui->currentFileName();
  if( fileName != "" ) m_parentWtGui->saveCurrentModel();

  {
    WApplication::UpdateLock appLock( m_parentWtGui );
    m_stopOptimization->hide();
    m_startOptimization->show();
    m_layout->removeWidget( text );
    if( appLock ) m_parentWtGui->triggerUpdate();
  }//

  //foreach( WWidget *w, m_disableWhenBusyItems ) w->setDisabled(false);
}//void doGeneticOptimization()


void WtGeneticallyOptimize::setContinueOptimizing( const bool doContinue )
{
  boost::mutex::scoped_lock lock( m_continueMutex );
  m_continueOptimizing = doContinue;
}//void setContinueOptimizing( const bool doContinue )


bool WtGeneticallyOptimize::continueOptimizing()
{
  boost::mutex::scoped_lock lock( m_continueMutex );
  return m_continueOptimizing;
}//bool continueOptimizing()


void WtGeneticallyOptimize::optimizationUpdateFcn( double chi2 )
{
  WApplication::UpdateLock lock( m_parentWtGui );
  if( !lock )
  {
    cerr << "Couldn't get WApplication lock!!!" << endl;
    m_parentWtGui->doJavaScript( "alert( \"Failed to get WApplication lock in WtGeneticallyOptimize::optimizationUpdateFcn(double)\" )" );
    return;
  }//if( !lock )

  m_bestChi2.push_back( chi2 );
  syncGraphDataToNLSimple();

  if( m_saveAfterEachGeneration->isChecked() )
  {
   const std::string &fileName = m_parentWtGui->currentFileName();
   if( fileName != "" ) m_parentWtGui->saveCurrentModel();
  }//if( we want to save the model )

  // Push the changes to the browser
  wApp->triggerUpdate();
}//void optimizationUpdateFcn( double chi2 )



void WtGeneticallyOptimize::syncGraphDataToNLSimple()
{
  typedef ConsentrationGraph::value_type cg_type;


  m_chi2Model->removeRows( 0, m_chi2Model->rowCount() );
  m_chi2Model->insertRows( 0, m_bestChi2.size() );
  for( size_t i = 0; i < m_bestChi2.size(); ++i )
  {
    m_chi2Model->setData( i, 0, i );
    m_chi2Model->setData( i, 1, m_bestChi2[i] );
  }//for( add chi data to the model )

  const string error_msg = "Failed to get thread lock for syncGraphDataToNLSimple."
                     " Are you optimizing in another thread?: "
                     + string( SRC_LOCATION );

  NLSimplePtr modelPtr( m_parentWtGui, false, error_msg );
  if( !modelPtr ) return;



  const WDateTime start = m_startTrainingTimeSelect->dateTime();
  const WDateTime end = m_endTrainingTimeSelect->dateTime();

  const int nNeededRow = modelPtr->m_cgmsData.size()
                         //+ modelPtr->m_fingerMeterData.size()
                         + modelPtr->m_mealData.size()
                         + modelPtr->m_predictedBloodGlucose.size()
                         //+ modelPtr->m_glucoseAbsorbtionRate.size()
                         + modelPtr->m_freePlasmaInsulin.size()
                         + modelPtr->m_customEvents.size()
                         + modelPtr->m_predictedInsulinX.size();

  m_graphModel->removeRows( 0, m_graphModel->rowCount() );
  m_graphModel->insertRows( 0, nNeededRow );


  int row = 0;
  foreach( const cg_type &element, modelPtr->m_cgmsData )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kCgmsData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_fingerMeterData )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kFingerStickData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_mealData )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kMealData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_predictedBloodGlucose )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kPredictedBloodGlucose, element.m_value );
  }//

  PosixTime lastInsulin = modelPtr->m_freePlasmaInsulin.size() ? modelPtr->m_freePlasmaInsulin.getStartTime() : PosixTime(boost::posix_time::min_date_time);

  foreach( const cg_type &element, modelPtr->m_freePlasmaInsulin )
  {
    if( (element.m_time-lastInsulin) < TimeDuration( 0, 5, 0 ) ) continue;
    lastInsulin = element.m_time;

    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kFreePlasmaInsulin, element.m_value );
  }//


  foreach( const cg_type &element, modelPtr->m_glucoseAbsorbtionRate )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kGlucoseAbsRate, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_customEvents )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kCustomEventData, element.m_value );
  }//

  foreach( const cg_type &element, modelPtr->m_predictedInsulinX )
  {
    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kPredictedInsulinX, element.m_value );
  }//
}//void syncGraphDataToNLSimple()

