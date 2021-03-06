#include "DubsConfig.hh"

#include <math.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <limits.h>

#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <Wt/WPen>
#include <Wt/WText>
#include <Wt/WTime>
#include <Wt/WDate>
#include <Wt/WLink>
#include <Wt/WLineF>
#include <Wt/WPanel>
#include <Wt/WLabel>
#include <Wt/WTable>
#include <Wt/WDialog>
#include <Wt/WAnchor>
#include <Wt/WServer>
#include <Wt/WPainter>
#include <Wt/WSpinBox>
#include <Wt/WGroupBox>
#include <Wt/WLineEdit>
#include <Wt/WTextArea>
#include <Wt/WMenuItem>
#include <Wt/WDateTime>
#include <Wt/WComboBox>
#include <Wt/WAnimation>
#include <Wt/WTabWidget>
#include <Wt/WTableView>
#include <Wt/WTableView>
#include <Wt/WPopupMenu>
#include <Wt/WHBoxLayout>
#include <Wt/WDatePicker>
#include <Wt/WPushButton>
#include <Wt/WPushButton>
#include <Wt/WHBoxLayout>
#include <Wt/WFileUpload>
#include <Wt/WDatePicker>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WRadioButton>
#include <Wt/WButtonGroup>
#include <Wt/WItemDelegate>
#include <Wt/WBorderLayout>
#include <Wt/WIntValidator>
#include <Wt/Dbo/QueryModel>
#include <Wt/Dbo/QueryModel>
#include <Wt/Auth/AuthWidget>
#include <Wt/WContainerWidget>
#include <Wt/WDoubleValidator>
#include <Wt/WLengthValidator>
#include <Wt/Chart/WDataSeries>
#include <Wt/WStandardItemModel>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Chart/WChartPalette>
#include <Wt/Auth/PasswordService>
#include <Wt/WSortFilterProxyModel>
#include <Wt/Chart/WCartesianChart>

#if(USE_CERNS_ROOT)
#include "TH2.h"
#include "TH1.h"
#include "TH2F.h"
#include "TMD5.h"
#include "TH1F.h"
#include "TSystem.h"
#include "TRandom3.h"
#endif

#include "dubs/WtGui.hh"
#include "dubs/WtUtils.hh"
#include "dubs/DubUser.hh"
#include "dubs/DubsSession.hh"
#include "dubs/ResponseModel.hh"
#include "dubs/OverlayCanvas.hh"
#include "dubs/ProgramOptions.hh"
#include "dubs/CgmsDataImport.hh"
#include "dubs/WtChartClasses.hh"
#include "dubs/DubsApplication.hh"
#include "dubs/WtUserManagment.hh"
#include "dubs/WtCreateNLSimple.hh"
#include "dubs/GeneticEvalUtils.hh"
#include "dubs/ConsentrationGraph.hh"


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
// gui->m_userDbPtr->role==DubUser::FullUser

  if( m_parent )
  {
    if( waite )
    {
      m_lock.reset( new RecursiveScopedLock( m_parent->m_modelMutex ) );
    }else
      m_lock.reset( new RecursiveScopedLock( m_parent->m_modelMutex, boost::try_to_lock ) );
  }//if( gui )

  if( !m_parent || !m_lock->owns_lock() )
  {
    const string msg = "Warning; failed to get the NLSimplePtr thread lock\n"
                       + failureMessage;
    cerr << msg << endl;
    if( gui && (failureMessage!="") )
      gui->doJavaScript( "alert( '" + msg + "' );" );
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


bool NLSimplePtr::operator!() const
{
  return !has_lock();
}


int NLSimplePtr::count( WtGui *gui )
{
  RecursiveScopedLock lock( sm_nLockMapMutex );

  if( sm_nLocksMap.find( gui ) == sm_nLocksMap.end() )
    return 0;
  return sm_nLocksMap[gui];
}//count


void NLSimplePtr::resetCount( WtGui *gui )
{
  if( !gui )
  {
    cerr << SRC_LOCATION << "\n\tInvalid pointer passed in" << endl;
    return;
  }//if( !gui )

  RecursiveScopedLock global_lock( sm_nLockMapMutex );

  {
    RecursiveScopedLock local_lock( gui->m_modelMutex );  //hmmmm - will this cause a
    sm_nLocksMap.erase( gui );
  }
}//resetCount


WtGui::~WtGui()
{
  NLSimplePtr::resetCount( this );
}//WtGui::~WtGui()


WtGui::WtGui( Wt::Dbo::ptr<DubUser> user, Wt::WApplication *app, Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_app( app ),
    m_model(),
    m_modelMutex(),
    m_userDbPtr( user ),
    m_upperEqnDiv( NULL ),
    m_fileMenuPopup( NULL ),
    m_tabs( NULL ),
    m_optimizationTab( NULL ),
    m_nlSimleDisplayModel( new NLSimpleDisplayModel( this, NULL ) ),
    m_overviewTab( NULL ),
    m_bsModel( NULL ),
    m_bsGraph( NULL ),
    m_bsGraphOverlay( NULL ),
    m_bsBeginTimePicker( NULL ),
    m_bsEndTimePicker( NULL ),
    m_errorGridModel( NULL ),
    m_errorGridGraph( NULL ),
    m_nextTimePeriodButton( NULL ),
    m_previousTimePeriodButton( NULL ),
    m_notesTab( NULL )
{
  init();
}//WtGui constructor



void WtGui::resetGui()
{
  boost::recursive_mutex::scoped_lock lock( m_modelMutex, boost::try_to_lock );

  if( !lock.owns_lock() )
  {
    const string msg = "Another operation (thread) is currently working, sorry I cant do this operation of resetGui()"
                       + string(SRC_LOCATION);
    wApp->doJavaScript( "alert( '" + msg + "' );", true );
    cerr << msg << endl;
    return;
  }//if( couldn't get the lock )


  m_nlSimleDisplayModel->aboutToSetNewModel();
  m_model = NLSimpleShrdPtr();
  m_nlSimleDisplayModel->doneSettingNewModel();

  init();
}//void resetGui()


void WtGui::deleteModel( const string &modelname )
{
  if( !m_userDbPtr || !m_userDbPtr.session() )
    return;

  {
    Dbo::Transaction transaction( *(m_userDbPtr.session()) );
    Dbo::ptr<UsersModel> model = m_userDbPtr.modify()->models.find().where("fileName = ?").bind( modelname );
    if( model )
      model.remove();
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
  if( selected.empty() )
    button->disable();
  else
    button->enable();
}//void enableOpenModelDialogOkayButton( WPushButton *button, WTableView *view )



void WtGui::finishOpenModelDialog( WDialog *dialog, WTableView *view )
{

  WDialog::DialogCode code = dialog->result();

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
      typedef Dbo::QueryModel<Dbo::ptr<UsersModel> > ViewModel_t;
      ViewModel_t *viewmodel = dynamic_cast< ViewModel_t * >( view->model() );
      assert( viewmodel );
      setModelFileName( boost::any_cast<string>( viewmodel->data( selectedRow, 0 ) ) );
      resetGui();
    }catch(...)
    {
      cerr << endl << "Any Cast Failed in openModelDialog()" << endl;
    }

    delete dialog;
    return;
  }else if( currentFileName() == "" )
    newModel();

  delete dialog;
}//void WtGui::finishOpenModelDialog( WDialog *dialog )



void WtGui::openModelDialog()
{
  saveModelConfirmation();

  size_t nModels = 0;
  if( m_userDbPtr && m_userDbPtr.session() )
  {
    Dbo::Transaction transaction( *(m_userDbPtr.session()) );
    nModels = m_userDbPtr->models.size();
    transaction.commit();
  }//if( m_userDbPtr )

  if( nModels == 0 )
  {
    newModel();
    return;
  }//if( no models owned by user )

  WDialog *dialog = new WDialog( "Open Model" );
  new WText("Select model to open: ", dialog->contents() );
  new WBreak( dialog->contents() );
  Dbo::QueryModel<Dbo::ptr<UsersModel> > *viewmodel = new Dbo::QueryModel<Dbo::ptr<UsersModel> >( dialog->contents() );
  viewmodel->setQuery( m_userDbPtr->models.find() );
  viewmodel->addColumn( "fileName", "File Name" );
  viewmodel->addColumn( "created", "Created" );
  viewmodel->addColumn( "modified", "Modified" );

  // cerr << "Found " << m_userDbPtr->models.find().resultList().size()
  //      << " potential models to open" << endl;

  Div *viewDiv = new Div( "viewDiv", dialog->contents() );
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

  new WBreak( dialog->contents() );
  WPushButton *ok = new WPushButton( "Open", dialog->contents() );
  ok->setEnabled( false );
  WPushButton *cancel   = new WPushButton( "Cancel", dialog->contents() );
  WPushButton *newModelB = new WPushButton( "Create New Model", dialog->contents() );

  ok->clicked().connect(        dialog, &WDialog::accept );
  newModelB->clicked().connect( dialog, &WDialog::accept );
  newModelB->clicked().connect( boost::bind( &WTableView::setSelectedIndexes, view, WModelIndexSet() ) );
  cancel->clicked().connect(    dialog, &WDialog::reject );
  view->selectionChanged().connect(  boost::bind( &WtGui::enableOpenModelDialogOkayButton, this, ok, view ) );
  enableOpenModelDialogOkayButton( ok, view );

  dialog->finished().connect( boost::bind( &WtGui::finishOpenModelDialog, this, dialog, view ) );
  dialog->show();
}//void openModelDialog()




void WtGui::init()
{
  boost::recursive_mutex::scoped_lock lock( m_modelMutex );

  clear();

  setStyleClass( "root" );
  WBorderLayout *layout = new WBorderLayout();
  //layout->setSpacing(0);
  layout->setContentsMargins( 0, 0, 0, 0 );
  setLayout( layout );

  if( m_userDbPtr->currentFileName.empty() && !m_model )
  {
    openModelDialog();
    return;
  }else if( !m_userDbPtr->currentFileName.empty() && !m_model )
  {
//    assert( !m_model );
    setModel( m_userDbPtr->currentFileName );
  }

  if( !m_model )
  {
    openModelDialog();
    return;
  }//if( !m_model )


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
  item->triggered().connect( boost::bind( &WPopupMenu::setHidden, m_fileMenuPopup, true, WAnimation() ) );


  WPopupMenu *dataMenuPopup = new WPopupMenu();
  WPushButton *dataMenuButton = new WPushButton( "Data", m_upperEqnDiv );
  dataMenuButton->setStyleClass( "dataMenuButton" );
  dataMenuButton->clicked().connect( boost::bind( &WPopupMenu::exec, dataMenuPopup, dataMenuButton, Wt::Vertical) );
  dataMenuButton->clicked().connect( boost::bind( &WPushButton::disable, dataMenuButton ) );
  dataMenuPopup->aboutToHide().connect( boost::bind( &WPushButton::enable, dataMenuButton ) );

  item = dataMenuPopup->addItem( "Add Data" );
  item->triggered().connect( boost::bind( &WtGui::addDataDialog, this ) );
  item->triggered().connect( boost::bind( &WPopupMenu::setHidden, dataMenuPopup, true, WAnimation() ) );

  DubsApplication *dubsApp = dynamic_cast<DubsApplication *>( m_app );
  if( dubsApp )
  {
    WText *logout = new WText( "logout", m_upperEqnDiv );
    logout->setAttributeValue( "style", "color: blue;" );
    logout->clicked().connect( dubsApp, &DubsApplication::logout );
    logout->setFloatSide( Right );
    logout->setPadding( WLength(2,WLength::FontEx), Wt::Right );
  }//if( dubsApp )


  layout->addWidget( m_upperEqnDiv, WBorderLayout::North );

//  WPushButton *logout = new WPushButton( "logout", m_upperEqnDiv );
//  logout->setStyleClass( "logoutButton" );
//  logout->clicked().connect( boost::bind( &WtGui::logout, this) );


  m_tabs = new WTabWidget();
  m_tabs->setStyleClass( "m_tabs" );
  layout->addWidget( m_tabs, WBorderLayout::Center );


  m_bsGraph = new WChartWithLegend(this);
//  m_nlSimleDisplayModel->useAllColums();
  m_bsGraph->setModel( m_nlSimleDisplayModel.get() );
  m_bsGraph->setLegendEnabled(false);
  m_bsGraph->setPlotAreaPadding( 25, Wt::Right );
  m_bsGraph->setPlotAreaPadding( 70, Wt::Bottom );
  m_bsGraph->axis(Chart::XAxis).setScale(Chart::DateTimeScale);
  m_bsGraph->axis(Chart::XAxis).setLabelAngle(45.0);
  m_bsGraph->axis(Chart::Y2Axis).setVisible(true);
  m_bsGraph->axis(Chart::Y2Axis).setTitle( "Consumed Carbs" );
  const WPen &y2Pen = m_bsGraph->palette()->strokePen(4);
  m_bsGraph->axis(Chart::Y2Axis).setPen( y2Pen );
  m_bsGraph->axis(Chart::YAxis).setTitle( "mg/dL" );
  m_bsGraph->setMinimumSize( 200, 150 );

  m_bsGraphOverlay = new OverlayCanvas( m_bsGraph, true, true );
  m_bsGraphOverlay->userDragged().connect( this, &WtGui::userDragZoomedBsGraph );

  m_nlSimleDisplayModel->useColumn(NLSimple::kPredictedBloodGlucose);
  Chart::WDataSeries predictSeries( m_nlSimleDisplayModel->columnCount()-2, Chart::LineSeries );
  m_bsGraph->addSeries( predictSeries );

  m_nlSimleDisplayModel->useColumn(NLSimple::kCgmsData);
  Chart::WDataSeries cgmsSeries(m_nlSimleDisplayModel->columnCount()-2, Chart::LineSeries);
  //cgmsSeries.setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
  m_bsGraph->addSeries( cgmsSeries );

  m_nlSimleDisplayModel->useColumn(NLSimple::kFingerMeterData);
  Chart::WDataSeries fingerSeries( m_nlSimleDisplayModel->columnCount()-2, Chart::PointSeries );
  m_bsGraph->addSeries( fingerSeries );

  m_nlSimleDisplayModel->useColumn(NLSimple::kCalibrationData);
  Chart::WDataSeries calibrationSeries( m_nlSimleDisplayModel->columnCount()-2, Chart::PointSeries );
  m_bsGraph->addSeries( calibrationSeries );

  m_nlSimleDisplayModel->useColumn(NLSimple::kMealData);
  Chart::WDataSeries mealSeries( m_nlSimleDisplayModel->columnCount()-2, Chart::PointSeries, Chart::Y2Axis );
  m_bsGraph->addSeries( mealSeries );


//  m_nlSimleDisplayModel->useColumn(NLSimple::kFreePlasmaInsulin);
//  Chart::WDataSeries insulinSeries( m_nlSimleDisplayModel->columnCount()-2, Chart::LineSeries );
//  m_bsGraph->addSeries( insulinSeries );

  m_bsGraph->setXSeriesColumn( m_nlSimleDisplayModel->columnCount()-1 );


  m_errorGridModel = new WStandardItemModel(  this );
  m_errorGridModel->insertColumns( m_errorGridModel->columnCount(), NumErrorRegions );

  m_errorGridGraph = new ClarkErrorGridGraph( Chart::ScatterPlot );
  m_errorGridLegend = new Div( "m_errorGridLegend" );
  m_errorGridGraph->setModel( m_errorGridModel );
  m_errorGridGraph->setXSeriesColumn(0);
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

  DubsApplication *app = dynamic_cast<DubsApplication *>( m_app );

  const bool isMobile = (app && app->isMobile());
  DubEventEntry *dataEntry = new DubEventEntry( isMobile, this );
  dataEntry->entered().connect( boost::bind(&WtGui::addData, this, _1) );

  if( isMobile )
  {
    m_tabs->addTab( dataEntry, "Event Entry" );
  }else
  {
    layout->addWidget( dataEntry, WBorderLayout::South );

  }//if( isMobile ) / else



  NLSimplePtr modelPtr( this );

  /*
  foreach( NLSimple::EventDefMap::value_type &t, modelPtr->m_customEventDefs )
  {
  }
  */

//  string local_url = "local_resources/";
//  if( boost::algorithm::contains( url(), "dubs.app" ) )
//    local_url = "dubs/exec/local_resources/";

  Div *datePickingDiv  = new Div( "datePickingDiv" );
  m_previousTimePeriodButton = new WPushButton( "Previous", datePickingDiv );
//  m_previousTimePeriodButton->setIcon( local_url + "back_arrow.gif" );
  m_previousTimePeriodButton->setFloatSide( Left );
  m_previousTimePeriodButton->clicked().connect( this, &WtGui::showPreviousTimePeriod );

  WDateTime starDisplayTime = WDateTime::fromPosixTime( modelPtr->m_cgmsData.getStartTime() );
  WDateTime endDisplayTime = WDateTime::fromPosixTime( modelPtr->m_cgmsData.getEndTime() );

  m_bsBeginTimePicker  = new DateTimeSelect( "Start Date/Time:&nbsp;",
                                              starDisplayTime,
                                             datePickingDiv );
  m_bsEndTimePicker    = new DateTimeSelect( "&nbsp;&nbsp;&nbsp;&nbsp;End Date/Time:&nbsp;",
                                             endDisplayTime,
                                             datePickingDiv );


  WPushButton *zoomOut = new WPushButton( "Full Date Range", datePickingDiv );
  zoomOut->clicked().connect( this, &WtGui::zoomToFullDateRange );
  WPushButton *mostRecentDay = new WPushButton( "Most Recent Day", datePickingDiv );
  mostRecentDay->clicked().connect( this, &WtGui::zoomMostRecentDay );


  m_nextTimePeriodButton = new WPushButton( "Next", datePickingDiv );
//  m_nextTimePeriodButton->setIcon( local_url + "forward_arrow.gif" );
  m_nextTimePeriodButton->setFloatSide( Right );
  m_nextTimePeriodButton->clicked().connect( this, &WtGui::showNextTimePeriod );


  //set the displayed data range to the last range used
  if( m_userDbPtr && m_userDbPtr.session() )
  {
    Dbo::Transaction transaction( *(m_userDbPtr.session()) );
    const DubUser::UsersModels &models = m_userDbPtr->models;
    Dbo::ptr<UsersModel> model = models.find().where( "fileName = ?" ).bind( m_userDbPtr->currentFileName );
    if( model && model->displayOptions.size() )
    {
      Dbo::ptr<ModelDisplayOptions> displayOptions = model->displayOptions.front();

      if( displayOptions->displayBegin > displayOptions->displayEnd )
        std::swap( displayOptions.modify()->displayBegin, displayOptions.modify()->displayEnd );

      m_bsBeginTimePicker->set( displayOptions->displayBegin );
      m_bsEndTimePicker->set( displayOptions->displayEnd );
    }//if( model )
    transaction.commit();
  }//if( m_userDbPtr )

  updateDataRange();

  m_bsBeginTimePicker->changed().connect( boost::bind( &WtGui::updateDisplayedDateRange, this ) );
  m_bsEndTimePicker->changed().connect( boost::bind( &WtGui::updateDisplayedDateRange, this ) );

  m_overviewTab = new Div( "bsTabDiv" );
  WBorderLayout *bsTabLayout = new WBorderLayout();
  bsTabLayout->setSpacing( 0 );
  bsTabLayout->setContentsMargins( 0, 0, 0, 0 );
  m_overviewTab->setLayout( bsTabLayout );
  bsTabLayout->addWidget( m_bsGraph, WBorderLayout::Center );
  bsTabLayout->addWidget( datePickingDiv, WBorderLayout::South );
  m_tabs->addTab( m_overviewTab, "Display" );

  /*Div *optionsTabDiv = new Div( "optionsTabDiv" );
  m_tabs->addTab( optionsTabDiv, "Options" );*/


  m_optimizationTab = new GeneticallyOptimizeTab( this );
  m_tabs->addTab( m_optimizationTab, "Optimize" );

  Div *errorGridTabDiv = new Div( "errorGridTabDiv" );
  WBorderLayout *errorGridTabLayout = new WBorderLayout();
  errorGridTabLayout->setSpacing( 0 );
  errorGridTabLayout->setContentsMargins( 0, 0, 0, 0 );
  errorGridTabDiv->setLayout( errorGridTabLayout );
  errorGridTabLayout->addWidget( m_errorGridLegend, WBorderLayout::West );
  errorGridTabLayout->addWidget( m_errorGridGraph, WBorderLayout::Center );
  m_errorGridGraph->setMinimumSize( 150, 150 );
  m_tabs->addTab( errorGridTabDiv, "Error Grid" );


  ModelSettingsTab *settings = new ModelSettingsTab( &(modelPtr->m_settings) );
  m_tabs->addTab( settings, "Settings" );

  Div *cgmsDataTableDiv = new Div( "cgmsDataTableDiv" );
  WGridLayout *cgmsDataTableLayout = new WGridLayout();
  cgmsDataTableDiv->setLayout( cgmsDataTableLayout );

  m_inputModels.push_back( new WtConsGraphModel( this, NLSimple::kMealData,        this ) );
  m_inputModels.push_back( new WtConsGraphModel( this, NLSimple::kFingerMeterData, this ) );
  m_inputModels.push_back( new WtConsGraphModel( this, NLSimple::kCalibrationData, this ) );
  m_inputModels.push_back( new WtConsGraphModel( this, NLSimple::kCustomEvents,    this ) );
  m_inputModels.push_back( new WtConsGraphModel( this, NLSimple::kCgmsData,        this ) );
  m_inputModels.push_back( new WtConsGraphModel( this, NLSimple::kBoluses,         this ) );



  for( size_t i = 0; i < m_inputModels.size(); ++i )
  {
    NLSimple::DataGraphs type = m_inputModels[i]->type();
    WString title;
    switch(type)
    {
      case NLSimple::kCgmsData:              title = "CGMS Data";         break;
      case NLSimple::kGlucoseAbsorbtionRate: title = "Gluc. Abs. Rate";   break;
      case NLSimple::kMealData:              title = "Carbohydrates";     break;
      case NLSimple::kFingerMeterData:       title = "Finger Stick";      break;
      case NLSimple::kCalibrationData:       title = "Calibration Stick"; break;
      case NLSimple::kCustomEvents:          title = "Custom Events";     break;
      case NLSimple::kBoluses:               title = "Boluses";           break;
      case NLSimple::kPredictedInsulinX:     title = "Pred. Ins. X";      break;
      case NLSimple::kPredictedBloodGlucose: title = "Pred. Blood Gluc."; break;
      case NLSimple::kFreePlasmaInsulin:     title = "Plasma Insulin";    break;
      case NLSimple::kNumDataGraphs:         title = "Time";              break;
    };//enum Columns

    title = "<b>" + title + ":</b>";

    const int local_row = 3 * (static_cast<int>(i)/3);
    const int local_col = static_cast<int>(i)%3;

    WText *text = new WText( title, XHTMLUnsafeText );
    cgmsDataTableLayout->addWidget( text, local_row, local_col, 1, 1, AlignLeft | AlignTop );

    WTableView *rawDataView = new WTableView();
    cgmsDataTableLayout->addWidget( rawDataView, 1+local_row, local_col, 1, 1 );
    rawDataView->setModel( m_inputModels[i] );
    rawDataView->setColumnResizeEnabled(true);
    rawDataView->setAlternatingRowColors(true);
    rawDataView->setSelectionBehavior( SelectRows );
    rawDataView->setSelectionMode( ExtendedSelection );
    rawDataView->setSortingEnabled( false );
    rawDataView->setRowHeight(22);
    rawDataView->setColumnWidth( 0, 125 );
    rawDataView->setColumnWidth( 1, 70 );
    rawDataView->setMinimumSize( 195, 150 );

    Div *buttonDiv = new Div();
    WPushButton *delDataButton = new WPushButton( "Delete Selected", buttonDiv );
    // if( type == NLSimple::kBoluses ) delDataButton->clicked().connect( boost::bind( &WtGui::refreshInsConcFromBoluses, this) );
    // if( type == NLSimple::kMealData ) delDataButton->clicked().connect( boost::bind( &WtGui::refreshClucoseConcFromMealData, this) );
    delDataButton->clicked().connect( boost::bind( &WtGui::delRawData, this, rawDataView ) );
    delDataButton->disable();
    cgmsDataTableLayout->addWidget( buttonDiv, 2+local_row, local_col, 1, 1 );
    rawDataView->selectionChanged().connect( boost::bind( &WtGui::enableRawDataDelButton, this, rawDataView, delDataButton ) );

    if( type == NLSimple::kBoluses )
    {
      WPushButton *refreshButton = new WPushButton( "Rebuild Insulin Conc.", buttonDiv );
      refreshButton->clicked().connect( boost::bind( &WtGui::refreshInsConcFromBoluses, this) );
    }//if( type == NLSimple::kBoluses )

    if( type == NLSimple::kMealData )
    {
      WPushButton *refreshButton = new WPushButton( "Rebuild Clucose Conc.", buttonDiv );
      refreshButton->clicked().connect( boost::bind( &WtGui::refreshClucoseConcFromMealData, this) );
    }//if( type == NLSimple::kGlucoseAbsorbtionRate )


    cgmsDataTableLayout->setRowStretch( local_row, 0 );
    cgmsDataTableLayout->setRowStretch( 1+local_row, 10 );
    cgmsDataTableLayout->setRowStretch( 2+local_row, 0 );
  }//for( loop over ... )

  cgmsDataTableLayout->setRowStretch( static_cast<int>( 3 * (m_inputModels.size()/3)), 0 );

  Div *bottomRawDataDiv = new Div();
  cgmsDataTableLayout->addWidget( bottomRawDataDiv, static_cast<int>( 3*m_inputModels.size()/3), 0, 1, 3, AlignLeft | AlignTop );
  WPushButton *addDataButton = new WPushButton( "Add Data", bottomRawDataDiv );
  addDataButton->clicked().connect( this, &WtGui::addDataDialog );

  m_tabs->addTab( cgmsDataTableDiv, "Raw Data" );

  CustomEventTab *customEventTab = new CustomEventTab( this );
  m_tabs->addTab( customEventTab, "Custom Events" );


  //Create 'Notes' Tabs
  m_notesTab = new NotesTab( this );
  /*WMenuItem *tabItem = */m_tabs->addTab( m_notesTab, "Notes" );

  m_excludeTimeRangeTab = new ExcludeTimeRangesTab( this );
  m_tabs->addTab( m_excludeTimeRangeTab, "Excluded Data"  );


  m_tabs->currentChanged().connect( boost::bind( &WtGui::tabClickedCallback, this, _1 ) );

  syncDisplayToModel();
//  zoomToFullDateRange();

  NLSimplePtr::resetCount( this );
}//WtGui::init()



void WtGui::delRawData( WTableView *view )
{
  WtConsGraphModel *model = dynamic_cast<WtConsGraphModel *>( view->model() );
  assert( model );
  const WModelIndexSet selected = view->selectedIndexes();
  //'selected' is column zero of the selected rows
  view->setSelectedIndexes( WModelIndexSet() );

  WDialog dialog( "Confirmation" );
  const string msg = "Are you sure you would like to delete these %u "
                     "data points?" + boost::lexical_cast<string>(selected.size());
  new WText( msg, dialog.contents() );
  new WBreak( dialog.contents() );
  WPushButton *ok = new WPushButton( "Yes", dialog.contents() );
  WPushButton *no = new WPushButton( "No", dialog.contents() );
  ok->clicked().connect( &dialog, &WDialog::accept );
  no->clicked().connect( &dialog, &WDialog::reject );
  WDialog::DialogCode code = dialog.exec();
  if( code == WDialog::Rejected ) return;

  //We have to be careful here, since if we delete data cooresponding to an
  //  index then the rest of the indices become invalid
  vector<PosixTime> toDelete;

  foreach( const WModelIndex &index, selected )
  {
    //model->removeRow( index.data() );
    try
    {
      const WDateTime wdt = boost::any_cast<WDateTime>( index.data() );
      toDelete.push_back( wdt.toPosixTime() );
    }catch(...){ cerr << SRC_LOCATION << " Failed cast :(" << endl; }
  }//foreach selected index

  foreach( const PosixTime &t, toDelete ) model->removeRow( t );

  syncDisplayToModel();
}//void WtGui::delRawData()


void WtGui::enableRawDataDelButton( WTableView *view, Wt::WPushButton *button )
{
  if( view->selectedIndexes().empty() ) button->disable();
  else button->enable();
}//void WtGui::enableRawDataDelButton()


void WtGui::showNextTimePeriod()
{
  const PosixTime &begin = m_nlSimleDisplayModel->beginDisplayTime();
  const PosixTime &end = m_nlSimleDisplayModel->endDisplayTime();
  const TimeDuration duration = end - begin;

  const PosixTime dataEnd = m_bsEndTimePicker->top().toPosixTime();
  const PosixTime newEnd = ((end + duration) < dataEnd) ? (end + duration) : dataEnd;
  const PosixTime newBegin = newEnd - duration;

  m_bsEndTimePicker->set( WDateTime::fromPosixTime(newEnd) );
  m_bsBeginTimePicker->set( WDateTime::fromPosixTime(newBegin) );
  updateDisplayedDateRange();
}//void showNextTimePeriod()

void WtGui::showPreviousTimePeriod()
{
  const PosixTime &begin = m_nlSimleDisplayModel->beginDisplayTime();
  const PosixTime &end = m_nlSimleDisplayModel->endDisplayTime();
  const TimeDuration duration = end - begin;

  const PosixTime dataBegin = m_bsBeginTimePicker->bottom().toPosixTime();
  const PosixTime newBegin = ((begin-duration) > dataBegin) ? (begin-duration) : dataBegin;
  const PosixTime newEnd = newBegin + duration;

  m_bsEndTimePicker->set( WDateTime::fromPosixTime(newEnd) );
  m_bsBeginTimePicker->set( WDateTime::fromPosixTime(newBegin) );
  updateDisplayedDateRange();
}//void showPreviousTimePeriod()



void WtGui::refreshInsConcFromBoluses()
{
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;

  modelPtr->refreshInsConcFromBoluses();
  syncDisplayToModel();
}//void WtGui::refreshInsConcFromBoluses()


void WtGui::refreshClucoseConcFromMealData()
{
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;

  modelPtr->refreshClucoseConcFromMealData();
  syncDisplayToModel();
}


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

  button = new Wt::WRadioButton("Custom Events data", container);
  new Wt::WBreak(container);
  group->addButton(button, kGenericEvent );


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
  if( dialog->result() == WDialog::Accepted )
    saveModel( newFileName );

  //delete dialog;  //crashes for some reason even when non modal
}//void saveModelAsDialog()


void WtGui::zoomToFullDateRange()
{
  m_bsEndTimePicker->set( m_bsEndTimePicker->top() );
  m_bsBeginTimePicker->set( m_bsBeginTimePicker->bottom() );
  updateDisplayedDateRange();
}//void zoomToFullDateRange();

void WtGui::zoomMostRecentDay()
{
  m_bsEndTimePicker->set( m_bsEndTimePicker->top() );
  m_bsBeginTimePicker->set( m_bsEndTimePicker->top().addDays(-1) );
  updateDisplayedDateRange();
}//void WtGui::zoomMostRecentDay()



void WtGui::userDragZoomedBsGraph( int x0, int y0, Wt::WMouseEvent event )
{
  if( fabs(event.document().x - double(x0)) < 20 )
    return;

  const WPointF start = m_bsGraph->mapFromDevice( WPointF(x0, y0) );
  const WPointF finish = m_bsGraph->mapFromDevice( WPointF(event.widget().x, event.widget().y) );

  const std::time_t startEpich = start.x();
  const std::time_t finishEpich = finish.x();

  WDateTime startTime = WDateTime::fromTime_t( startEpich );
  WDateTime finishTime = WDateTime::fromTime_t( finishEpich );

  if( startTime > finishTime )
  {
    std::swap( finishTime, startTime );

    const PosixTime oldBeginDisplay = m_bsBeginTimePicker->dateTime().toPosixTime();
    const PosixTime oldEndDisplay = m_bsBeginTimePicker->dateTime().toPosixTime();

    TimeDuration displayDuration = oldEndDisplay - oldBeginDisplay;
    TimeDuration selectDuration = finishTime.toPosixTime() - startTime.toPosixTime();

    PosixTime newBeginDisplay, newEnddisplay;

    if( selectDuration.ticks() < 0.025*displayDuration.ticks() )
    {
      //do nothing
      return;
    }else if( selectDuration.ticks() < 0.2*displayDuration.ticks() )
    {
      double multiplier = 1.5;  //quandruple the time range
      if( selectDuration.ticks() < 0.1*displayDuration.ticks() ) //double the time range
        multiplier = 0.5;
      TimeDuration dur( multiplier*displayDuration.hours(), multiplier*displayDuration.minutes(), multiplier*displayDuration.seconds(), multiplier*displayDuration.fractional_seconds() );
      newBeginDisplay = oldBeginDisplay - dur;
      if( newBeginDisplay < m_bsBeginTimePicker->bottom().toPosixTime() )
        newBeginDisplay = m_bsBeginTimePicker->bottom().toPosixTime();

      newEnddisplay = oldEndDisplay + dur;
      if( newEnddisplay > m_bsEndTimePicker->top().toPosixTime() )
        newEnddisplay = m_bsEndTimePicker->top().toPosixTime();
    }else
    {  //zoom all the way out
      newBeginDisplay = m_bsBeginTimePicker->bottom().toPosixTime();
      newEnddisplay = m_bsEndTimePicker->top().toPosixTime();
    }

    startTime = WDateTime::fromPosixTime(newBeginDisplay);
    finishTime = WDateTime::fromPosixTime(newEnddisplay);
  }//if( startTime > finishTime )

  if( finishTime > m_bsEndTimePicker->top() )
    finishTime = m_bsEndTimePicker->top();

  if( startTime < m_bsBeginTimePicker->bottom() )
    startTime = m_bsBeginTimePicker->bottom();

  m_bsEndTimePicker->set( finishTime );
  m_bsBeginTimePicker->set( startTime );
  updateDisplayedDateRange();
}//void userDragZoomedBsGraph( int x1, int x2, Wt::WMouseEvent event )



void WtGui::updateDataRange()
{
  NLSimplePtr modelPtr( this );
  if( !modelPtr ) return;

  PosixTime ptimeStart(boost::posix_time::max_date_time);
  PosixTime ptimeEnd(boost::posix_time::min_date_time);

  for( NLSimple::DataGraphs graph = NLSimple::DataGraphs(0);
       graph < NLSimple::kNumDataGraphs;
       graph = NLSimple::DataGraphs(graph+1) )
  {
    const ConsentrationGraph &data = modelPtr->dataGraph(graph);
    if( !data.empty() )
    {
      ptimeEnd = max( ptimeEnd, data.getEndTime() );
      ptimeStart = min( ptimeStart, data.getStartTime() );
    }//if( !data.empty() )
  }//for(...)

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
  if( !m_userDbPtr || !m_userDbPtr.session() )
    return;

  const PosixTime end = m_bsEndTimePicker->dateTime().toPosixTime();
  const PosixTime start = m_bsBeginTimePicker->dateTime().toPosixTime();
  m_nlSimleDisplayModel->setDisplayedTimeRange( start, end );
//  m_bsGraph->axis(Chart::XAxis).setRange( Wt::asNumber(start), Wt::asNumber(end) );

  if( end >= m_bsEndTimePicker->top().toPosixTime() )
    m_nextTimePeriodButton->disable();
  else
    m_nextTimePeriodButton->enable();

  if( start <= m_bsBeginTimePicker->bottom().toPosixTime() )
    m_previousTimePeriodButton->disable();
  else
    m_previousTimePeriodButton->enable();

  //Now save to database
  Dbo::Transaction transaction( *(m_userDbPtr.session()) );
  const DubUser::UsersModels &models = m_userDbPtr->models;

  Dbo::ptr<UsersModel> model = models.find().where( "fileName = ?" ).bind( m_userDbPtr->currentFileName );

  if( model && model->displayOptions.size() )
  {
    Dbo::ptr<ModelDisplayOptions> displayOptions = model->displayOptions.front();
    displayOptions.modify()->displayBegin = WDateTime::fromPosixTime( start );
    displayOptions.modify()->displayEnd = WDateTime::fromPosixTime( end );
  }else
  {
    cerr << "Failed to find a model with the filename '" << m_userDbPtr->currentFileName
         << "'" << endl;
  }//if( model ) / else

  transaction.commit();
}//void updateDisplayedDateRange()


void WtGui::addData( WtGui::EventInformation info )
{
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;


  CgmsDataImport::InfoType type = CgmsDataImport::ISig;
  switch( info.type )
  {
    case WtGui::kNotSelected: assert(0); break;
    case WtGui::kCgmsReading:      type = CgmsDataImport::CgmsReading; break;
    case WtGui::kMeterReading:     type = CgmsDataImport::MeterReading; break;
    case WtGui::kMeterCalibration: type = CgmsDataImport::MeterCalibration; break;
    case WtGui::kGlucoseEaten:     type = CgmsDataImport::GlucoseEaten; break;
    case WtGui::kBolusTaken:       type = CgmsDataImport::BolusTaken; break;
    case WtGui::kGenericEvent:     type = CgmsDataImport::GenericEvent; break;
    case WtGui::kNumEntryType: assert(0); break;
  };//switch( et )

  m_nlSimleDisplayModel->addData( type, info.dateTime.toPosixTime(), info.value );

/*
  switch( info.type )
  {
  case WtGui::kNotSelected: break;
  case WtGui::kCgmsReading:
    modelPtr->addCgmsData( info.dateTime.toPosixTime(), info.value );
    break;
  case WtGui::kMeterReading:
    modelPtr->addNonCalFingerStickData( info.dateTime.toPosixTime(), info.value );
    break;
  case WtGui::kMeterCalibration:
    modelPtr->addCalibrationData( info.dateTime.toPosixTime(), info.value );
    break;
  case WtGui::kGlucoseEaten:
    modelPtr->addConsumedGlucose( info.dateTime.toPosixTime(), info.value );
    break;
  case WtGui::kBolusTaken:
    modelPtr->addBolusData( info.dateTime.toPosixTime(), info.value );
    break;
  case WtGui::kGenericEvent:
    modelPtr->addCustomEvent( info.dateTime.toPosixTime(), info.value );
    break;
  case WtGui::kNumEntryType: break;
  };//switch( et )
*/
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
    case kGenericEvent:     infoType = CgmsDataImport::GenericEvent;    break;
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
    wApp->doJavaScript( "alert( '" + msg + "' );", false );
    cerr << msg << endl;
    return;
  }catch(...)
  {
    string msg = "Warning: failed file decoding in WtGui::"
                 "( EntryType, WFileUpload * )";
    msg += "\nFile name=" + clientFileName;
    msg += "\nEntryType=" + boost::lexical_cast<string>( int(type) );
    wApp->doJavaScript( "alert( '" + msg + "' );", false );
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
      modelPtr->addNonCalFingerStickData( *newData );
    break;
    case WtGui::kMeterCalibration:
      modelPtr->addCalibrationData( *newData );
    break;
    case WtGui::kGlucoseEaten:
      modelPtr->addGlucoseAbsorption( *newData );
    break;
    case WtGui::kBolusTaken:
      modelPtr->addBolusData( *newData, findNewSteadyState );
    break;
    case WtGui::kGenericEvent:
      modelPtr->addCustomEvents( *newData );
    break;
    case WtGui::kNumEntryType: break;
    case WtGui::kNotSelected: break;
  };//switch( et )

  syncDisplayToModel(); //taking the lazy way out and just reloading
                        //all data to the model
}//void addData( EntryType type, const string file )


void WtGui::syncDisplayToModel()
{
  //TODO: Whenever this function is called, we could much more efficiently
  //      tell the WViews what has changed...
//  WApplication::UpdateLock appLock( this );

  updateDataRange();
  m_bsGraph->update();
  foreach( WtConsGraphModel *model, m_inputModels )
    model->refresh();
  //m_notesTab->updateViewTable();
}//void syncDisplayToModel()



void WtGui::updateClarkAnalysis()
{
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;

  const ConsentrationGraph &finger = modelPtr->m_fingerMeterData;
  const ConsentrationGraph &calibration = modelPtr->m_calibrationData;
  const ConsentrationGraph &cgms = modelPtr->m_cgmsData;
  const size_t nMin = size_t( modelPtr->m_settings.m_minFingerStickForCharacterization );

  if( finger.size() >= nMin ) updateClarkAnalysis( finger, cgms, true, false );
  else
  {
    ConsentrationGraph combined = finger;
    combined.addNewDataPoints( calibration );
    updateClarkAnalysis( combined, cgms, true, true );
  }//
}//void updateClarkAnalysis()

void WtGui::updateClarkAnalysis( const ConsentrationGraph &xGraph,
                                 const ConsentrationGraph &yGraph,
                                 bool isCgmsVMeter,
                                 bool isUsingCalibrationData )
{
  typedef ConsentrationGraph::value_type cg_type;

  WApplication::UpdateLock appLock( m_app );
  NLSimplePtr modelPtr( this, false, SRC_LOCATION );
  if( !modelPtr ) return;


  m_errorGridModel->removeRows( 0, m_errorGridModel->rowCount() );
  m_errorGridModel->insertRows( 0, (int)xGraph.size() );

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

  if( isUsingCalibrationData )
    new WText( "<font color=\"red\">Using Calib. Data!</font>", XHTMLUnsafeText, m_errorGridLegend );

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
  if( code == WDialog::Accepted )
    saveModel( m_userDbPtr->currentFileName );
}//void saveModelConfirmation()


void WtGui::newModel()
{
  saveModelConfirmation();

  NLSimplePtr modelPtr( this, false, SRC_LOCATION );  //just for the mutex
  if( !modelPtr )
    return;

  WDialog *dialog = new WDialog( "Upload Data To Create A New Model From:" );
  WtCreateNLSimple *creater = new WtCreateNLSimple( dialog->contents() );
  dialog->resize( WLength(80.0, WLength::Percentage), WLength(80.0, WLength::Percentage) );
  //dialog->setAttributeValue(  "style", "border: solid 3px black;");
  creater->created().connect(  dialog, &WDialog::accept );
  creater->canceled().connect( dialog, &WDialog::reject );
  dialog->setResizable( true );
  dialog->finished().connect(  boost::bind( &WtGui::createNLSimpleDialogFinished, this, creater, dialog ) );
  dialog->setModal( true );
  dialog->show();
}//void newModel()


void WtGui::createNLSimpleDialogFinished( WtCreateNLSimple *creator,  WDialog *dialog )
{
  const WDialog::DialogCode code = dialog->result();

  if( code == WDialog::Accepted )
  {
    m_nlSimleDisplayModel->aboutToSetNewModel();
    m_model = creator->model();
    saveModel( creator->modelName() );
    m_nlSimleDisplayModel->doneSettingNewModel();

    init();
  }//if( code == WDialog::Accepted )

  delete dialog;
}//void CreateNLSimpleDialogFinished( WDialog::DialogCode code )


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
  if( !m_userDbPtr || !m_userDbPtr.session() )
    return;

  if( fileName == "" )
  {
    saveModelAsDialog();
    return;
  }//if( fileName.empty() )

  std::stringstream serializedModelStream;

  {
    NLSimplePtr modelPtr( this, false, SRC_LOCATION );
    if( !modelPtr )
      return;

    boost::archive::text_oarchive oa( serializedModelStream );
    oa << *modelPtr;

//    modelPtr->saveToFile( formFileSystemName(fileName) );
    NLSimplePtr::resetCount( this );
  }


  {
    Dbo::Transaction transaction( *(m_userDbPtr.session()) );
    const DubUser::UsersModels &models = m_userDbPtr->models;
    Dbo::ptr<UsersModel> model = models.find().where( "fileName = ?" ).bind( fileName );

    if( model )
    {
      model.modify()->modified = WDateTime::fromPosixTime( boost::posix_time::second_clock::local_time() );
      model.modify()->serializedData = serializedModelStream.str();
      transaction.commit();
      return;
    }
    transaction.commit();
  }

  //If we have made it here, this model has not been saved before, so lets do
  //  this
  Dbo::Transaction transaction( *(m_userDbPtr.session()) );

  Dbo::ptr<UsersModel> newModelPtr = m_userDbPtr.session()->add( new UsersModel() );
  newModelPtr.modify()->user     = m_userDbPtr;
  newModelPtr.modify()->fileName = fileName;
  newModelPtr.modify()->created  = WDateTime::fromPosixTime( boost::posix_time::second_clock::local_time() );
  newModelPtr.modify()->modified = newModelPtr->created;
  newModelPtr.modify()->serializedData = serializedModelStream.str();

  Dbo::ptr<ModelDisplayOptions> displayOptions = m_userDbPtr.session()->add( new ModelDisplayOptions() );
  displayOptions.modify()->usermodel = newModelPtr;

  if( m_bsBeginTimePicker && m_bsEndTimePicker )
  {
    displayOptions.modify()->displayBegin = m_bsBeginTimePicker->dateTime();
    displayOptions.modify()->displayEnd = m_bsEndTimePicker->dateTime();
  }//if( m_bsBeginTimePicker && m_bsEndTimePicker )

  if( !displayOptions->displayBegin.isValid()
      || !displayOptions->displayEnd.isValid()
      || (displayOptions->displayBegin == displayOptions->displayEnd) )
  {
    NLSimplePtr modelPtr( this, false, SRC_LOCATION );
    if( modelPtr )
    {
      displayOptions.modify()->displayBegin = WDateTime::fromPosixTime( modelPtr->dataGraph(NLSimple::kCgmsData).getStartTime() );
      displayOptions.modify()->displayEnd = WDateTime::fromPosixTime( modelPtr->dataGraph(NLSimple::kCgmsData).getEndTime() );
    }//if( modelPtr )
  }//if( we dont have valid times )


  if( newModelPtr )
    cerr << "Added new model:\n  "
         << "newModel->user->name='" << newModelPtr->user->name
         << "', newModel->fileName='" << newModelPtr->fileName
         << "', newModel->created='" << newModelPtr->created.toPosixTime() << "'"
         << endl;

  transaction.commit();

  setModelFileName( fileName );
}//void saveModel( const std::string &fileName )


void WtGui::deleteModelFile( const std::string &fileName )
{
  if( fileName.empty() || !m_userDbPtr.session() || !m_userDbPtr.session() )
    return;

  Dbo::ptr<UsersModel> usrmodel;

  {
    Dbo::Transaction transaction( *(m_userDbPtr.session()) );
    const DubUser::UsersModels &models = m_userDbPtr->models;
    usrmodel = models.find().where( "fileName = ?" ).bind( fileName );
    if( usrmodel )
      usrmodel.remove();
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

  if( !m_userDbPtr || !m_userDbPtr.session() )
    return;

  if( !lock.owns_lock() )
  {
    const string msg = "Another operation (thread) is currently working, sorry I cant do this operation of setModel( string ): "
                       + string( SRC_LOCATION );
    wApp->doJavaScript( "alert( '" + msg + "' );", true );
    cerr << msg << endl;
    return;
  }//if( couldn't get the lock )

  m_nlSimleDisplayModel->aboutToSetNewModel();

  Dbo::ptr<UsersModel> usrmodel;
  {
    Dbo::Transaction transaction( *(m_userDbPtr.session()) );
    const DubUser::UsersModels &models = m_userDbPtr->models;
    usrmodel = models.find().where( "fileName = ?" ).bind( fileName );
    transaction.commit();
  }

  if( usrmodel && !usrmodel->serializedData.empty() )
  {
    try
    {
      m_model = NLSimpleShrdPtr( new NLSimple( "description", 0.0, ProgramOptions::kBasalGlucConc, kGenericT0 ) );
      stringstream inputstream( usrmodel->serializedData );
      boost::archive::text_iarchive ia( inputstream );
      ia >> (*m_model);
      setModelFileName( fileName );
      cerr << "\n\nSuccessgully got the NLSimple named "
           << fileName << " From the database" << endl;
      m_nlSimleDisplayModel->doneSettingNewModel();
      return;
    }catch(...)
    {
      wApp->doJavaScript( "alert( 'Failed to open NLSimple named " + fileName + " from the database' );", true );
      return;
//      deleteModelFile( fileName );
    }
  }//if( usrmodel && !usrmodel->serializedData.empty() )

  try
  {
    m_model = NLSimpleShrdPtr( new NLSimple( formFileSystemName(fileName) ) );
    setModelFileName( fileName );
  }catch(...){
    cerr << "Failed to open NLSimple named " << fileName << endl;
    wApp->doJavaScript( "alert( \"Failed to open NLSimple named " + fileName + "\" );", true );
    deleteModelFile( fileName );
  }//try/catch

  m_nlSimleDisplayModel->doneSettingNewModel();
}//void setModel( const std::string &fileName )


void WtGui::setModelFileName( const std::string &fileName )
{
  if( !m_userDbPtr || !m_userDbPtr.session() )
    return;

  if( m_userDbPtr->currentFileName == fileName )
    return;

  Dbo::Transaction transaction( *(m_userDbPtr.session()) );
  m_userDbPtr.modify()->currentFileName = fileName;
  if( !transaction.commit() )
    cerr << "setModelFileName( const string & ) failed commit" << endl;
}//void setModelFileName( const std::string &fileName )


void WtGui::tabClickedCallback( int clickedIndex )
{
  const int overviewTabIndex = m_tabs->indexOf( m_overviewTab );
  const int notesTabIndex = m_tabs->indexOf( m_notesTab );
  const int exludeDataIndex = m_tabs->indexOf( m_excludeTimeRangeTab );

  if( clickedIndex != notesTabIndex )
    m_notesTab->saveCurrent();

  if( clickedIndex == overviewTabIndex )
    m_bsGraphOverlay->show();
  else
    m_bsGraphOverlay->hide();

  if( clickedIndex == exludeDataIndex )
    m_excludeTimeRangeTab->showChartOverlay();
  else
    m_excludeTimeRangeTab->hideChartOverlay();

}//void tabClickedCallback(int);






DubEventEntry::DubEventEntry( const bool isMobile, WtGui *wtguiparent, WContainerWidget *parent )
  : WContainerWidget( parent ),
  m_time( new DateTimeSelect( "&nbsp;&nbsp;Date/Time:&nbsp;",
                              WDateTime::fromPosixTime(boost::posix_time::second_clock::local_time())
                            ) ),
  m_type( new WComboBox() ),
  m_customTypes( new WComboBox() ),
  m_value( new WLineEdit() ),
  m_units( new WText() ),
  m_button( new WPushButton("submit") ),
  m_saveModel( new WCheckBox( "Save Model&nbsp;" ) ),
  m_wtgui( wtguiparent )
{
  if( isMobile )
    setStyleClass( "DubEventEntryMobile" );
  else
    setStyleClass( "DubEventEntry" );

  setInline(false);

  Div *timeSelectDiv = new Div( "DubEventEntry_timeSelectDiv" );
  timeSelectDiv->setInline(true);

  timeSelectDiv->addWidget( m_time );
  WPushButton *nowB = new WPushButton( "now", timeSelectDiv );
  WPushButton *lastTimeB = new WPushButton( "data end", timeSelectDiv );
  nowB->setStyleClass( "dubEventEntryTimeButton" );
  lastTimeB->setStyleClass( "dubEventEntryTimeButton" );
  nowB->clicked().connect( m_time, &DateTimeSelect::setToCurrentTime );

  lastTimeB->clicked().connect( this, &DubEventEntry::setTimeToLastData );

  m_saveModel->setCheckState( Checked );
  m_saveModel->hide();


  for( WtGui::EntryType et = WtGui::EntryType(0);
       et < WtGui::kNumEntryType;
       et = WtGui::EntryType( et+1 ) )
  {
    switch( et )
    {
      case WtGui::kNotSelected:      m_type->addItem( "Event Type" );   break;
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

  if( !isMobile )
  {
    addWidget( new WText( "<b>Enter New Event:</b>&nbsp;&nbsp;" ) );
    addWidget( timeSelectDiv );
    addWidget( m_type );
    addWidget( m_value );
    addWidget( m_units );
    addWidget( m_customTypes );
    addWidget( new WText( "&nbsp;&nbsp;" ) );
    m_saveModel->setStyleClass( "DubEventEntry_m_saveModel" );
    addWidget( m_saveModel );
    addWidget( m_button );
  }else
  {
    WGridLayout *layout = new WGridLayout();
    setLayout( layout );
    layout->addWidget( new WText( "<b>Enter New Event:</b>" ), 0, 0, 1, 4 );

    layout->addWidget( new WLabel( "Event Type" ), 1, 0, 1, 1 );
    layout->addWidget( m_type, 1, 1, 1, 3, AlignLeft );

    layout->addWidget( new WLabel( "Date/Time" ), 2, 0, 1, 1 );
    layout->addWidget( timeSelectDiv, 2, 1, 1, 3 );

    layout->addWidget( new WLabel( "Value" ), 3, 0, 1, 1, AlignTop );
    layout->addWidget( m_value,               3, 1, 1, 1, AlignTop );
    layout->addWidget( m_units,               3, 2, 1, 1, AlignTop | AlignLeft );
    layout->addWidget( m_customTypes,         3, 3, 1, 1, AlignTop | AlignLeft );

    layout->addWidget( m_button, 4, 0, 1, 2, AlignLeft | AlignTop );
    layout->addWidget( m_saveModel, 4, 2, 1, 2, AlignLeft | AlignTop );
  }//if( !isMobile ) / else


  m_customTypes->setHidden( true );
  m_value->enterPressed().connect( boost::bind( &DubEventEntry::emitEntered, this ) );
  m_type->changed().connect( boost::bind( &DubEventEntry::typeChanged, this ) );
  m_button->clicked().connect( boost::bind( &DubEventEntry::emitEntered, this ) );

  typeChanged();
}//DubEventEntry(...)

DubEventEntry::~DubEventEntry(){}

void DubEventEntry::emitEntered()
{
  if( m_type->currentIndex() == WtGui::kNotSelected ) return;

  WtGui::EventInformation info;
  info.dateTime = m_time->dateTime();
  info.type = WtGui::EntryType( m_type->currentIndex() );

  if( m_type->currentIndex() != WtGui::kCustomEventData )
  {
    try
    {
      info.value = boost::lexical_cast<double>( m_value->text().narrow() );
    }catch(...)
    {
      cerr << "Warning failed lexical_cast<double> in emitEntered" << endl;
      return;
    }//try/catch
  } else
  {
    bool found = false;
    NLSimplePtr modelPtr( m_wtgui );
    const NLSimple::EventDefMap &customEvents = modelPtr->m_customEventDefs;
    foreach( const NLSimple::EventDefMap::value_type &p, customEvents )
    {
      if( m_customTypes->currentText() == p.second.getName() )
      {
        found = true;
        info.value = static_cast<double>( p.first );
        break;
      }//if( m_customTypes->currentText() == p->second.getName() )
    }//foreach possible custom event type
    if( !found )
    {
      cerr << "void DubEventEntry::emitEntered(): check function logic (will blissfully continue for now" << endl;
      return;
    }//if( !found )
  }//if( m_type->currentIndex() != WtGui::kCustomEventData ) / else

  reset();

  m_signal.emit(info);

  if( m_saveModel->isChecked() ) m_wtgui->saveCurrentModel();
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
  m_units->setHidden(false);
  m_value->setHidden(false);
  m_customTypes->setHidden(true);
}//void DubEventEntry::reset()



void DubEventEntry::setTimeToLastData()
{
  if( !m_wtgui ) return;
  m_time->set( m_wtgui->getEndTimePicker()->top() );

}//void setTimeToLastData()


void DubEventEntry::typeChanged()
{
  m_value->setEnabled(true);
  m_button->setEnabled(true);
  m_units->setHidden(false);
  m_value->setHidden(false);
  m_customTypes->setHidden(true);

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
      m_value->setFocus();
    break;
    case WtGui::kMeterReading:
      m_units->setText("mg/dL");
      m_value->setValidator( new WIntValidator(0,500) );
      m_value->setFocus();
    break;
    case WtGui::kMeterCalibration:
      m_units->setText("mg/dL");
      m_value->setValidator( new WIntValidator(0,500) );
      m_value->setFocus();
    break;
    case WtGui::kGlucoseEaten:
      m_units->setText("grams");
      m_value->setValidator( new WIntValidator(0,150) );
      m_value->setFocus();
    break;
    case WtGui::kBolusTaken:
      m_units->setText("units");
      m_value->setValidator( new WDoubleValidator(0,50) );
      m_value->setFocus();
    break;
    case WtGui::kGenericEvent:
    {
      m_customTypes->setHidden(false);
      m_units->setHidden(true);
      m_value->setHidden(true);
      m_units->setText("");
      m_value->setText( "NA" );
      m_value->setEnabled(false);
      m_customTypes->clear();
      NLSimplePtr modelPtr( m_wtgui );
      const NLSimple::EventDefMap &customEvents = modelPtr->m_customEventDefs;
      foreach( const NLSimple::EventDefMap::value_type &p, customEvents )
        m_customTypes->addItem( p.second.getName() );
      break;
      m_customTypes->setFocus();
    };//

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


ModelSettingsTab::ModelSettingsTab( ModelSettings *modelSettings,
                                        WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_settings( modelSettings )
{
  setStyleClass( "ModelSettingsTab" );
  init();
}

ModelSettingsTab::~ModelSettingsTab() {}
Signal<> &ModelSettingsTab::changed() { return m_changed; }
Signal<> &ModelSettingsTab::predictionChanged() { return m_predictionChanged; }
void ModelSettingsTab::emitChanged() { m_changed.emit(); }
void ModelSettingsTab::emitPredictionChanged() { m_predictionChanged.emit(); }

void ModelSettingsTab::init()
{
  clear();
  if( !m_settings ) return;

  WGridLayout *layout = new WGridLayout();
  setLayout( layout, Wt::AlignTop | Wt::AlignLeft );

  const int maxRows = 8;
  int row = 0, column = 0;

  MemVariableSpinBox *sb = new DoubleSpinBox( &(m_settings->m_personsWeight), "", "kg", 0, 250 );
  layout->addWidget( new WText("Your Weight"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_cgmsIndivReadingUncert), "", "", 0, 1 );
  layout->addWidget( new WText("Ind. CGMS Uncert."), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_defaultCgmsDelay), "", -15, 35 );
  layout->addWidget( new WText("Default CGMS Delay"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_cgmsDelay), "", -15, 35 );
  layout->addWidget( new WText("Actual CGMS Delay"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_predictAhead), "", 0, 180 );
  layout->addWidget( new WText("Amount to Predict Ahead"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new TimeDurationSpinBox( &(m_settings->m_dt), "", 1.0/60.0, 30 );
  layout->addWidget( new WText("Integration delta"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_lastPredictionWeight), "", "", 0, 1 );
  layout->addWidget( new WText("Last Prediction Weight"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_targetBG), "", "mg/dL", 20, 250 );
  layout->addWidget( new WText("Target Blood Glucose"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_bgLowSigma), "", "mg/dL", 0, 50 );
  layout->addWidget( new WText("Low BG 1&sigma; Tolerance", XHTMLUnsafeText), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new DoubleSpinBox( &(m_settings->m_bgHighSigma), "", "mg/dL", 0, 100 );
  layout->addWidget( new WText("High BG 1&sigma; Tolerance", XHTMLUnsafeText), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genPopSize), "", "Indivuduals", 2, 5000 );
  layout->addWidget( new WText("Gen. Pop. Size"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genConvergNsteps), "", "steps", 1, 200 );
  layout->addWidget( new WText("Gen. Conv. N-steps"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genNStepMutate), "", "steps", 1, 50 );
  layout->addWidget( new WText("Gen. N-step Mutate"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

  sb = new IntSpinBox( &(m_settings->m_genNStepImprove), "", "steps", 1, 50 );
  layout->addWidget( new WText("Gen. N-step Improve"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;


  sb = new DoubleSpinBox( &(m_settings->m_genSigmaMult), "", "", 0.0, 20 );
  layout->addWidget( new WText("Gen. Mutate Sigma"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;


  sb = new DoubleSpinBox( &(m_settings->m_genConvergCriteria), "", "", 0.0, 20 );
  layout->addWidget( new WText("Gen. Conv. Criteria"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;


  sb = new IntSpinBox( &(m_settings->m_minFingerStickForCharacterization), "", "", 0.0, 5000 );
  layout->addWidget( new WText("Min FS 4 CGMS Charac"), row, column, 1, 1, AlignRight );
  layout->addWidget( sb, row, column+1, 1, 1, AlignLeft );
  m_memVarSpinBox.push_back( sb );
  sb->valueChanged().connect( this, &ModelSettingsTab::emitChanged );
  //sb->valueChanged().connect( this, &ModelSettingsTab::emitPredictionChanged );
  row    = (row<maxRows) ? row + 1 : 0;
  column = (row==0) ? column + 2: column;

/*
  PosixTime m_endTrainingTime;
  PosixTime m_startTrainingTime;
*/
}//void init()


GeneticallyOptimizeTab::GeneticallyOptimizeTab( WtGui *wtGuiParent, Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_parentWtGui( wtGuiParent )
{
  setInline(false);
  setStyleClass( "GeneticallyOptimizeTab" );

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
//  m_graphModel->setHeaderData( WtGui::kCustomEventData,       WString("User Defined Events") );
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
  m_graph->setMinimumSize( 200, 150 );


  Chart::WDataSeries cgmsSeries(WtGui::kCgmsData, Chart::LineSeries);
//  cgmsSeries.setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
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

//  m_chi2Model = new WStandardItemModel( this );
//  m_chi2Model->insertColumns( m_chi2Model->columnCount(), 2 );
//  m_chi2Model->setHeaderData( 0, WString("Generation Number") );
//  m_chi2Model->setHeaderData( 1, WString("Best &chi;<sup>2</sup>") );


  m_chi2DbModel = new Dbo::QueryModel< Dbo::ptr<OptimizationChi2> >(this);

  {
    Dbo::ptr<DubUser> user = m_parentWtGui->dubUserPtr();

    if( user && user.session() )
    {
      Dbo::Transaction transaction( *(user.session()) );

      const string fileName = user->currentFileName;
      Dbo::ptr<UsersModel> usermodel = user->models.find().where("fileName = ?").bind(fileName);

      if( usermodel )
      {
        m_chi2DbModel->setQuery( usermodel->chi2s.find() );
        m_chi2DbModel->addColumn( "generation", "Generation" );
        m_chi2DbModel->addColumn( "chi2", "chi2" );
      }
      if( !transaction.commit() )
        cerr << "\nDid not commit adding to the chi2 of the optimization\n" << endl;
    }//if( user && user.session() )
  }



  m_chi2Graph = new Chart::WCartesianChart(Chart::ScatterPlot);
//  m_chi2Graph->setModel( m_chi2Model );
  m_chi2Graph->setModel( m_chi2DbModel );
  m_chi2Graph->setXSeriesColumn(0);
  m_chi2Graph->setLegendEnabled(false);
  m_chi2Graph->setPlotAreaPadding( 70, Wt::Bottom );
  m_chi2Graph->axis(Chart::YAxis).setTitle( L"Best \x03C7\x00B2" );
  m_chi2Graph->setMinimumSize( 200, 150 );
  m_chi2Graph->axis(Chart::XAxis).setTitle( "Generation Number" );
  m_chi2Graph->addSeries( Chart::WDataSeries( 1, Chart::LineSeries ) );

  if( m_chi2DbModel->rowCount() < 2 )
  {
    setChi2XRange( 0.0, 1.0 );
    setChi2YRange( 0.0, 10000.0 );
  }


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
  m_parentWtGui->getBeginTimePicker()->topBottomUpdated().connect( this, &GeneticallyOptimizeTab::updateDateSelectLimits );

  m_startTrainingTimeSelect->changed().connect( this, &GeneticallyOptimizeTab::syncGraphDataToNLSimple );
  m_endTrainingTimeSelect->changed().connect( this, &GeneticallyOptimizeTab::syncGraphDataToNLSimple );

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
  m_startOptimization->clicked().connect( this, &GeneticallyOptimizeTab::startOptimization );

  m_stopOptimization = new WPushButton( "Stop Genetic Optimization", buttonDiv );
  m_stopOptimization->hide();
  m_stopOptimization->clicked().connect( boost::bind( &GeneticallyOptimizeTab::setContinueOptimizing, this, false ) );

  m_minuit2Optimization = new WPushButton( "Simplex Fine Tune", buttonDiv );
  m_minuit2Optimization->clicked().connect( this, &GeneticallyOptimizeTab::startMinuit2Optimization );

  syncGraphDataToNLSimple();
}//GeneticallyOptimizeTab


GeneticallyOptimizeTab::~GeneticallyOptimizeTab()
{
  if( m_currentOptThread )
  {
    cerr << SRC_LOCATION
         << "\n\tWarning, m_currentOptThread is still going!!" << endl;
//    m_parentWtGui->
    m_currentOptThread->join();
  }//
}//~GeneticallyOptimizeTab()


void GeneticallyOptimizeTab::updateDateSelectLimits()
{
  const WDateTime &top = m_parentWtGui->getBeginTimePicker()->top();
  const WDateTime &bottom = m_parentWtGui->getBeginTimePicker()->bottom();

  m_startTrainingTimeSelect->setTop( top );
  m_startTrainingTimeSelect->setBottom( bottom );
  m_endTrainingTimeSelect->setTop( top );
  m_endTrainingTimeSelect->setBottom( bottom );
}//void updateDateSelectLimits()

void GeneticallyOptimizeTab::startMinuit2Optimization()
{
  if( m_currentOptThread )
  {
    WApplication::UpdateLock appLock( m_parentWtGui->app() );
    m_parentWtGui->app()->doJavaScript( "alert( 'Currently bussy performing another optimization, sorry :(' );", false );
    return;
  }//if( m_currentOptThread )

  boost::function<void(void)> worker;
  worker = boost::bind( &GeneticallyOptimizeTab::doMinuit2Optimization, this );
//  WServer::instance()->post( m_parentWtGui->app()->sessionId(), worker );
  m_currentOptThread.reset( new boost::thread( worker ) );
}//void GeneticallyOptimizeTab::startMinuit2Optimization()



void GeneticallyOptimizeTab::doMinuit2Optimization()
{
  WText *text = NULL;
  TimeRangeVec timeRange;

  {
    NLSimplePtr model( m_parentWtGui, false,
                       "Failed to get thread lock for Minuit2 minimization. "
                       "Are you trying to optimize the same model twice at the "
                       "same time?"
                       + string(SRC_LOCATION) );
    if( !model )
      return;

    WApplication::UpdateLock appLock( m_parentWtGui->app() );
    m_startOptimization->hide();
    text = new WText( "<font color=\"blue\"><b>Currently Optimizing</b></font>", XHTMLUnsafeText );
    m_layout->addWidget( text, WBorderLayout::North );

    PosixTime startTime = m_startTrainingTimeSelect->currentValue();
    PosixTime endTime = m_endTrainingTimeSelect->currentValue();

    startTime = std::max( startTime, model->m_cgmsData.getStartTime() );
    endTime = std::min( endTime, model->m_cgmsData.getEndTime() );

    timeRange.push_back( TimeRange(startTime, endTime) );

    m_parentWtGui->app()->triggerUpdate();
  }//


  try
  {
    GeneticEvalUtils::perform_migrad_optimization( m_parentWtGui, timeRange );
//  model->fitModelToDataViaMinuit2( model->m_settings.m_lastPredictionWeight );
  }catch( std::exception &e )
  {
    WApplication::UpdateLock appLock( m_parentWtGui->app() );
    doJavaScript( "alert( 'Optimization failed due to: " + string(e.what()) + "');" );
    m_parentWtGui->app()->triggerUpdate();
  }//try / catch

  {
    WApplication::UpdateLock appLock( m_parentWtGui->app() );

    m_parentWtGui->syncDisplayToModel();
    m_parentWtGui->updateClarkAnalysis();

    m_startOptimization->show();
    m_layout->removeWidget( text );
    m_parentWtGui->app()->triggerUpdate();

    boost::function<void(void)> worker;
    worker = boost::bind( &GeneticallyOptimizeTab::optimizationFinished, this );
    WServer::instance()->post( m_parentWtGui->app()->sessionId(), worker );
  }//
}//void GeneticallyOptimizeTab::doMinuit2Optimization()


void GeneticallyOptimizeTab::optimizationFinished()
{
  if( !m_currentOptThread )
  {
    cerr << SRC_LOCATION << "\n\t!m_currentOptThread" << endl;
    return;
  }


  WApplication::UpdateLock appLock( m_parentWtGui->app() );

  m_currentOptThread->join();
  m_currentOptThread.reset();
}//void optimizationFinished()

void GeneticallyOptimizeTab::setChi2XRangeAuto()
{
  m_chi2Graph->axis(Chart::XAxis).setAutoLimits( Chart::MinimumValue | Chart::MaximumValue );
}

void GeneticallyOptimizeTab::setChi2YRangeAuto()
{
  m_chi2Graph->axis(Chart::YAxis).setAutoLimits( Chart::MinimumValue | Chart::MaximumValue );
}


void GeneticallyOptimizeTab::setChi2XRange( double ymin, double ymax )
{
  if( ymin > ymax )
    swap( ymin, ymax );

  if( fabs(ymin-ymax) < 0.01 )
    ymax += 1.0;

  m_chi2Graph->axis(Chart::XAxis).setRange( ymin, ymax );
}//void setChi2XRange( double ymin, double ymax )

void GeneticallyOptimizeTab::setChi2YRange( double ymin, double ymax )
{
  if( ymin > ymax )
    swap( ymin, ymax );

  if( fabs(ymin-ymax) < 0.01 )
    ymax += 1.0;

  m_chi2Graph->axis(Chart::YAxis).setRange( ymin, ymax );
}//setChi2YRange( double ymin, double ymax )

void GeneticallyOptimizeTab::startOptimization()
{
  if( m_currentOptThread )
  {
    WApplication::UpdateLock appLock( m_parentWtGui->app() );
    m_parentWtGui->app()->doJavaScript( "alert( 'Currently bussy performing another optimization, sorry :(' );", false );
    return;
  }//if( m_currentOptThread )

  boost::function<void(void)> worker;
  worker = boost::bind( &GeneticallyOptimizeTab::doGeneticOptimization, this );

//  cerr << "Launching optimization in a new thread" << endl;
//  WServer::instance()->post( m_parentWtGui->app()->sessionId(),
//                             boost::bind( &GeneticallyOptimizeTab::doGeneticOptimization, this ) );

  m_currentOptThread.reset( new boost::thread( worker ) );
}//void GeneticallyOptimizeTab::startOptimization()


void GeneticallyOptimizeTab::doGeneticOptimization()
{
  //std::vector<Wt::WWidget *> m_disableWhenBusyItems;
  //foreach( WWidget *w, m_disableWhenBusyItems ) w->setDisabled(true);

//  m_parentWtGui->app()->attachThread(true);

  setContinueOptimizing( true );

  WText *text = NULL;
  TimeRangeVec timeRange;

  {
    NLSimplePtr model( m_parentWtGui, false,
                       "Failed to get thread lock for genetic minimization. "
                       "Are you trying to optimize the same model twice at the "
                       "same time?"
                       + string( SRC_LOCATION ) );
    if( !model )
      return;

    WApplication::UpdateLock appLock( m_parentWtGui->app() );
    //if( appLock )

    PosixTime startTime = m_startTrainingTimeSelect->currentValue();
    PosixTime endTime = m_endTrainingTimeSelect->currentValue();

    startTime = std::max( startTime, model->m_cgmsData.getStartTime() );
    endTime = std::min( endTime, model->m_cgmsData.getEndTime() );

    timeRange.push_back( TimeRange(startTime, endTime) );

    m_stopOptimization->show();  //TODO: figure out if we didn't get appLock, will the changes be propogated to user eventually?
    m_startOptimization->hide();
    text = new WText( "<font color=\"blue\"><b>Currently Optimizing</b></font>", XHTMLUnsafeText );
    m_layout->addWidget( text, WBorderLayout::North );
//    if( appLock )
    m_parentWtGui->app()->triggerUpdate();
  }//

//  m_bestChi2.clear();
  {
    Dbo::ptr<DubUser> user = m_parentWtGui->dubUserPtr();

    if( user && user.session() )
    {
      Dbo::Transaction transaction( *(user.session()) );

      const string fileName = user->currentFileName;
      Dbo::ptr<UsersModel> usermodel = user->models.find().where("fileName = ?").bind(fileName);

      if( usermodel )
      {
        typedef Wt::Dbo::collection<Wt::Dbo::ptr<OptimizationChi2> > Chi2s;
        Chi2s chi2s = usermodel->chi2s;
        for( Chi2s::iterator iter = chi2s.begin(); iter != chi2s.end(); ++iter )
          iter->remove();
      }
      if( !transaction.commit() )
        cerr << "\nDid not commit adding to the chi2 of the optimization\n" << endl;
    }//if( user && user.session() )
  }

  boost::function<void(double)> chi2Calback = boost::bind( &GeneticallyOptimizeTab::optimizationUpdateFcn, this, _1);
  boost::function<bool(void)> continueFcn = boost::bind( &GeneticallyOptimizeTab::continueOptimizing, this);


  cerr << "about to do the workptr" << endl;
  try
  {
    GeneticEvalUtils::perform_genetic_optimization( m_parentWtGui,
                                           timeRange, chi2Calback, continueFcn );

//    model->geneticallyOptimizeModel( model->m_settings.m_lastPredictionWeight,
//                                     timeRange, chi2Calback, continueFcn );
  }catch( exception &e )
  {
    string msg = "Warning: Optimization failed:\n";
    msg += e.what();
    m_parentWtGui->app()->doJavaScript( "alert( \"" + msg + "\" );", false );
    cerr << msg << endl;
  }catch(...)
  {
    string msg = "Warning: Optimization failed";
    m_parentWtGui->app()->doJavaScript( "alert( '" + msg + "' );", false );
    cerr << msg << endl;
  }//try / catch

  //just to make sure we don't loose all this work
  const std::string &fileName = m_parentWtGui->currentFileName();
  if( fileName != "" )
    m_parentWtGui->saveCurrentModel();

  {
    WApplication::UpdateLock appLock( m_parentWtGui->app() );
    m_parentWtGui->syncDisplayToModel();
    m_parentWtGui->updateClarkAnalysis();

    m_stopOptimization->hide();
    m_startOptimization->show();
    m_layout->removeWidget( text );
    m_parentWtGui->app()->triggerUpdate();
    WServer::instance()->post( m_parentWtGui->app()->sessionId(),
                                 boost::bind( &GeneticallyOptimizeTab::optimizationFinished, this ) );
  }//

  //foreach( WWidget *w, m_disableWhenBusyItems ) w->setDisabled(false);

}//void doGeneticOptimization()


void GeneticallyOptimizeTab::setContinueOptimizing( const bool doContinue )
{
  boost::mutex::scoped_lock lock( m_continueMutex );
  m_continueOptimizing = doContinue;
}//void setContinueOptimizing( const bool doContinue )


bool GeneticallyOptimizeTab::continueOptimizing()
{
  boost::mutex::scoped_lock lock( m_continueMutex );
  return m_continueOptimizing;
}//bool continueOptimizing()


void GeneticallyOptimizeTab::optimizationUpdateFcn( double chi2 )
{
  WApplication::UpdateLock lock( m_parentWtGui->app() );
  if( !lock )
  {
    cerr << "Couldn't get WApplication lock!!!" << endl;
    m_parentWtGui->doJavaScript( "alert( 'Failed to get WApplication lock in GeneticallyOptimizeTab::optimizationUpdateFcn(double)' );" );
    return;
  }//if( !lock )

//  m_bestChi2.push_back( chi2 );

  {
    Dbo::ptr<DubUser> user = m_parentWtGui->dubUserPtr();
    if( user && user.session() )
    {
      Dbo::Transaction transaction( *(user.session()) );

      const string fileName = user->currentFileName;
      Dbo::ptr<UsersModel> usermodel = user->models.find().where("fileName = ?").bind(fileName);
      Dbo::ptr<OptimizationChi2> newChi2;
      newChi2 = user.session()->add( new OptimizationChi2() );
      newChi2.modify()->generation  = static_cast<int>(usermodel->chi2s.size());
      newChi2.modify()->chi2        = chi2;
      newChi2.modify()->usermodel   = usermodel;
      if( !transaction.commit() )
        cerr << "\nDid not commit adding to the chi2 of the optimization\n" << endl;
    }//if( user && user.session() )
  }

  syncGraphDataToNLSimple();

  if( m_saveAfterEachGeneration->isChecked() )
  {
   const std::string &fileName = m_parentWtGui->currentFileName();
   if( fileName != "" )
     m_parentWtGui->saveCurrentModel();
  }//if( we want to save the model )

  // Push the changes to the browser
  m_parentWtGui->app()->triggerUpdate();
}//void optimizationUpdateFcn( double chi2 )



void GeneticallyOptimizeTab::syncGraphDataToNLSimple()
{
  typedef ConsentrationGraph::value_type cg_type;
  WApplication::UpdateLock appLock( m_parentWtGui->app() );

  Dbo::ptr<DubUser> user = m_parentWtGui->dubUserPtr();
  if( user && user.session() )
  {
    Dbo::Transaction transaction( *(user.session()) );

    m_chi2DbModel->reload();
    transaction.commit();
  }//if( user && user.session() )



//  m_chi2Model->removeRows( 0, m_chi2Model->rowCount() );
//  m_chi2Model->insertRows( 0, m_bestChi2.size() );
//  for( size_t i = 0; i < m_bestChi2.size(); ++i )
//  {
//    m_chi2Model->setData( i, 0, i );
//    m_chi2Model->setData( i, 1, m_bestChi2[i] );
//  }//for( add chi data to the model )

  const string error_msg = "Failed to get thread lock for syncGraphDataToNLSimple."
                     " Are you optimizing in another thread?: "
                     + string( SRC_LOCATION );

  NLSimplePtr modelPtr( m_parentWtGui, false, error_msg );
  if( !modelPtr ) return;



  const WDateTime start = m_startTrainingTimeSelect->dateTime();
  const WDateTime end = m_endTrainingTimeSelect->dateTime();

  const size_t nNeededRow = modelPtr->m_cgmsData.size()
                            //+ modelPtr->m_fingerMeterData.size()
                            + modelPtr->m_mealData.size()
                            + modelPtr->m_predictedBloodGlucose.size()
                            //+ modelPtr->m_glucoseAbsorbtionRate.size()
                            + modelPtr->m_freePlasmaInsulin.size()
                            + modelPtr->m_customEvents.size()
                            + modelPtr->m_predictedInsulinX.size();

  m_graphModel->removeRows( 0, m_graphModel->rowCount() );
  m_graphModel->insertRows( 0, static_cast<int>(nNeededRow) );


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

  PosixTime lastOne = modelPtr->m_freePlasmaInsulin.size() ? modelPtr->m_freePlasmaInsulin.getStartTime() : PosixTime(boost::posix_time::min_date_time);

  foreach( const cg_type &element, modelPtr->m_freePlasmaInsulin )
  {
    if( (element.m_time-lastOne) < NLSimpleDisplayModel::sm_plasmaInsulinDt ) continue;
    lastOne = element.m_time;

    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kFreePlasmaInsulin, element.m_value );
  }//


  foreach( const cg_type &element, modelPtr->m_glucoseAbsorbtionRate )
  {
    if( (element.m_time-lastOne) < NLSimpleDisplayModel::sm_plasmaInsulinDt ) continue;
    lastOne = element.m_time;

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
    if( (element.m_time-lastOne) < NLSimpleDisplayModel::sm_plasmaInsulinDt ) continue;
    lastOne = element.m_time;

    const WDateTime x = WDateTime::fromPosixTime( element.m_time );
    if( (x < start) || (x > end) ) continue;
    m_graphModel->setData( row, WtGui::kTimeData, x );
    m_graphModel->setData( row++, WtGui::kPredictedInsulinX, element.m_value );
  }//
}//void syncGraphDataToNLSimple()





CustomEventTab::CustomEventTab( WtGui *wtGuiParent,
                                    Wt::WContainerWidget *parent )
  : WContainerWidget( parent ), m_parentWtGui( wtGuiParent ),
  m_layout( new WBorderLayout(this) ),
  m_currentCE(-1),
  m_currentCEModel( new WtGeneralArrayModel(0, NULL, NULL, this) ),
  m_currentCEChart( new Chart::WCartesianChart(Wt::Chart::ScatterPlot,parent)),
  m_eventTypesView( new WTableView() ),
  m_eventTypesModel( new WStandardItemModel(this) )
{
  m_eventTypesModel->insertColumns( 0, 2 );
  m_eventTypesModel->setHeaderData( 0, Horizontal, WString("ID"), DisplayRole );
  m_eventTypesModel->setHeaderData( 1, Horizontal, WString("Name"), DisplayRole );
  m_eventTypesView->resize( WLength( 40, WLength::FontEx ),
                            WLength( 12, WLength::FontEm ) );
  m_eventTypesView->setSortingEnabled( true );
  m_eventTypesView->setSelectionMode( SingleSelection );
  m_eventTypesView->setAlternatingRowColors( true );
  m_eventTypesView->setSelectionBehavior( SelectRows );
  m_eventTypesView->setColumnWidth( 0, WLength(5, WLength::FontEx) );
  m_eventTypesView->setColumnWidth( 1, WLength(24, WLength::FontEx) );
  m_eventTypesView->setModel( m_eventTypesModel );
  m_eventTypesView->selectionChanged().connect( this, &CustomEventTab::displaySelectedModel );
  WPanel *panel = new WPanel();
  panel->setTitle( "Defined Custom Events:" );
  panel->setCentralWidget( m_eventTypesView );


  Div *eastDiv = new Div();
  eastDiv->addWidget( panel );
  Div *buttonDiv = new Div( "CustomEventTab_buttonDiv", eastDiv );
  WPushButton *addModelButton = new WPushButton( "Add Event Type", buttonDiv );
  WPushButton *delModelButton = new WPushButton( "Delete Event Type", buttonDiv );
  new WBreak( buttonDiv );
  WPushButton *defDexButton = new WPushButton( "Add Default Dexcom Events", buttonDiv );

  addModelButton->clicked().connect( this, &CustomEventTab::addCustomEventDialog );
  delModelButton->clicked().connect( boost::bind( &CustomEventTab::confirmUndefineCustomEventDialog, this ) );
  defDexButton->clicked().connect( boost::bind( &CustomEventTab::defineDefaultDexcomEvents, this ) );

  WText *headerText = new WText( "Response Functions of Defined Events" );

  m_currentCEChart->setMinimumSize( 200, 150 );
  m_currentCEChart->setModel( m_currentCEModel );
  m_currentCEChart->setXSeriesColumn(0);
  m_currentCEChart->setLegendEnabled(false);
  m_currentCEChart->setPlotAreaPadding( 25, Wt::Right );
  m_currentCEChart->setPlotAreaPadding( 70, Wt::Bottom );
  //m_bsGraph->axis(Chart::XAxis).setScale(Chart::DateTimeScale);
  m_currentCEChart->axis(Chart::XAxis).setTitle( "Minutes After Beginning" );
  Chart::WDataSeries series( 1, Chart::LineSeries);
//  series.setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
  m_currentCEChart->addSeries( series );

  if( m_eventTypesModel->rowCount() )
  {
    std::set<WModelIndex> indices;
    indices.insert( m_eventTypesModel->index(0,0) );
    indices.insert( m_eventTypesModel->index(0,1) );
    m_eventTypesView->setSelectedIndexes( indices );
  }//if( m_eventTypesModel->rowCount() )

  m_layout->addWidget( eastDiv, WBorderLayout::East );
  m_layout->addWidget( m_currentCEChart, WBorderLayout::Center );
  m_layout->addWidget( headerText, WBorderLayout::North );

  updateAvailableEventTypes();

  displaySelectedModel();
}//CustomEventTab constructor

CustomEventTab::~CustomEventTab(){};

void CustomEventTab::updateAvailableEventTypes()
{
  NLSimplePtr nlsimpleptr( m_parentWtGui );
  const NLSimple::EventDefMap &eventDefMap = nlsimpleptr->m_customEventDefs;

  m_eventTypesModel->removeRows( 0, m_eventTypesModel->rowCount() );
  m_eventTypesModel->insertRows( 0, eventDefMap.size() );

  int row = 0;
  NLSimple::EventDefMap::const_iterator iter;
  for( iter = eventDefMap.begin(); iter != eventDefMap.end(); ++iter )
  {
    m_eventTypesModel->setData( row, 0, iter->first );
    m_eventTypesModel->setData( row++, 1, WString(iter->second.getName()) );
  }//for loop over and add data to the model
}//void updateAvailableEventTypes()



void CustomEventTab::defineDefaultDexcomEvents()
{
  NLSimplePtr nlsimpleptr( m_parentWtGui );
  nlsimpleptr->defineDefautDexcomEvents();
  updateAvailableEventTypes();
}//void CustomEventTab::defineDefaultDexcomEvents()




int CustomEventTab::selectedEventType() const
{
  const set<WModelIndex> selected = m_eventTypesView->selectedIndexes();

  if( selected.empty() ) return INT_MIN;

  set<WModelIndex>::const_iterator ind;
  for( ind = selected.begin(); ind != selected.end(); ++ind )
  {
    if( ind->column() == 0 )
    {
      try{ return boost::any_cast<int>( ind->data() ); }
      catch(...){}
    }//if( ind->row() == 0 )
  }//for( loop over selected indices )

  return INT_MIN;
}//int selectedEventType() const


void CustomEventTab::displaySelectedModel()
{
  m_currentCEModel->setArrayAddresses( 0, NULL, NULL );
  if( !m_eventTypesModel->rowCount() ) return;

  const int eventType = selectedEventType();
  if( eventType == INT_MIN ) return;

  NLSimplePtr nlsimpleptr( m_parentWtGui );
  const std::map<int, EventDef> &eventDefs = nlsimpleptr->m_customEventDefs;
  std::map<int, EventDef>::const_iterator eventDef = eventDefs.find(eventType);

  if( eventDef == eventDefs.end() )
  {
    cerr << "CustomEventTab::displaySelectedModel(): Couldn't find an"
         << " event of type " << eventType << "\nThis shouldn't happen"
         << endl;
    return;
  }//
  const EventDef &event = eventDef->second;

  cerr << "event.getNPoints()=" << event.getNPoints() << ": ";
  for( size_t i = 0; i < event.getNPoints(); ++i ) cerr << "( " << event.times()[i] << ", " << event.values()[i] << " ), ";
  cerr << endl;
  m_currentCEModel->setArrayAddresses( event.getNPoints(),
                                       event.times(),
                                       event.values() );
}//void displaySelectedModel()


void CustomEventTab::addCustomEventDialog()
{
  WDialog dialog( "Define a new Custom Event Type:" );

  Wt::WGroupBox *container = new Wt::WGroupBox("Effect On System", dialog.contents() );
  Wt::WButtonGroup *group = new Wt::WButtonGroup( dialog.contents() );

  Wt::WRadioButton *button;
  button = new Wt::WRadioButton("Constant Effect", container);
  new Wt::WBreak(container);
  group->addButton(button, IndependantEffect);

  button = new Wt::WRadioButton("Insulin Dependant", container);
  new Wt::WBreak(container);
  group->addButton(button, MultiplyInsulin);

  button = new Wt::WRadioButton("Carb Depentdant", container);
  new Wt::WBreak(container);
  group->addButton(button, MultiplyCarbConsumed);

  group->setCheckedButton( group->button(IndependantEffect) );

  int n_points = 6;
  TimeDuration duration( 2, 0, 0, 0 );
  new TimeDurationSpinBox( &duration, "Total Duration",
                           0.0, 24.0*60, dialog.contents() );
  new WBreak( dialog.contents() );
  new IntSpinBox( &n_points, "Number of Points", "", 1, 50, dialog.contents() );
  new WBreak( dialog.contents() );

  int event_id = -1;
  IntSpinBox *id = new IntSpinBox( &event_id, "ID:", "",
                                   0, 1000, dialog.contents() );
  WLabel *label = new WLabel( "Name ", dialog.contents() );
  WLineEdit *name = new WLineEdit( "", dialog.contents() );
  label->setBuddy( name );

  new WBreak( dialog.contents() );
  WPushButton *ok = new WPushButton( "Create", dialog.contents() );
  ok->disable();
  WPushButton *cancel = new WPushButton( "Cancel", dialog.contents() );
  ok->clicked().connect( &dialog, &WDialog::accept );
  cancel->clicked().connect( &dialog, &WDialog::reject );

  id->valueChanged().connect(
                 boost::bind( &CustomEventTab::validateCustomEventNameAndID,
                              this, name, (WAbstractSpinBox *)id->spinBox(), ok ) );
  name->changed().connect(
                 boost::bind( &CustomEventTab::validateCustomEventNameAndID,
                              this, name, (WAbstractSpinBox *)id->spinBox(), ok ) );

  const WDialog::DialogCode code = dialog.exec();

  if( code == WDialog::Rejected ) return;

  cerr << "Defineing custom event tagged by ID=" << event_id
       << " named " << name->text().narrow()
       << " with a duration of " << duration
       << " of EventDefType=" << group->checkedId()
       << " described by " << n_points << " points" << endl;

  defineCustomEvent( event_id, name->text().narrow(), duration,
                     group->checkedId(), n_points );

}//void addCustomEventDialog();


void CustomEventTab::validateCustomEventNameAndID( WLineEdit *name,
                                                      WAbstractSpinBox *id,
                                                     WPushButton *button )
{
  button->enable();

  const string nameStr = name->text().narrow();

  double value = 0.0;

  try
  {
    value = boost::lexical_cast<double>( id->text().narrow() );
  }catch(...)
  {
    id->setText( "" );
    button->disable();
    return;
  }

  const int idValue =  std::floor( value + 0.5 );

  if( value != value )
    button->disable();

  NLSimplePtr nlsimpleptr( m_parentWtGui );
  const std::map<int, EventDef> &eventDefs = nlsimpleptr->m_customEventDefs;
  NLSimple::EventDefMap::const_iterator iter = eventDefs.begin();

  if( nameStr.empty() ) button->disable();

  for( ; iter != eventDefs.end(); ++iter )
  {
    if( iter->first == idValue ) button->disable();
    if( iter->second.getName() == nameStr ) button->disable();
  }//for( loop over defined EventDefs )

  if( button->isEnabled() )
  {
    //Set the name and id objects to validated
    //name->set
  }
}//validateCustomEventNameAndID


void CustomEventTab::defineCustomEvent( const int recordType,
                                          const string name,
                                          const TimeDuration eventDuration,
                                          const int eventDefType, //of type EventDefType, see ResponseModel.hh
                                          const int nPoints )
{
  NLSimplePtr nlsimpleptr( m_parentWtGui );
  nlsimpleptr->defineCustomEvent( recordType, name, eventDuration,
                                  EventDefType(eventDefType), nPoints );
  updateAvailableEventTypes();
  displaySelectedModel();
}//void CustomEventTab::defineCustomEvent( ... )


void CustomEventTab::confirmUndefineCustomEventDialog()
{
  const int eventType = selectedEventType();

  string name = "";

  {
    NLSimplePtr nlsimpleptr( m_parentWtGui );
    const std::map<int, EventDef> &eventDefs = nlsimpleptr->m_customEventDefs;
    std::map<int, EventDef>::const_iterator eventDef = eventDefs.find(eventType);

    if( eventDef == eventDefs.end() )
    {
      cerr << "CustomEventTab::confirmUndefineCustomEventDialog(): WTF?"
          << endl;
      return;
    }//if( eventDef == eventDefs.end() )
    name = eventDef->second.getName();
  }

  WDialog dialog( "Confirm" );
  const string msg = "Are you sure you would like to delete the event named '"
                     + name + "'";
  new WText( msg, dialog.contents() );
  new WBreak( dialog.contents() );
  WPushButton *yes = new WPushButton( "Yes", dialog.contents() );
  WPushButton *no = new WPushButton( "no", dialog.contents() );
  yes->clicked().connect( &dialog, &WDialog::accept );
  no->clicked().connect( &dialog, &WDialog::reject );

  WDialog::DialogCode status = dialog.exec();

  if( status == WDialog::Rejected ) return;

  undefineCustomEvent( eventType );
}//void confirmUndefineCustomEventDialog( int index );


bool CustomEventTab::undefineCustomEvent( int recordType )
{
  NLSimplePtr nlsimpleptr( m_parentWtGui );
  const bool status = nlsimpleptr->undefineCustomEvent( recordType );

  updateAvailableEventTypes();
  displaySelectedModel();

  return status;
}//bool undefineCustomEvent( int recordType )




NotesTab::NotesTab( WtGui *wtGuiParent, Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_parentWtGui( wtGuiParent ),
    m_dateSelect( new DateTimeSelect( "Note Date/Time:" ) ),
    m_model( new WtNotesVectorModel( m_parentWtGui, this ) ),
    m_tableView( new WTableView() ),
    m_textArea( new WTextArea() ),
    m_beingEdited( NULL ),
    m_saveCheckBox( new WCheckBox( "Auto save to disk upon edit" ) ),
    m_newNoteButton( new WPushButton( "New Note" ) ),
    m_saveButton( new WPushButton( "Update Current" ) ),
    m_cancelButton( new WPushButton( "Cancel Edit" ) ),
    m_deleteButton( new WPushButton( "Delete Selected" ) )
{
  WGridLayout *layout = new WGridLayout();
  setLayout( layout );

  m_saveCheckBox->setChecked();

  Div *header = new Div();
  new WText( "On this tab you can enter notes for yourself about anything "
             "you&apos;de like:&nbsp;&nbsp;&nbsp;", XHTMLUnsafeText, header );
//  header->addWidget( m_saveCheckBox );
  layout->addWidget( header, 0, 0, 1, 1, AlignTop | AlignLeft );
  layout->setRowStretch( 0, 0 );

  m_tableView->setModel( m_model );
  m_tableView->setStyleClass( m_tableView->styleClass() + " NotesTabTableView" );
  m_tableView->setSelectable(true);
  m_tableView->setSelectionBehavior( SelectRows );
  m_tableView->setSelectionMode( SingleSelection );
  m_tableView->setAlternatingRowColors( true );
  m_tableView->setColumnResizeEnabled(true);
  m_tableView->setColumnWidth( 1, WLength( 30, WLength::FontEx ) );
  m_tableView->setSortingEnabled( false );
  m_tableView->selectionChanged().connect( this, &NotesTab::handleSelectionChange );
  layout->addWidget( m_tableView, 1, 0, 1, 1  );
  layout->setRowStretch( 1, 5 );


  //m_deleteButton->setFloatSide( Right );
  m_deleteButton->setHiddenKeepsGeometry(true);
  m_deleteButton->clicked().connect( this, &NotesTab::removeCurrentEntry );
  layout->addWidget( m_deleteButton, 2, 0, 1, 1, AlignTop | AlignLeft );
  layout->setRowStretch( 2, 0 );

  Div *timestampDiv = new Div();
  WPushButton *nowButton = new WPushButton( "Set to Now" );
  timestampDiv->addWidget( m_dateSelect );
  timestampDiv->addWidget( nowButton );

  nowButton->clicked().connect( m_dateSelect, &DateTimeSelect::setToCurrentTime );
  layout->addWidget( timestampDiv, 3, 0, 1, 1, AlignTop | AlignLeft );
  layout->setRowStretch( 3, 0 );

  layout->addWidget( m_textArea, 4, 0, 1, 1 );
  layout->setRowStretch( 4, 10 );

  Div *buttonDiv = new Div();
  m_newNoteButton->clicked().connect( this, &NotesTab::newEntry );
  m_saveButton->clicked().connect( boost::bind( &NotesTab::saveCurrent, this, false ) );
  m_cancelButton->clicked().connect( this, &NotesTab::cancelEdit );
  buttonDiv->addWidget( m_newNoteButton );
  buttonDiv->addWidget( m_saveButton );
  buttonDiv->addWidget( m_cancelButton );
  buttonDiv->addWidget( new WText( "&nbsp;&nbsp;&nbsp;", XHTMLUnsafeText) );
  buttonDiv->addWidget( m_saveCheckBox );

  layout->addWidget( buttonDiv, 5, 0, 1, 1, AlignTop | AlignLeft );
  layout->setRowStretch( 5, 0 );
}//NotesTab constructor

void NotesTab::newEntry()
{
  saveCurrent();
  m_beingEdited = NULL;

  m_saveButton->enable();
  m_saveButton->setHidden( false );

  m_newNoteButton->enable();
  m_newNoteButton->setHidden( false );

  m_cancelButton->enable();
  m_cancelButton->setHidden( false );

  m_deleteButton->disable();
  m_deleteButton->setHidden( true );

  m_textArea->setText( "" );
  m_dateSelect->setToCurrentTime();
  m_tableView->setSelectedIndexes( WModelIndexSet() );
}//void NotesTab::newEntry()


void NotesTab::cancelEdit()
{
  if( !m_beingEdited )
  {
    m_dateSelect->setToCurrentTime();
    m_textArea->setText( "" );
    return;
  }//if( !m_beingEdited )

  m_dateSelect->set( WDateTime::fromPosixTime(m_beingEdited->time) );
  m_textArea->setText( m_beingEdited->text );
}//void NotesTab::cancelEdit()


void NotesTab::saveCurrent( const bool askUserFirst )
{
  const PosixTime time = m_dateSelect->dateTime().toPosixTime();
  const string text = m_textArea->text().narrow();

  if( m_beingEdited )
  {
    const bool textIsSame = ( text == m_beingEdited->text );
    const bool timeIsSame = ( (time - m_beingEdited->time) <= boost::posix_time::minutes(1) );
    if( textIsSame && timeIsSame ) return;
  }else if( text == "" ) return;

  if( askUserFirst )
  {
    WDialog dialog( "Save Current Note?" );
    new WText("Would you like to save changes to current note?", dialog.contents() );
    new WBreak( dialog.contents() );
    WPushButton *yes = new WPushButton(  "Yes", dialog.contents() );
    WPushButton *no = new WPushButton(  "No", dialog.contents() );
    yes->clicked().connect( &dialog, &WDialog::accept );
    no->clicked().connect( &dialog, &WDialog::reject );
    WDialog::DialogCode code = dialog.exec();

    if( code == WDialog::Rejected ) return;
  }//if( !forceSave )

  if( !m_beingEdited )
  {
    if( text == "" ) return;
    NLSimple::NotesVector::iterator pos = m_model->addRow( time, text );
    m_beingEdited = &(*pos);
    WModelIndexSet indset;
    indset.insert( m_model->index( pos ) );
    m_tableView->setSelectedIndexes( indset );
    if( m_saveCheckBox->isChecked() ) m_parentWtGui->saveCurrentModel();
    return;
  }//if( !m_beingEdited )

  //if we made it here we are editing a pre-existing note, with the date
  //  having been changed
  const bool timeIsSame = ( (time - m_beingEdited->time) <= boost::posix_time::minutes(1) );
  if( timeIsSame )
  {
    m_beingEdited->text = text;
    if( m_saveCheckBox->isChecked() ) m_parentWtGui->saveCurrentModel();
    return;
  }//if( timeIsSame )

  //First delete the current entry, then insert a new one
  m_model->removeRow( m_beingEdited );
  NLSimple::NotesVector::iterator pos = m_model->addRow( time, text );
  m_beingEdited = &(*pos);
  WModelIndexSet indset;
  indset.insert( m_model->index( pos ) );
  m_tableView->setSelectedIndexes( indset );
  if( m_saveCheckBox->isChecked() ) m_parentWtGui->saveCurrentModel();
}//void NotesTab::saveCurrent()


void NotesTab::removeCurrentEntry()
{
  const WModelIndexSet selected = m_tableView->selectedIndexes();
  if( selected.empty() ) assert( !m_beingEdited );
  if( selected.empty() ) return;

  assert( m_beingEdited );

  const WModelIndex selectedIndex = *(selected.begin());
  assert( selectedIndex.isValid() );
  const boost::any selectedTime = m_model->data( selectedIndex );
  WDateTime dateTime;
  try{ dateTime = boost::any_cast<WDateTime>( selectedTime ); }
  catch(...){ assert(0); }
  assert( m_beingEdited->time == dateTime.toPosixTime() );

  WDialog dialog( "Confirmation" );
  const WString text = "Are you sure you would like to delete the note from "
                       + dateTime.toString() + "?";
  new WText( text, dialog.contents() );
  new WBreak( dialog.contents() );
  WPushButton *yes = new WPushButton(  "Yes", dialog.contents() );
  WPushButton *no = new WPushButton(  "No", dialog.contents() );
  yes->clicked().connect( &dialog, &WDialog::accept );
  no->clicked().connect( &dialog, &WDialog::reject );
  WDialog::DialogCode code = dialog.exec();

  if( code == WDialog::Rejected ) return;

  m_model->removeRow( m_beingEdited );
  m_beingEdited = NULL;

  m_textArea->setText( "" );
  m_dateSelect->setToCurrentTime();
  m_tableView->setSelectedIndexes( WModelIndexSet() );

  m_deleteButton->disable();
  m_deleteButton->setHidden( true );
}//void NotesTab::removeCurrentEntry()


void NotesTab::handleSelectionChange()
{
  //problem: at the end of this function we re-select the row of m_tableView
  //         the user intended, incase we inserted a row  when we call
  //         saveCurrent(); when we re-select the row, we recursively call
  //         this function, so we need a mutex to make sure we don't mess
  //         things up.

  //make sure only one thread at a time can access this function
  NLSimplePtr dummyPtrForMutex( m_parentWtGui );

  //now make sure this thread can only access this function one time
  static boost::mutex mymutex;
  boost::mutex::scoped_lock mylock( mymutex, boost::try_to_lock );
  if( !mylock ) return;

  const WModelIndexSet selected = m_tableView->selectedIndexes();

  if( selected.empty() )
  {
    m_beingEdited = NULL;
    m_textArea->setText( "" );
    m_dateSelect->setToCurrentTime();
    m_deleteButton->disable();
    m_deleteButton->setHidden( true );
    return;
  }//if( selected.empty() )

  WModelIndex selectedIndex = *(selected.begin());
  assert( selectedIndex.isValid() );
  const TimeTextPair newlySelected = *(m_model->dataPointer( selectedIndex ));

  saveCurrent();

  m_beingEdited = m_model->find( newlySelected );
  assert( m_beingEdited );

  m_deleteButton->enable();
  m_deleteButton->setHidden( false );

  m_textArea->setText( m_beingEdited->text );
  m_dateSelect->set( WDateTime::fromPosixTime(m_beingEdited->time) );

  //Now incase saveCurrent() inserted a row into the model, make sure
  //  the desired row is selected
  selectedIndex = m_model->index( m_beingEdited );

  WModelIndexSet selectedSet;
  selectedSet.insert( selectedIndex );
  m_tableView->setSelectedIndexes( selectedSet );

  m_deleteButton->enable();
  m_deleteButton->setHidden( false );
}//void NotesTab::handleSelectionChange()

void NotesTab::updateViewTable()
{
  //What did I want this function to do?
  m_model->refresh();
}//void NotesTab::updateViewTable()




ExcludeTimeRangesTab::ExcludedRangesChart::ExcludedRangesChart(
              ExcludeTimeRangesTab *parentTab, Wt::WContainerWidget *parent )
  : WChartWithLegend( parentTab->m_parentWtGui, parent ),
    m_parentTab( parentTab )
{
}//

ExcludeTimeRangesTab::ExcludedRangesChart::~ExcludedRangesChart()
{
}

void ExcludeTimeRangesTab::ExcludedRangesChart::paint( Wt::WPainter& painter, const Wt::WRectF& rectangle ) const
{
  WChartWithLegend::paint( painter, rectangle );

  //excluded ranges are already greyed out, here we just want to yellow out the
  //  currently selected range

  const WModelIndexSet selected = m_parentTab->m_view->selectedIndexes();

  const bool draw = ( selected.size() || m_parentTab->m_newRangeButton->isHidden() );

  if( draw )
  {
    PosixTime start = m_parentTab->m_startExcludeSelect->dateTime().toPosixTime();
    PosixTime end = m_parentTab->m_endExcludeSelect->dateTime().toPosixTime();

    if( start.is_special() || end.is_special() )
      return;

    boost::shared_ptr<NLSimpleDisplayModel> displayModel;
    displayModel = m_parentGui->getSimpleSimDisplayModel();
    const PosixTime &minTime = displayModel->beginDisplayTime();
    const PosixTime &maxTime = displayModel->endDisplayTime();

    if( start >= minTime && start < maxTime  && end > minTime  )
    {
      if( start < minTime )
        start = minTime;
      if( start > maxTime )
        start = maxTime;
      if( end < minTime )
        end = minTime;
      if( end > maxTime )
        end = maxTime;

      WPointF upperLeft = mapToDevice( WDateTime::fromPosixTime(start),
                                       axis(Chart::YAxis).maximum() );
      WPointF lowerRight = mapToDevice( WDateTime::fromPosixTime(end),
                                       axis(Chart::YAxis).minimum() );
      WColor fillColor( 50, 0, 0, 100 );
      painter.fillRect( WRectF( upperLeft, lowerRight ), WBrush( fillColor ) );
//      cerr << "\n\nE"
//           << ", ul={" << upperLeft.x() << "," << upperLeft.y() << "}"
//           << ", lr={" << lowerRight.x() << "," << lowerRight.y() << "}"
//           << endl << endl;
    }//if( start >= minTime && start < maxTime  && end > minTime  )

    painter.setPen( Wt::WPen() );

  }//if( draw )

}//paint(...)

void ExcludeTimeRangesTab::ExcludedRangesChart::paintEvent( Wt::WPaintDevice *paintDevice )
{
  WChartWithLegend::paintEvent( paintDevice );
}



ExcludeTimeRangesTab::ExcludeTimeRangesTab( WtGui *parentWtGui,
                                                Wt::WContainerWidget *parent )
  : WContainerWidget( parent ), m_parentWtGui( parentWtGui ),
    m_chart( new ExcludedRangesChart( this ) ),
    m_chartOverlay( NULL ),
    m_displayModel( new NLSimpleDisplayModel( parentWtGui ) ),
    m_view( new WTableView() ),
    m_listModel( new WtTimeRangeVecModel( parentWtGui ) ),
    m_deleteButton( new WPushButton( "Delete Selected" ) ),
    m_newRangeButton( new WPushButton( "Add New Range" ) ),
    m_startExcludeSelect( new DateTimeSelect( "Start Time") ),
    m_endExcludeSelect( new DateTimeSelect( "&nbsp;&nbsp;&nbsp;End Time") ),
    m_addRangeButton( new WPushButton( "Add to Model" ) ),
    m_saveModel( new WCheckBox("save model") ),
    m_description( new WText() )
{
  WGridLayout *layout = new WGridLayout();
  WContainerWidget::setLayout( layout );

  //hook the widgets up to the functions interacting with them should call
  m_view->selectionChanged().connect( this, &ExcludeTimeRangesTab::displaySelected );
  m_addRangeButton->clicked().connect( this, &ExcludeTimeRangesTab::addEnteredRangeToModel );
  m_deleteButton->clicked().connect( this, &ExcludeTimeRangesTab::deleteSelectedRange );
  m_endExcludeSelect->changed().connect( boost::bind( &ExcludedRangesChart::update, m_chart, WFlags<PaintFlag>() ) );
  m_startExcludeSelect->changed().connect( boost::bind( &ExcludedRangesChart::update, m_chart, WFlags<PaintFlag>() ) );

//  m_endExcludeSelect->changed().connect( this, &ExcludeTimeRangesTab::updateGraphWithUserRange );
//  m_startExcludeSelect->changed().connect( this, &ExcludeTimeRangesTab::updateGraphWithUserRange );

  //Set up the view for the list of excluded time ranges
  m_view->setModel( m_listModel );
  m_view->setStyleClass( m_view->styleClass() + " ExcludeTimeRangesTabTableView" );
  m_view->setSelectable( true );
  m_view->setSelectionBehavior( SelectRows );
  m_view->setSelectionMode( SingleSelection );
  m_view->setAlternatingRowColors( true );
  m_view->setColumnResizeEnabled( false );
  m_view->setColumnWidth( 0, WLength( 18, WLength::FontEx ) );
  m_view->setColumnWidth( 1, WLength( 18, WLength::FontEx ) );
  m_view->setSortingEnabled( false );

  //Set up the chart that displays the selcted time range
  m_chart->setModel( m_displayModel );
  m_chart->setLegendEnabled(false);
  m_chart->setPlotAreaPadding( 25, Wt::Right );
  m_chart->setPlotAreaPadding( 70, Wt::Bottom );
  m_chart->axis(Chart::XAxis).setScale(Chart::DateTimeScale);
  m_chart->axis(Chart::XAxis).setLabelAngle(45.0);
  m_chart->axis(Chart::Y2Axis).setVisible(true);
  m_chart->axis(Chart::Y2Axis).setTitle( "Consumed Carbs" );
  const WPen &y2Pen = m_chart->palette()->strokePen(4);
  m_chart->axis(Chart::Y2Axis).setPen( y2Pen );
  m_chart->axis(Chart::YAxis).setTitle( "mg/dL" );
  m_chart->setMinimumSize( 200, 150 );

  //Tell the model and chart which data series we want to see
  m_displayModel->useColumn( NLSimple::kCgmsData );
  m_displayModel->useColumn( NLSimple::kFingerMeterData );
  m_displayModel->useColumn( NLSimple::kCalibrationData );
  m_displayModel->useColumn( NLSimple::kMealData );
  m_chart->addSeries( Chart::WDataSeries( 0, Chart::LineSeries  ) );
  m_chart->addSeries( Chart::WDataSeries( 1, Chart::PointSeries ) );
  m_chart->addSeries( Chart::WDataSeries( 2, Chart::PointSeries ) );
  m_chart->addSeries( Chart::WDataSeries( 3, Chart::PointSeries ) );

  m_chart->setXSeriesColumn( m_displayModel->columnCount()-1 );

  m_chartOverlay = new OverlayCanvas( m_chart, true, true );
//  m_chartOverlay->jsException()->connect( boost::bind( printexp, _1 ) );
  m_chartOverlay->userDragged().connect( boost::bind( &ExcludeTimeRangesTab::userDraggedCallback, this, _1, _2, _3 ) );

  m_newRangeButton->clicked().connect( boost::bind( &ExcludeTimeRangesTab::allowUserToEnterNewRange, this ) );

  //Now put all the widgets into the gui
  layout->addWidget( m_view, 0, 0, 1, 2  );
  layout->addWidget( m_chart, 0, 2, 1, 1  );
  layout->addWidget( m_newRangeButton, 1, 0, 1, 1 );
  layout->addWidget( m_deleteButton, 1, 1, 1, 1 );
  layout->addWidget( m_description, 1, 2, 1, 1  );
  Div *newTimePeriodDiv = new Div();

  newTimePeriodDiv->addWidget( m_startExcludeSelect );
  newTimePeriodDiv->addWidget( m_endExcludeSelect );
  newTimePeriodDiv->addWidget( m_addRangeButton );
  newTimePeriodDiv->addWidget( m_saveModel );
  m_saveModel->setChecked();
  m_saveModel->hide();
  layout->addWidget( newTimePeriodDiv, 2, 0, 1, 3 );

  layout->setColumnStretch( 0, 2 );
  layout->setColumnStretch( 1, 2 );
  layout->setColumnStretch( 2, 10 );
  layout->setRowStretch( 0, 10 );
//  layout->setRowStretch( 1, 1 );
  layout->setRowStretch( 2, 1 );

  m_saveModel->setHiddenKeepsGeometry( true );
  m_deleteButton->setHiddenKeepsGeometry( true );
  m_addRangeButton->setHiddenKeepsGeometry( true );
  m_endExcludeSelect->setHiddenKeepsGeometry( true );
  m_startExcludeSelect->setHiddenKeepsGeometry( true );

  setShowingExistingRange();
}//ExcludeTimeRangesTab constructor


void ExcludeTimeRangesTab::hideChartOverlay()
{
  if( m_chartOverlay )
    m_chartOverlay->hide();
}//void hideChartOverlay()


void ExcludeTimeRangesTab::showChartOverlay()
{
  m_chartOverlay->show();
//  m_chartOverlay->alignWithParent();
}//void showChartOverlay()


void ExcludeTimeRangesTab::userDraggedCallback( int x0, int y0, Wt::WMouseEvent event )
{
  if( fabs(event.document().x - double(x0)) < 5 )
    return;

  WPointF start = m_chart->mapFromDevice( WPointF(x0, y0) );
  WPointF finish = m_chart->mapFromDevice( WPointF(event.widget().x, event.widget().y) );

  const bool zoom = event.ctrlKey();

  const std::time_t startEpich = start.x();
  const std::time_t finishEpich = finish.x();

  const WDateTime earliesData = WDateTime::fromPosixTime( m_parentWtGui->getSimpleSimDisplayModel()->earliestData() );
  const WDateTime latestData  = WDateTime::fromPosixTime( m_parentWtGui->getSimpleSimDisplayModel()->latestData() );


  WDateTime startTime = WDateTime::fromTime_t( startEpich );
  WDateTime finishTime = WDateTime::fromTime_t( finishEpich );


  cerr << "Initish startTime=" << startTime.toString()
       << ", finishTime=" << finishTime.toString() << endl << endl;

//  const TimeDuration initialTimeDiff = finishTime - startTime;

  if( finishTime < earliesData )
    finishTime = earliesData;
  if( finishTime > latestData )
    finishTime = latestData;
  if( startTime < earliesData )
    startTime = earliesData;
  if( startTime > latestData )
    startTime = latestData;

  if( zoom )
  {
    if( startTime == finishTime )
    {
      startTime = earliesData;
      finishTime = latestData;
    }//if( startTime == finishTime )

    if( startTime > finishTime )
      std::swap( finishTime, startTime );
    zoomGraph( startTime, finishTime );
  }else
  {
    if( m_view->selectedIndexes().size() > 0 )
    {
      cerr << "ExcludeTimeRangesTab::userDraggedCallback(...): "
              "You cant zoom in when anything is selected" << endl;
      return;
    }//if( m_view->selectedIndexes().size() > 0 )

    cerr << "\nExcluding from " << startTime.toString()
         << " to " << finishTime.toString() << endl << endl;

    m_startExcludeSelect->set( startTime );
    m_endExcludeSelect->set( finishTime );
  }//if( zoom ) / else

  m_chart->update();  //XXX - Is this necassary?
}//void userDragZoomedBsGraph( int x1, int x2, Wt::WMouseEvent event )



void ExcludeTimeRangesTab::displaySelected()
{
  const WModelIndexSet selected = m_view->selectedIndexes();
//  if( selected.empty() && m_listModel->rowCount() ) return;

  if( m_listModel->rowCount() && selected.size() )
  {
    assert( selected.size() == 1 );

    WModelIndex beginDateIndex = *selected.begin();
    WModelIndex endDateIndex = m_listModel->index( beginDateIndex.row(), 1 );

    WDateTime start, end;
    try
    {
      start = boost::any_cast<WDateTime>( beginDateIndex.data() );
      end = boost::any_cast<WDateTime>( endDateIndex.data() );
    }catch(...)
    {
      cerr << SRC_LOCATION << ":\n   Thats weird - failed any_cast!" << endl;
      return;
    }//try / catch

    m_startExcludeSelect->set( start );
    m_endExcludeSelect->set( end );
    m_description->setText( start.toString() + " through " + end.toString() );
//    m_displayModel->setDisplayedTimeRange( start.toPosixTime(), end.toPosixTime() );
    setShowingExistingRange();
  }else
  {
//    const PosixTime now = boost::posix_time::second_clock::local_time();
    m_description->setText( "Select a range to view activity" );
//    m_displayModel->setDisplayedTimeRange( now, now );
    setShowingExistingRange();
  }//if we have something to display / else

//  m_saveModel->setHidden( true );
  m_saveModel->setHidden( false );
  m_chart->update();
}//displaySelected()


void ExcludeTimeRangesTab::setAddingNewRange()
{
  m_view->setSelectedIndexes( WModelIndexSet() );
  m_deleteButton->setHidden( true );
  m_newRangeButton->setHidden( true );
  m_addRangeButton->setHidden( false );
  m_endExcludeSelect->setHidden( false );
  m_startExcludeSelect->setHidden( false );
}//void setAddingNewRange()

void ExcludeTimeRangesTab::setShowingExistingRange()
{
  if( m_view->selectedIndexes().size() )
  {
    m_newRangeButton->setHidden( false );
    m_deleteButton->setHidden( false );
    m_addRangeButton->setHidden( true );
    m_endExcludeSelect->setHidden( true );
    m_startExcludeSelect->setHidden( true );
  }else
  {
    m_newRangeButton->setHidden( false );
    m_deleteButton->setHidden( true );
    m_addRangeButton->setHidden( true );
    m_endExcludeSelect->setHidden( true );
    m_startExcludeSelect->setHidden( true );
  }//if( m_view->selectedIndexes().size() ) / else
}//void setShowingExistingRange()

void ExcludeTimeRangesTab::allowUserToEnterNewRange()
{
  m_view->setSelectedIndexes( WModelIndexSet() );

  m_saveModel->setHidden( false );
  m_deleteButton->setHidden( true );
  m_newRangeButton->setHidden( true );

  m_addRangeButton->setHidden( false );
  m_endExcludeSelect->setHidden( false );
  m_startExcludeSelect->setHidden( false );

//  m_endExcludeSelect->setToCurrentTime();
//  m_startExcludeSelect->setToCurrentTime();

  updateDataRangeDates();
//  m_displayModel->setDisplayedTimeRange( m_startExcludeSelect->dateTime().toPosixTime(),
//                                         m_endExcludeSelect->dateTime().toPosixTime() );
}//allowUserToEnterNewRange()


void ExcludeTimeRangesTab::addEnteredRangeToModel()
{
  const WDateTime end = m_endExcludeSelect->dateTime();
  const WDateTime start = m_startExcludeSelect->dateTime();

  if( start > end ) return;

  bool added = m_listModel->addRow( start.toPosixTime(), end.toPosixTime() );
  if( added && m_saveModel->isChecked() ) m_parentWtGui->saveCurrentModel();

  int newRow = -1;
  const int nCol = m_listModel->columnCount();

  for( int row = 0; row < nCol; ++row )
  {
    WModelIndex col1 = m_listModel->index( row, 0 );
    WModelIndex col2 = m_listModel->index( row, 1 );

    try
    {
      WDateTime first = boost::any_cast<WDateTime>( col1.data() );
      WDateTime second = boost::any_cast<WDateTime>( col2.data() );
      if( (first == start) && (second == end) )
      {
        newRow = row;
        break;
      }//if( we found the row we just inserted )
    }catch(...){}
  }//for( loop over rows )

  if( newRow < 0 )
  {
    cerr << SRC_LOCATION << ":\n   thats odd, couldnt find the date we just"
         << " entered :(" << endl;
    m_view->setSelectedIndexes( WModelIndexSet() );
    return;
  }//if( newRow < 0 )

  WModelIndexSet selected;
  selected.insert( m_listModel->index( newRow, 0 ) );
  m_view->setSelectedIndexes( selected );
  displaySelected();
}//void addEnteredRangeToModel()

void ExcludeTimeRangesTab::finishDeleteSelectedDialog( WDialog *dialog,
                                                         const WModelIndex selected,
                                                         WCheckBox *save)
{
  WDialog::DialogCode code = dialog->result();

  if( code == WDialog::Rejected )
  {
    delete dialog;
    return;
  }

  m_listModel->removeRows( selected.row(), 1 );

  if( save->isChecked() )
    m_parentWtGui->saveCurrentModel();

  const int selectedRow = selected.row();

  WModelIndexSet setSelected;
  if( selectedRow < m_listModel->rowCount() )
    setSelected.insert( m_listModel->index( selectedRow, 1 ) );
  else if( selectedRow )
    setSelected.insert( m_listModel->index( selectedRow-1, 1 ) );

  m_view->setSelectedIndexes( setSelected );

  displaySelected();

  delete dialog;
}//finishDeleteSelectedDialog(...)

void ExcludeTimeRangesTab::deleteSelectedRange()
{
  const WModelIndexSet selected = m_view->selectedIndexes();
  if( selected.empty() ) return;
  assert( selected.size() == 1 );

  WModelIndex beginDateIndex = *selected.begin();
  const int selectedRow = beginDateIndex.row();
  WModelIndex endDateIndex = m_listModel->index( selectedRow, 1 );

  WDateTime start, end;
  try
  {
    end = boost::any_cast<WDateTime>( endDateIndex.data() );
    start = boost::any_cast<WDateTime>( beginDateIndex.data() );
  }catch(...)
  {
    cerr << SRC_LOCATION << ":\n   Thats weird - failed any_cast!" << endl;
    return;
  }//try / catch

  WDialog *dialog = new WDialog( "Confirmation" );
  const WString text = "Are you sure you would like to delete the time range"
                       "from " + start.toString() + " to " + end.toString()
                       + "?";
  new WText( text, dialog->contents() );
  new WBreak( dialog->contents() );
  WPushButton *yes = new WPushButton(  "Yes", dialog->contents() );
  WPushButton *no = new WPushButton(  "No", dialog->contents() );
  WCheckBox *save = new WCheckBox( "Save Model" );
  save->setChecked();
  save->hide();
  yes->clicked().connect( dialog, &WDialog::accept );
  no->clicked().connect( dialog, &WDialog::reject );

  dialog->finished().connect( boost::bind( &ExcludeTimeRangesTab::finishDeleteSelectedDialog, this, dialog, beginDateIndex, save) );

  dialog->show();
}//void deleteSelectedRange()


void ExcludeTimeRangesTab::zoomGraph( const WDateTime start, const WDateTime end )
{
  if( !start.isValid() || !end.isValid() )
  {
    cerr << "Warning: ExcludeTimeRangesTab::zoomGraph(...): and invalid date" << endl;
    return;
  }//if( !start.isValid() || !end.isValid() )

  if( start < end )
  {
    m_description->setText( start.toString() + " through " + end.toString() );
    m_displayModel->setDisplayedTimeRange( start.toPosixTime(), end.toPosixTime() );
  }else
  {
    m_description->setText( end.toString() + " through " + start.toString() );
    m_displayModel->setDisplayedTimeRange( end.toPosixTime(), start.toPosixTime() );
  }//if( start < end ) / else
}//void zoomGraph( const WDateTime start, const WDateTime end )


void ExcludeTimeRangesTab::updateDataRangeDates()
{
  const PosixTime pBeginTime = m_parentWtGui->getSimpleSimDisplayModel()->earliestData();
  const PosixTime pEndTime   = m_parentWtGui->getSimpleSimDisplayModel()->latestData();
  const WDateTime endTime    = WDateTime::fromPosixTime( pEndTime );
  const WDateTime beginTime  = WDateTime::fromPosixTime( pBeginTime );

  m_startExcludeSelect->setTop( endTime );
  m_startExcludeSelect->setBottom( beginTime );
  m_startExcludeSelect->set( beginTime );

  m_endExcludeSelect->setTop( endTime );
  m_endExcludeSelect->setBottom( beginTime );
  m_endExcludeSelect->set( endTime );
}//void ExcludeTimeRangesTab::updateDataRangeDates()






