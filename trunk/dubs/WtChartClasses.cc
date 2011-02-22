#include <algorithm>

#include "boost/any.hpp"
#include "boost/foreach.hpp"
#include "boost/date_time.hpp"

#include <Wt/WPainter>
#include <Wt/WRectF>
#include <Wt/WDateTime>
#include <Wt/Chart/WCartesianChart>
#include <Wt/Chart/WChart2DRenderer>
#include <Wt/WAbstractItemModel>
#include <Wt/WApplication>
#include <Wt/WPointF>
#include <Wt/WString>
#include <Wt/Chart/WChartPalette>
#include <Wt/WRectArea>

#include "TMath.h"

#include "WtGui.hh"
#include "WtChartClasses.hh"
#include "ConsentrationGraph.hh"
#include "ResponseModel.hh"
#include "CgmsDataImport.hh"
#include "ArtificialPancrease.hh"

#include "boost/algorithm/string.hpp"

using namespace Wt;
using namespace std;
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

const boost::posix_time::time_duration NLSimpleDisplayModel::sm_plasmaInsulinDt( 0, 15, 0 );  //1 minutes


WChartWithLegend::WChartWithLegend( WtGui *parentGui, WContainerWidget *parent ) :
    Chart::WCartesianChart(Wt::Chart::ScatterPlot,parent), m_width(0),
  m_height(0), m_legTopOffset(30), m_legRightOffset(245),
  m_parentGui(parentGui)
{
  setMinimumSize( 250, 150 );
}//WChartWithLegend constructor

WChartWithLegend::~WChartWithLegend()
{
}

int WChartWithLegend::getHeight() const
{
  return m_height;
}

int WChartWithLegend::getWidth() const
{
  return m_width;
}

  //set how far legend is from the top of the GraphElement, in pixels
void WChartWithLegend::setLegendOffsetFromTop( const int &offset )
{
  m_legTopOffset = offset;
}

  //set how far legend is from the top of the GraphElement, in pixels
void WChartWithLegend::setLegendOffsetFromRight( const int &offset )
{
  m_legRightOffset = offset;
}

void WChartWithLegend::paint( WPainter& painter, const WRectF& rectangle ) const
{
  Wt::Chart::WCartesianChart::paint(painter, rectangle);
  for( size_t index=0; index<series().size(); ++index )
  {
    painter.setPen( Wt::WPen() );
    Wt::WPointF point( std::max(m_width-m_legRightOffset, 0),
                       m_legTopOffset+25*index );
    renderLegendItem(painter, point, series()[index]);
  }//for( loop over series )

  if( m_parentGui )
  {
    NLSimplePtr modelPtr( m_parentGui );
    const double minimum = axis(Wt::Chart::YAxis).minimum();
    const double maximum = axis(Wt::Chart::YAxis).maximum();
    const double ypos = minimum + (maximum-minimum)*0.05;

    boost::shared_ptr<NLSimpleDisplayModel> displayModel;
    displayModel = m_parentGui->getSimpleSimDisplayModel();
    const PosixTime &minTime = displayModel->beginDisplayTime();
    const PosixTime &maxTime = displayModel->endDisplayTime();

    foreach( const GraphElement &el, modelPtr->m_customEvents )
    {
      if( (el.m_time>=minTime) && (el.m_time<=maxTime) )
      {
        WPointF pos = mapToDevice( WDateTime::fromPosixTime(el.m_time), ypos );
        WRectF loc( pos.x(), pos.y(), 0.1, 0.1 );
        painter.drawText( loc, AlignLeft, WString(Form("%i", TMath::Nint(el.m_value))) );
      }//if( this point is inside the range to be displayed )
    }//foreach custom event

    string url = "local_resources/";
    if( boost::algorithm::contains( wApp->url(), "dubs.app" ) )
      url = "dubs/exec/local_resources/";

    painter.setPen( palette()->strokePen( WtGui::kFreePlasmaInsulin ) );
    foreach( const GraphElement &el, modelPtr->m_boluses )
    {
      if( (el.m_time>=minTime) && (el.m_time<=maxTime) )
      {
        WPointF pos = mapToDevice( WDateTime::fromPosixTime(el.m_time), 5*el.m_value );
        WPainter::Image syringe( url+"syringe.png", "local_resources/syringe.png");
        pos.setX( pos.x() - 0.5*syringe.width() );
        pos.setY( pos.y() - 0.5*syringe.height() );
        painter.drawImage( pos, syringe );
      }//if( this point is inside the range to be displayed )
    }//foreach( const GraphElement &el, modelPtr->m_customEvents )

    painter.setPen( palette()->strokePen( WtGui::kFreePlasmaInsulin ) );
    foreach( const GraphElement &el, modelPtr->m_mealData)
    {
      if( (el.m_time>=minTime) && (el.m_time<=maxTime) )
      {
        WPointF pos = mapToDevice( WDateTime::fromPosixTime(el.m_time),
                                   el.m_value, Wt::Chart::Y2Axis );
        WPainter::Image burger(url+"hamburger.png","local_resources/hamburger.png");
        pos.setX( pos.x() - 0.5*burger.width() );
        pos.setY( pos.y() - 0.5*burger.height() );

        //      Wt::WRectArea *value = new Wt::WRectArea( pos.x(), pos.y(), burger.width(), burger.height() );
        //      value->setToolTip( boost::lexical_cast<string>(el.m_value)
        //                         + " grams of carbs at "
        //                         + boost::lexical_cast<string>(el.m_time) );
        //      const_cast<WChartWithLegend *>(this)->addArea( value );
        painter.drawImage( pos, burger );
      }//if( this point is inside the range to be displayed )
    }//foreach( const GraphElement &el, modelPtr->m_mealData )


    painter.setPen( Wt::WPen() );
  }//if( m_parentGui )
}//paint


void WChartWithLegend::paintEvent( WPaintDevice *paintDevice )
{
  m_width = (int)(paintDevice->width().toPixels());
  m_height = (int)(paintDevice->height().toPixels());
  Wt::Chart::WCartesianChart::paintEvent(paintDevice);

  /*
      const int padding = plotAreaPadding(Wt::Bottom)
                          + plotAreaPadding(Wt::Top)
                          + axis(Wt::Chart::YAxis).margin();
      const int chartAreaHeight = m_height - padding; //should subract of some more for axis maybe?

      const double minimum = axis(Wt::Chart::YAxis).minimum();
      const double maximum = axis(Wt::Chart::YAxis).maximum();

      std::cerr << "minimum=" << minimum << ", maximum=" << maximum << std::endl;

      const double onePixel = (maximum-minimum) / chartAreaHeight;
      if( chartAreaHeight != chartAreaHeight ) return;

      axis(Wt::Chart::YAxis).setRange(0, maximum + legendSpace*onePixel);
      Wt::Chart::WCartesianChart::paintEvent(paintDevice);
*/
}//paintEvent()








NLSimpleDisplayModel::NLSimpleDisplayModel( WtGui *wapp,
                                            WObject *parent )
  : WAbstractItemModel( parent ),
    m_wApp( wapp ),
    m_beginDisplayTime( boost::date_time::neg_infin ),
    m_endDisplayTime( boost::date_time::pos_infin )
{
}//NLSimpleDisplayModel( constructor )


void NLSimpleDisplayModel::useAllColums()
{
  assert(0);
  for( NLSimple::DataGraphs col = NLSimple::DataGraphs(0);
       col < NLSimple::kNumDataGraphs;
       col = NLSimple::DataGraphs(col+1) )
    if( col != NLSimple::kFreePlasmaInsulin ) useColumn(col);
}

NLSimpleDisplayModel::~NLSimpleDisplayModel()
{
}


int NLSimpleDisplayModel::rowCount( const PosixTime &startTime,
                                    const PosixTime &endTime ) const
{
  NLSimplePtr diabeticModel( m_wApp );
  if( !diabeticModel ) return 0;

  size_t nRows = 0;
  foreach( const NLSimple::DataGraphs &t, m_columns )
  {
    const ConsentrationGraph &data = diabeticModel->dataGraph(t);
    const ConstGraphIter lb = data.lower_bound( startTime );
    const ConstGraphIter ub = data.upper_bound( endTime );
    if( lb == ub ) continue;

    if( t != NLSimple::kFreePlasmaInsulin ) nRows += (ub - lb);
    else
    {
      nRows += ((ub-1)->m_time - lb->m_time).total_seconds()
               / sm_plasmaInsulinDt.total_seconds();
    }//if( t != FreePlasmaInsulin )
  }//foreach( const Columns &t, m_columns )

  return static_cast<int>( nRows );
}//rowCount( PosixTime, PosixTime )

 int NLSimpleDisplayModel::rowCount( const Wt::WModelIndex & ) const
 {
   return rowCount( m_beginDisplayTime, m_endDisplayTime );
 }//rowCount

int NLSimpleDisplayModel::columnCount( const Wt::WModelIndex &) const
{
  return static_cast<int>( m_columns.size() + 1 );
}//columnCount

Wt::WModelIndex NLSimpleDisplayModel::parent( const Wt::WModelIndex & ) const
{
  return WModelIndex();
}

boost::any NLSimpleDisplayModel::data( const WModelIndex& index, int role ) const
{
  if( role != Wt::DisplayRole ) return boost::any();

  NLSimplePtr diabeticModel( m_wApp );
  if( !diabeticModel ) return boost::any();

  const int columnWanted = index.column();

//  cerr << "Want column=" << columnWanted << " row=" << index.row() << endl;

  if( columnWanted > static_cast<int>(m_columns.size()) ) return boost::any();

  const bool wantTime = (columnWanted==static_cast<int>(m_columns.size()));
  const size_t wantedRow = static_cast<size_t>( index.row() );

  size_t row_n = 0;

  for( size_t col = 0; col < m_columns.size(); ++col )
  {
    const NLSimple::DataGraphs &column = m_columns[col];
    const ConsentrationGraph &data = diabeticModel->dataGraph( column );

    const ConstGraphIter lb = data.lower_bound( m_beginDisplayTime );
    const ConstGraphIter ub = data.upper_bound( m_endDisplayTime );
    if( ub == lb ) continue;  //garuntees us 'lb' is a valid iterator for below

    size_t dataSize = ub - lb;


    if( !wantTime && (row_n>wantedRow) ) return boost::any();
    const bool isCorrColl = (col == static_cast<size_t>(columnWanted));

    if( column==NLSimple::kFreePlasmaInsulin )
    {
      const PosixTime t = lb->m_time + sm_plasmaInsulinDt*(wantedRow-row_n);
      ConstGraphIter wanted_time_lb = data.lower_bound(t);
      const bool inThisData = ((wanted_time_lb>=lb) && (wanted_time_lb<ub));
      if( isCorrColl && inThisData )
      {
        const double value = data.value( t );
        if( TMath::AreEqualAbs( 0.0, value, 0.00001 ) ) return boost::any();
        return boost::any( value );
      }else if( inThisData && !wantTime ) return boost::any();
      else if( inThisData && wantTime )  return boost::any( WDateTime::fromPosixTime(t) );

      dataSize = ((ub-1)->m_time - lb->m_time).total_seconds()
                 / sm_plasmaInsulinDt.total_seconds();
      row_n += dataSize;
      continue;
    }//if( (column == FreePlasmaInsulin) )


    const bool inThisData = ((row_n+dataSize) > wantedRow);

    ConstGraphIter dataIter = lb + (wantedRow-row_n);

    if( isCorrColl && inThisData )     return boost::any( dataIter->m_value );
    else if( inThisData && !wantTime ) return boost::any();
    else if( inThisData && wantTime )
    {
      if( column==NLSimple::kCgmsData )
        return boost::any( WDateTime::fromPosixTime( dataIter->m_time - diabeticModel->m_settings.m_cgmsDelay) );
      return boost::any( WDateTime::fromPosixTime( dataIter->m_time) );
    }
    row_n += dataSize;
  }//for( loop over columns )

  return boost::any();
}//boost::any data( const WModelIndex& index, int role ) const


WModelIndex NLSimpleDisplayModel::index( int row, int column, const WModelIndex & ) const
{
  NLSimplePtr diabeticModel( m_wApp );
  if( !diabeticModel ) return WModelIndex();
  return WAbstractItemModel::createIndex( row, column, (void *)this );
}//WModelIndex index( ... ) const


bool NLSimpleDisplayModel::setHeaderData( int section,
                                          Orientation orientation,
                                          const boost::any &value,
                                          int role)
{
  if(orientation != Horizontal)
    return WAbstractItemModel::setHeaderData( section, orientation, value, role );

  if( section == static_cast<int>(m_columns.size()) ) return true;

  if( role == EditRole ) role = DisplayRole;
  m_columnHeaderData[section][role] = value;

  headerDataChanged().emit(orientation, section, section);
  return true;
}//bool NLSimpleDisplayModel::setHeaderData(...)



void NLSimpleDisplayModel::clear()
{
  m_columnHeaderData.clear();
  m_columns.clear();
  reset();
}//void NLSimpleDisplayModel::clear()


void NLSimpleDisplayModel::aboutToSetNewModel()
{
  //call before setting a new model (sends out notifcation all rows are
  //  being removed)
  const int nRow = rowCount();
  if( !nRow ) return;
  beginRemoveRows	(	WModelIndex(), 0, nRow-1 );
}//void aboutToSetNewModel()

void NLSimpleDisplayModel::doneSettingNewModel()
{
  //call after setting a new model (sends out notifcation rows have been added)
  endRemoveRows();
  const int nRow = rowCount();
  if( !nRow ) return;
  beginInsertRows( WModelIndex(), 0, nRow );
  endInsertRows();
}//void doneSettingNewModel()


int NLSimpleDisplayModel::begginingRow( NLSimple::DataGraphs wantedColumn )
{
  if( wantedColumn == m_columns.size() )
  {
    cerr << "You shouldn't be calling NLSimpleDisplayModel::begginingRow("
         << " NLSimple::DataGraphs ) for the TimeColumn!" << endl;
    return 0;
  }//if( col == TimeColumn )

  NLSimplePtr diabeticModel( m_wApp );

  size_t row_n = 0;

  for( size_t col = 0; col < m_columns.size(); ++col )
  {
    const NLSimple::DataGraphs &column = m_columns[col];
    const bool isCorrColl = (column==wantedColumn);
    if( isCorrColl ) return row_n;

    const bool isScaledColum = (column==NLSimple::kFreePlasmaInsulin);
    ConsentrationGraph &data = diabeticModel->dataGraph( column );

    const ConstGraphIter lb = data.lower_bound( m_beginDisplayTime );
    const ConstGraphIter ub = data.upper_bound( m_endDisplayTime );
    if( lb == ub ) continue;

    size_t dataSize = (ub - lb);

    if( isScaledColum )
    {
      dataSize = ((ub-1)->m_time - lb->m_time).total_seconds()
                 / sm_plasmaInsulinDt.total_seconds();
    }//if( isScaledColum )

    row_n += dataSize;
  }//for( loop over columns )

  cerr << "NLSimpleDisplayModel::begginingRow( NLSimple::DataGraphs col ): shouldn't have "
       << "made it to the end :(" << endl;
  return 0;
}//int begginingRow( Columns col )



bool NLSimpleDisplayModel::addData( const CgmsDataImport::InfoType type,
                                    const PosixTime &time,
                                    const double &value )
{
  using namespace CgmsDataImport;
  NLSimplePtr modelPtr( m_wApp );

  NLSimple::DataGraphs col = NLSimple::kNumDataGraphs;
  switch( type )
  {
    case CgmsReading:      col = NLSimple::kCgmsData; break;
    case MeterReading:     col = NLSimple::kFingerMeterData; break;
    case MeterCalibration: col = NLSimple::kCalibrationData; break;
    case GlucoseEaten:     col = NLSimple::kMealData; break;
    case BolusTaken:       col = NLSimple::kBoluses; break;
    case GenericEvent:     col = NLSimple::kCustomEvents; break;
    case ISig: assert(0); break;
  };//switch( det )

  assert( col < NLSimple::kNumDataGraphs );

  const int beginRow = begginingRow( col );
  ConsentrationGraph &g = modelPtr->dataGraph( col );
  GraphIter pos = g.lower_bound( time );
  const int row = beginRow + pos - g.begin();

  const bool isInDisplayedTimeRange = (time>=m_beginDisplayTime)
                                      && (time<=m_endDisplayTime);

  if( isInDisplayedTimeRange ) beginInsertRows( WModelIndex(), row, row );

  switch( type )
  {
    case CgmsReading:      modelPtr->addCgmsData( time, value );        break;
    case MeterReading:     modelPtr->addNonCalFingerStickData( time, value ); break;
    case MeterCalibration: modelPtr->addCalibrationData( time, value ); break;
    case GlucoseEaten:     modelPtr->addConsumedGlucose( time, value ); break;
    case BolusTaken:       modelPtr->addBolusData( time, value );       break;
    case GenericEvent:     modelPtr->addCustomEvent( time, value );     break;
    case ISig: assert(0);                           break;
  };//switch( det )

  if( isInDisplayedTimeRange ) endInsertRows();

  return isInDisplayedTimeRange;
}//addData()


bool NLSimpleDisplayModel::removeRows( int row, int count, const WModelIndex & )
{
  cerr << "NLSimpleDisplayModel::removeRows(...) not allowed to be called" << endl;
  assert(0);  //I actually think you can use this function, but I didnt test it, so being 'safe'


  NLSimplePtr diabeticModel( m_wApp );
  size_t row_n = 0;

  for( size_t col = 0; col < m_columns.size(); ++col )
  {
    const NLSimple::DataGraphs &column = m_columns[col];
    const bool isScaledColum = (column==NLSimple::kFreePlasmaInsulin);
    ConsentrationGraph &data = diabeticModel->dataGraph( column );

    const GraphIter lb = data.lower_bound( m_beginDisplayTime );
    const GraphIter ub = data.upper_bound( m_endDisplayTime );

    size_t dataSize = (ub - lb);

    if( isScaledColum && (ub != lb) )
    {
      dataSize = ((ub-1)->m_time - lb->m_time).total_seconds()
                 / sm_plasmaInsulinDt.total_seconds();
    }//if( isScaledColum )

    const bool inThisData = ((row_n+dataSize) > static_cast<size_t>(row));

    if( inThisData && isScaledColum )
    {
      cerr << "Sorry cant remove data from Free plasma insulin :(" << endl;
      return false;
    }

    if( inThisData )
    {
      int localRow = row - row_n;
      GraphIter first = lb + localRow;
      GraphIter last = ub;
      if( (first+count) <= ub )
      {
        last = first + count;
        col = m_columns.size();
      } else row = row + last-first;

      cerr << "Deleteing '" << first->m_time << "=" << first->m_value
           << " plus " << last-first-1 << "(" << count << ") more " << endl;
      beginRemoveRows( WModelIndex(), row, row+last-first );
      data.erase( first, last );
      endInsertRows();
    }//if( isCorrColl && inThisData )

    row_n += dataSize;
  }//for( loop over columns )

  return true;
}//bool NLSimpleDisplayModel::removeRows(...)



void NLSimpleDisplayModel::useColumn( NLSimple::DataGraphs col )
{
  if( col == NLSimple::kNumDataGraphs ) return;

  m_columns.push_back( col );
  m_columnHeaderData.push_back( HeaderData() );

  WString title;
  switch( col )
  {
    case NLSimple::kCgmsData:              title = "CGMS Readings (delay corr.)";break;
    case NLSimple::kFreePlasmaInsulin:     title = "Free Plasma Insulin (pred.)";break;
    case NLSimple::kGlucoseAbsorbtionRate: title = "Glucose Abs. Rate (pred.)";  break;
    case NLSimple::kMealData:              title = "Consumed Carbohydrates";     break;
    case NLSimple::kBoluses:               title = "Boluses";                    break;
    case NLSimple::kFingerMeterData:       title = "Finger Stick Readings";      break;
    case NLSimple::kCalibrationData:       title = "Calibration Finger Stick";   break;
    case NLSimple::kCustomEvents:          title = "User Defined Events";        break;
    case NLSimple::kPredictedBloodGlucose: title = "Predicted Blood Glucose";    break;
    case NLSimple::kPredictedInsulinX:     title = "Insulin X (pred.)";          break;
    case NLSimple::kNumDataGraphs:         title = "Time"; assert(0);            break;
  };//switch( col )

  setHeaderData( m_columnHeaderData.size()-1, Horizontal, title );
}//void NLSimpleDisplayModel::useColumn( Columns col )



boost::any NLSimpleDisplayModel::headerData( int section,
                                             Orientation orientation,
                                             int role) const
{
  if( role == LevelRole ) return 0;

  if(orientation != Horizontal)
    return WAbstractItemModel::headerData( section, orientation, role );

  if( section == static_cast<int>(m_columns.size()) )
    return boost::any( WString("Time") );

  assert( static_cast<size_t>(section) < m_columnHeaderData.size() );
  const HeaderData &info = m_columnHeaderData[section];

  HeaderData::const_iterator i = info.find(role);

  return i != info.end() ? i->second : boost::any();
}//boost::any NLSimpleDisplayModel::headerData(...)


void NLSimpleDisplayModel::dataExternallyChanged()
{
  beginRemoveRows( WModelIndex(), 0, rowCount() );
  endRemoveRows();

  beginInsertRows( WModelIndex(), 0, rowCount() );
  endInsertRows();

  cerr << "Don't use void NLSimpleDisplayModel::dataExternallyChanged()!" << endl;
  assert(0);


  /*
  //We need to avoid a recursive calling of the dataChanged.emit() function
  //  (eg if we hook two NLSimpleDisplayModel's together to keep in sync)
  //  so we'll just use a mutex that allows only one data change to happen
  //  at a time, I think as long as we only change the model in one thread
  //  at a time (like we should), all should be fine.
  boost::mutex::scoped_lock lock( m_dataBeingChangedMutex, boost::try_to_lock );
  if( lock.owns_lock() ) cerr << "Owns Lock" << endl;
  else cerr << "Doesn't owns Lock" << endl;

  if( !lock.owns_lock() )  return;

  // const int oldNRow = rowCount();
  // const int rowStart = upperLeft.row();
  // const int rowEnd = lowerRight.row();
  // cerr << "oldNRow=" << oldNRow << ", rowStart=" << rowStart << " rowEnd=" << rowEnd << endl;
  // if( rowEnd > oldNRow ) rowsInserted().emit( WModelIndex(), oldNRow, rowEnd );
  // else if( rowEnd < oldNRow ) rowsRemoved().emit( WModelIndex(), rowEnd, oldNRow);

  cerr << "Changing " << upperLeft.row() << " to " << lowerRight.row() << endl;
  dataChanged().emit( upperLeft, lowerRight );
  */
}//voiddataExternallyChanged(...)


void NLSimpleDisplayModel::setDisplayedTimeRange( const PosixTime &begin,
                                                  const PosixTime &end )
{
  beginRemoveRows( WModelIndex(), 0, rowCount() );
  m_beginDisplayTime = m_endDisplayTime = kGenericT0;
  endRemoveRows();

  beginInsertRows( WModelIndex(), 0, rowCount( begin, end ) );
  m_endDisplayTime = end;
  m_beginDisplayTime = begin;
  endInsertRows();
}//void setDisplayedTimeRange(...)


WtGeneralArrayModel::WtGeneralArrayModel( const int nbin,
                                      const double *x_values,
                                      const double *y_values,
                                      Wt::WObject *parent )
  : WAbstractItemModel(parent), m_nbins(nbin), m_x(x_values), m_y(y_values)
{
}//WtGeneralArrayModel constructor

void WtGeneralArrayModel::setNBins( const int nbins )
{
  m_nbins = nbins;
  dataChanged().emit( index(0,0), index( rowCount(), columnCount() ) );
}

void WtGeneralArrayModel::setArrayAddresses( const int nbins,
                                           const double *x_values,
                                           const double *y_values )
{
  m_x = x_values;
  m_y = y_values;
  setNBins( nbins );
}//setArrayAddresses( ...)

int WtGeneralArrayModel::columnCount( const Wt::WModelIndex& ) const
{
   return 2;
}

int WtGeneralArrayModel::rowCount( const Wt::WModelIndex&  ) const
{
  return m_nbins;
}

WModelIndex WtGeneralArrayModel::parent( const Wt::WModelIndex& ) const
{
  return WModelIndex();
}

boost::any WtGeneralArrayModel::data( const Wt::WModelIndex& index, int ) const
{
  const int row = index.row();
  const int column = index.column();
  if( !m_x || !m_y || (column>1) || (row>=m_nbins) || (column<0) || (row<0) )
    return boost::any();

  if( 0 == column ) return boost::any( m_x[row] );
  return boost::any( m_y[row] );
}//data(...)

Wt::WModelIndex WtGeneralArrayModel::index( int row, int column, const Wt::WModelIndex& ) const
{
  if( !m_x || !m_y || (column>1) || (row>=m_nbins) || (column<0) || (row<0) )
    return WModelIndex();

  return WAbstractItemModel::createIndex( row, column, (void *)this );
}//index(...)


WtConsGraphModel::WtConsGraphModel( WtGui *app,
                                    NLSimple::DataGraphs column,
                                    Wt::WObject *parent )
  : WAbstractItemModel(parent), m_wApp(app), m_column( column )
{
}
int WtConsGraphModel::columnCount( const Wt::WModelIndex& ) const
{
  return 2;
}

int WtConsGraphModel::rowCount( const Wt::WModelIndex & ) const
{
  NLSimplePtr ptr( m_wApp );
  return static_cast<int>( ptr->dataGraph( m_column ).size() );
}

Wt::WModelIndex WtConsGraphModel::parent( const Wt::WModelIndex & ) const
{
  return WModelIndex();
}

boost::any WtConsGraphModel::data( const Wt::WModelIndex& index, int role ) const
{
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = ptr->dataGraph( m_column );

  if( role != Wt::DisplayRole ) return boost::any();

  const int row = index.row();
  const int column = index.column();
  const int size = static_cast<int>( g.size() );
  if( row >= size || column >= 2 || row < 0 || column < 0 )
    return boost::any();

  const GraphElement &el = g[row];
  if( column == 0 ) return boost::any( WDateTime::fromPosixTime(el.m_time) );

  if( m_column == NLSimple::kCustomEvents )
  {
    const NLSimple::EventDefMap &map = ptr->m_customEventDefs;
    const int key = TMath::Nint(el.m_value);
    const NLSimple::EventDefMap::const_iterator iter = map.find(key);
    if( iter == map.end() )  return boost::any();
    const string &name = iter->second.getName();
    return boost::any( WString(name) );
  }//if( m_column == NLSimple::kCustomEvents )

  return boost::any( el.m_value );
}//data()


WModelIndex WtConsGraphModel::index( int row, int column, const Wt::WModelIndex & ) const
{
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = ptr->dataGraph( m_column );

  const int size = static_cast<int>( g.size() );
  if( row >= size || column >= 2 || row < 0 || column < 0 )
    return WModelIndex();
  return WAbstractItemModel::createIndex( row, column, (void *)this );
}//index()

void WtConsGraphModel::refresh()
{
  beginRemoveRows	(	WModelIndex(), 0, rowCount() );
  endRemoveRows();
  beginInsertRows( WModelIndex(), 0, rowCount() );
  endInsertRows();
}//void WtConsGraphModel::refresh()


boost::any WtConsGraphModel::headerData( int section,
                                         Orientation orientation,
                                         int role ) const
{
  if( role == LevelRole ) return 0;
  if(orientation != Horizontal)
    return WAbstractItemModel::headerData( section, orientation, role );

  if( role != DisplayRole ) return WAbstractItemModel::headerData( section, orientation, role );
  if( section == 0 ) return boost::any( WString("Time") );

  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = ptr->dataGraph( m_column );
  return boost::any( WString( g.getGraphTypeStr() ) );
}//headerData


bool WtConsGraphModel::removeRows( int row, int count,
                                   const Wt::WModelIndex & )
{
  cerr << "WtConsGraphModel::removeRows...): Removing rows "
       << row << " to " << row+count-1 << endl;
  beginRemoveRows( WModelIndex(), row, row+count-1 );
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = ptr->dataGraph( m_column );

  vector<PosixTime> times;
  for( int r = row; r < (row+count); ++r ) times.push_back( g[r].m_time );
  foreach( const PosixTime &t, times ) g.erase( g.find( t ) );

  endRemoveRows();
  return true;
}//removeRows


bool WtConsGraphModel::removeRow( const boost::posix_time::ptime &t )
{
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = ptr->dataGraph( m_column );

  GraphIter pos = g.find( t );
  if( pos == g.end() ) return false;

  size_t row = pos - g.begin();

  beginRemoveRows( WModelIndex(), row, row-1 );
  g.erase( pos );
  endRemoveRows();
  return true;
}//removeRows


//Implementation of WtTimeRangeVecModel is incredably brief/dense since I plan
//  to rempliment all std::vector based models more elegantly
WtTimeRangeVecModel::WtTimeRangeVecModel( WtGui *app, Wt::WObject *parent )
  : WAbstractItemModel( parent ), m_wApp( app ){}
int WtTimeRangeVecModel::columnCount( const WModelIndex& ) const{ return 2; }
int WtTimeRangeVecModel::rowCount( const WModelIndex& ) const
{ return static_cast<int>( NLSimplePtr(m_wApp)->m_doNotUseTimeRanges.size() ); }
WModelIndex WtTimeRangeVecModel::parent( const WModelIndex& ) const { return WModelIndex(); }
boost::any WtTimeRangeVecModel::data( const Wt::WModelIndex& index, int role ) const
{
  if( role != Wt::DisplayRole ) return boost::any();
  NLSimplePtr ptr( m_wApp );
  const TimeRangeVec &g = ptr->m_doNotUseTimeRanges;
  const int row = index.row(), column = index.column();
  if( row < 0 || column < 0 || size_t(row) >= g.size() || column >= 2 ) return boost::any();
  if( column == 0 ) return boost::any( WDateTime::fromPosixTime(g[row].begin()) );
  return boost::any( WDateTime::fromPosixTime(g[row].end()) );
}//data
Wt::WModelIndex WtTimeRangeVecModel::index( int row, int column, const Wt::WModelIndex& ) const
{
  NLSimplePtr ptr( m_wApp );
  const TimeRangeVec &g = ptr->m_doNotUseTimeRanges;
  if( row < 0 || column < 0 || size_t(row) >= g.size() || column >= 2 ) return WModelIndex();
  return WAbstractItemModel::createIndex( row, column, (void *)this );
}//indsx(...)
boost::any WtTimeRangeVecModel::headerData( int section, Orientation orientation, int role  ) const
{
  if( role == LevelRole ) return 0;
  if( orientation != Horizontal || role != DisplayRole ) return WAbstractItemModel::headerData( section, orientation, role );
  if( !section ) return boost::any( WString("Start Time") );
  return boost::any( WString("End Time") );
}//headder data
bool WtTimeRangeVecModel::removeRows( int row, int count, const Wt::WModelIndex& )
{
  beginRemoveRows( WModelIndex(), row, row+count-1 );
  NLSimplePtr ptr( m_wApp );
  TimeRangeVec &g = ptr->m_doNotUseTimeRanges;

  TimeRangeVec times;
  for( int r = row; r < (row+count); ++r ) times.push_back( g[r] );
  for( size_t i=0; i<times.size(); ++i) g.erase( std::find( g.begin(), g.end(), times[i] ) );
  endRemoveRows();
  return true;
}//removeRows
bool WtTimeRangeVecModel::addRow( const PosixTime &start, const PosixTime &end )
{
  const TimeRange r(start,end);
  NLSimplePtr ptr( m_wApp );
  TimeRangeVec &g = ptr->m_doNotUseTimeRanges;
  TimeRangeVec::iterator pos = std::lower_bound( g.begin(), g.end(), r, &lessThan );
  if( pos!=g.end() && pos->begin()==start && pos->end()==end ) return false;
  beginInsertRows( WModelIndex(), pos-g.begin(), 1 );
  g.insert( pos, r );
  endInsertRows();
  return true;
}//addRow
void WtTimeRangeVecModel::refresh()
{
  beginRemoveRows	(	WModelIndex(), 0, rowCount() ); endRemoveRows();
  beginInsertRows( WModelIndex(), 0, rowCount() ); endInsertRows();
}//void refresh()


//Implementation of WtNotesVectorModel is incredably brief/dense since I plan
//  to rempliment all std::vector based models more elegantly
WtNotesVectorModel::WtNotesVectorModel( WtGui *app, WObject *parent )
    : WAbstractItemModel( parent ), m_wApp( app ){}
int WtNotesVectorModel::columnCount( const Wt::WModelIndex& ) const{ return 2; }
int WtNotesVectorModel::rowCount( const Wt::WModelIndex& ) const
{ return static_cast<int>( NLSimplePtr(m_wApp)->m_userNotes.size() ); }
WModelIndex WtNotesVectorModel::parent( const Wt::WModelIndex& ) const{ return WModelIndex(); }
boost::any WtNotesVectorModel::data( const Wt::WModelIndex& index, int role ) const
{
  if( role != Wt::DisplayRole ) return boost::any();
  NLSimplePtr ptr( m_wApp );
  const NLSimple::NotesVector &g = ptr->m_userNotes;
  const int row = index.row(), column = index.column();
  if( row < 0 || column < 0 || size_t(row) >= g.size() || column >= 2 ) return boost::any();
  if( column == 0 ) return boost::any( WDateTime::fromPosixTime(g[row].time) );
  string text = g[row].text;
  if( text.size() > sm_maxStrLen ) text = text.substr(0,sm_maxStrLen) + "...";
  return boost::any( WString(text) );
}//data
Wt::WModelIndex WtNotesVectorModel::index( int row, int column, const Wt::WModelIndex&) const
{
  NLSimplePtr ptr( m_wApp );
  const NLSimple::NotesVector &g = ptr->m_userNotes;
  if( row < 0 || column < 0 || size_t(row) >= g.size() || column >= 2 ) return WModelIndex();
  return WAbstractItemModel::createIndex( row, column, (void *)this );
}//indsx(...)
boost::any WtNotesVectorModel::headerData( int section, Orientation orientation, int role ) const
{
  if( role == LevelRole ) return 0;
  if( orientation != Horizontal || role != DisplayRole ) return WAbstractItemModel::headerData( section, orientation, role );
  if( !section ) return boost::any( WString("Time") );
  return boost::any( WString("Notes") );
}//headder data
bool WtNotesVectorModel::removeRows( int row, int count, const WModelIndex& )
{
  beginRemoveRows( WModelIndex(), row, row+count-1 );
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;

  NLSimple::NotesVector toDelete;
  for( int r = row; r < (row+count); ++r ) toDelete.push_back( g[r] );
  for( size_t i=0; i<toDelete.size(); ++i) g.erase( std::find( g.begin(), g.end(), toDelete[i] ) );
  endRemoveRows();
  return true;
}
size_t WtNotesVectorModel::vectorIndex( TimeTextPair *pos )
{
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  if( g.empty() ) return g.size();
  const TimeTextPair *start = &(g[0]);
  const TimeTextPair *last = &(g[g.size()-1]);
  assert( pos>=start && pos<=last );
  return pos-start;
}//size_t vectorIndex( TimeTextPair *pos )

Wt::WModelIndex WtNotesVectorModel::index( NLSimple::NotesVector::iterator iter )
{
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  if( g.empty() ) return WModelIndex();
  if( iter < g.begin() ) return WModelIndex();
  if( iter >= g.end() ) return WModelIndex();

  return index( iter-g.begin(), 0 );
}//WModelIndex index( NLSimple::NotesVector::iterator iter );
Wt::WModelIndex WtNotesVectorModel::index( TimeTextPair *ttptr )
{
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  const size_t ind = vectorIndex( ttptr );
  return index( g.begin() + ind );
}//Wt::WModelIndex WtNotesVectorModel::index( const TimeTextPair *ptr )
TimeTextPair *WtNotesVectorModel::dataPointer( const WModelIndex &index )
{
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  if( g.empty() ) return NULL;
  const int row = index.row();
  if( row < 0 || static_cast<size_t>(row)>=g.size() ) return NULL;
  return &(g[row]);
}//NLSimple::NotesVector *WtNotesVectorModel::dataPointer( const WModelIndex &index )
TimeTextPair *WtNotesVectorModel::find( const TimeTextPair &data )
{
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  NLSimple::NotesVector::iterator pos = std::find( g.begin(), g.end(), data );
  if( pos == g.end() ) return NULL;
  return &(*pos);
}//TimeTextPair *WtNotesVectorModel::find( const TimeTextPair &data )
bool WtNotesVectorModel::removeRow( TimeTextPair *toBeRemoved )
{
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  const size_t index = vectorIndex( toBeRemoved );

  beginRemoveRows( WModelIndex(), index, index );

  assert( index < g.size() );
  TimeTextPair removed = g[index];
  cerr << "WtNotesVectorModel::removeRow(): Removing index=" << index
       << " with time='" << removed.time << "' and text='" << removed.text
       << "'" << endl;
  g.erase( g.begin()+index );

  endRemoveRows();

  return true;
}//bool WtNotesVectorModel::removeRow( TimeTextPair *toBeRemoved )

NLSimple::NotesVector::iterator WtNotesVectorModel::addRow( const PosixTime &time, const std::string &text )
{
  const TimeTextPair r(time,text);
  NLSimplePtr ptr( m_wApp );
  NLSimple::NotesVector &g = ptr->m_userNotes;
  NLSimple::NotesVector::iterator pos = std::lower_bound( g.begin(), g.end(), r );
  if( pos!=g.end() && pos->time==time && pos->text==text ) return g.end();

  int newInd = pos-g.begin();

  cerr << "pos=" << pos-g.begin() << endl;

  beginInsertRows( WModelIndex(), newInd, 1 );

  g.insert( pos, r );

  endInsertRows();

  NLSimple::NotesVector::iterator newPos = std::find( g.begin(), g.end(), r );

  cerr << "newPos=" << newPos-g.begin() << endl;

  assert( newPos->text==text );
  assert( newPos->time==time );

  return newPos;
}
void WtNotesVectorModel::refresh()
{
  beginRemoveRows	(	WModelIndex(), 0, rowCount() ); endRemoveRows();
  beginInsertRows( WModelIndex(), 0, rowCount() ); endInsertRows();
}//void refresh()







