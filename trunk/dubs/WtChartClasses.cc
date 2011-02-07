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

#include "TMath.h"

#include "WtGui.hh"
#include "WtChartClasses.hh"
#include "ConsentrationGraph.hh"
#include "ResponseModel.hh"
#include "CgmsDataImport.hh"
#include "ArtificialPancrease.hh"

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

    foreach( const GraphElement &el, modelPtr->m_customEvents )
    {
      WPointF pos = mapToDevice( WDateTime::fromPosixTime(el.m_time), ypos );
      WRectF loc( pos.x(), pos.y(), 0.1, 0.1 );
      painter.drawText( loc, AlignLeft, WString(Form("%i", TMath::Nint(el.m_value))) );
    }//foreach custom event


    painter.setPen( palette()->strokePen( WtGui::kFreePlasmaInsulin ) );
    foreach( const GraphElement &el, modelPtr->m_boluses )
    {
      WPointF pos = mapToDevice( WDateTime::fromPosixTime(el.m_time), 5*el.m_value );
      //WRectF loc( pos.x(), pos.y(), 0.1, 0.1 );
      painter.drawImage( pos, WPainter::Image("local_resources/syringe.png","local_resources/syringe.png") );
      //painter.drawText( loc, AlignLeft, WString(Form("%i", TMath::Nint(el.m_value))) );
    }//foreach( const GraphElement &el, modelPtr->m_customEvents )

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
    m_wApp( wapp )
{
}//NLSimpleDisplayModel( constructor )


void NLSimpleDisplayModel::useAllColums()
{
  for( Columns col = Columns(1); col < NumColumns; col = Columns(col+1) ) useColumn(col);
}

 NLSimpleDisplayModel::~NLSimpleDisplayModel()
 {
 }




ConsentrationGraph &NLSimpleDisplayModel::graphFromColumn( const int col )
{
  assert( col > 0 );
  assert( col <= static_cast<int>(m_columns.size()) );

  return graph( m_columns[col-1] );
}//ConsentrationGraph &NLSimpleDisplayModel::graphFromRow( const int row )


ConsentrationGraph &NLSimpleDisplayModel::graph( NLSimple *diabeticModel, const Columns col )
{
  switch( col )
  {
    case TimeColumn:            assert(0); //return errorGraph;
    case CgmsData:              return diabeticModel->m_cgmsData;
    case FreePlasmaInsulin:     return diabeticModel->m_freePlasmaInsulin;
    case GlucoseAbsorbtionRate: return diabeticModel->m_glucoseAbsorbtionRate;
    case MealData:              return diabeticModel->m_mealData;
    case FingerMeterData:       return diabeticModel->m_fingerMeterData;
    case CustomEvents:          return diabeticModel->m_customEvents;
    case PredictedInsulinX:     return diabeticModel->m_predictedInsulinX;
    case PredictedBloodGlucose: return diabeticModel->m_predictedBloodGlucose;
    case NumColumns:            assert(0); //return errorGraph;
  };//switch( col )

  assert(0);
  static ConsentrationGraph errorGraph (kGenericT0, TimeDuration(0,5,0), CustomEvent );
  return errorGraph;
}//graph



const ConsentrationGraph &NLSimpleDisplayModel::graph( const NLSimple *diabeticModel, const Columns col )
{
  switch( col )
  {
    case TimeColumn:            assert(0); //return errorGraph;
    case CgmsData:              return diabeticModel->m_cgmsData;
    case FreePlasmaInsulin:     return diabeticModel->m_freePlasmaInsulin;
    case GlucoseAbsorbtionRate: return diabeticModel->m_glucoseAbsorbtionRate;
    case MealData:              return diabeticModel->m_mealData;
    case FingerMeterData:       return diabeticModel->m_fingerMeterData;
    case CustomEvents:          return diabeticModel->m_customEvents;
    case PredictedInsulinX:     return diabeticModel->m_predictedInsulinX;
    case PredictedBloodGlucose: return diabeticModel->m_predictedBloodGlucose;
    case NumColumns:            assert(0); //return errorGraph;
  };//switch( col )

  assert(0);
  static ConsentrationGraph errorGraph (kGenericT0, TimeDuration(0,5,0), CustomEvent );
  return errorGraph;
}//graph


ConsentrationGraph &NLSimpleDisplayModel::graph( const Columns col )
{
  NLSimplePtr diabeticModel( m_wApp );
  return graph( diabeticModel.get(), col );
}//ConsentrationGraph &graph( const Columns row )


const ConsentrationGraph &NLSimpleDisplayModel::graph( const Columns col ) const
{
   NLSimplePtr diabeticModel( m_wApp );
   return graph( diabeticModel.get(), col );
 }//ConsentrationGraph &graph( Columns row ) const


 int NLSimpleDisplayModel::rowCount( const Wt::WModelIndex & ) const
 {
   NLSimplePtr diabeticModel( m_wApp );
   if( !diabeticModel ) return 0;
   if( m_columns.empty() ) return 0;

   size_t nRows = 0;
   foreach( const Columns &t, m_columns )
   {
     const ConsentrationGraph &g = graph(t);
     if( g.empty() ) continue;

     if( t != FreePlasmaInsulin ) nRows += g.size();
     else
     {
       nRows += (g.getEndTime() - g.getStartTime()).total_seconds()
                / sm_plasmaInsulinDt.total_seconds();
     }//if( t != FreePlasmaInsulin )

   }//foreach( const Columns &t, m_columns )

   return static_cast<int>( nRows );
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
  NLSimplePtr diabeticModel( m_wApp );

  if( !diabeticModel ) return boost::any();
  if( role != Wt::DisplayRole ) return boost::any();

  const int columnWanted = index.column();
  if( columnWanted >= columnCount() ) return boost::any();

  const bool wantTime = (columnWanted==TimeColumn);
  const size_t wantedRow = static_cast<size_t>( index.row() );

  size_t row_n = 0;

  for( size_t col = 0; col < m_columns.size(); ++col )
  {
    const ConsentrationGraph &data = graph( m_columns[col] );
    const Columns &column = m_columns[col];
    size_t dataSize = data.size();
    if( !wantTime && (row_n>wantedRow) ) return boost::any();
    const bool isCorrColl = ((col+1) == static_cast<size_t>(columnWanted));

    if( column==FreePlasmaInsulin )
    {
      const PosixTime t = data.getStartTime() + sm_plasmaInsulinDt*(wantedRow-row_n);
      const bool inThisData = (t <= data.getEndTime());
      if( isCorrColl && inThisData )
      {
        const double value = data.value( t );
        if( TMath::AreEqualAbs( 0.0, value, 0.00001 ) ) return boost::any();
        return boost::any( value );
      }else if( inThisData && !wantTime ) return boost::any();
      else if( inThisData && wantTime )  return boost::any( WDateTime::fromPosixTime(t) );

      dataSize = (data.getEndTime() - data.getStartTime()).total_seconds()
                 / sm_plasmaInsulinDt.total_seconds();
      row_n += dataSize;
      continue;
    }//if( (column == FreePlasmaInsulin) )


    const bool inThisData = ((row_n+dataSize) > wantedRow);

    if( isCorrColl && inThisData )     return boost::any( data[wantedRow-row_n].m_value );
    else if( inThisData && !wantTime ) return boost::any();
    else if( inThisData && wantTime )  return boost::any( WDateTime::fromPosixTime(data[wantedRow-row_n].m_time) );
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

  if( section == TimeColumn ) return true;

  if( role == EditRole ) role = DisplayRole;
  m_columnHeaderData[section-1][role] = value;

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


int NLSimpleDisplayModel::begginingRow( NLSimpleDisplayModel::Columns wantedColumn )
{
  if( wantedColumn == TimeColumn )
  {
    cerr << "You shouldn't be calling NLSimpleDisplayModel::begginingRow( Columns col )"
         << " for the TimeColumn!" << endl;
    return 0;
  }//if( col == TimeColumn )


  size_t row_n = 0;

  for( size_t col = 0; col < m_columns.size(); ++col )
  {
    const Columns &column = m_columns[col];
    const bool isCorrColl = (column==wantedColumn);
    if( isCorrColl ) return row_n;

    const bool isScaledColum = (column==FreePlasmaInsulin);
    ConsentrationGraph &data = graph( column );

    const size_t dataSize = !isScaledColum ? data.size()
                            : ((data.getEndTime() - data.getStartTime()).total_seconds()
                                  / sm_plasmaInsulinDt.total_seconds());
    row_n += dataSize;
  }//for( loop over columns )

  cerr << "NLSimpleDisplayModel::begginingRow( Columns col ): shouldn't have "
       << "made it to the end :(" << endl;
  return 0;
}//int begginingRow( Columns col )



bool NLSimpleDisplayModel::addData( const CgmsDataImport::InfoType type,
                                    const PosixTime &time,
                                    const double &value )
{
  using namespace CgmsDataImport;
  NLSimplePtr modelPtr( m_wApp );

  Columns col = TimeColumn;
  switch( type )
  {
    case CgmsReading:      col = CgmsData; break;
    case MeterReading:     col = FingerMeterData; break;
    case MeterCalibration: col = FingerMeterData; break;
    case GlucoseEaten:     col = MealData; break;
    case BolusTaken:       col = FreePlasmaInsulin; break;
    case GenericEvent:     col = CustomEvents; break;
    case ISig: assert(0); break;
  };//switch( det )

  assert( col != TimeColumn );

  const int beginRow = begginingRow( col );
  ConsentrationGraph &g = graph( col );
  GraphIter pos = g.lower_bound( time );
  const int row = beginRow + pos - g.begin();

  beginInsertRows( WModelIndex(), row, row );
  cerr << "Beggining to insert row " << row << " into " << col << " data" << endl;

  switch( type )
  {
    case CgmsReading:      modelPtr->addCgmsData( time, value );        break;
    case MeterReading:     modelPtr->addFingerStickData( time, value ); break;
    case MeterCalibration: modelPtr->addFingerStickData( time, value ); break;
    case GlucoseEaten:     modelPtr->addConsumedGlucose( time, value ); break;
    case BolusTaken:       modelPtr->addBolusData( time, value );       break;
    case GenericEvent:     modelPtr->addCustomEvent( time, value );     break;
    case ISig: assert(0);                           break;
  };//switch( det )

  cerr << "Done inserting row " << row << " into " << col << " data" << endl;

  endInsertRows();

  return true;
}//add data()


bool NLSimpleDisplayModel::removeRows( int row, int count, const WModelIndex & )
{
  size_t row_n = 0;

  for( size_t col = 0; col < m_columns.size(); ++col )
  {
    const Columns &column = m_columns[col];
    const bool isScaledColum = (column==FreePlasmaInsulin);
    ConsentrationGraph &data = graph( column );

    const size_t dataSize = !isScaledColum ? data.size()
                            : ((data.getEndTime() - data.getStartTime()).total_seconds()
                                  / sm_plasmaInsulinDt.total_seconds());

    const bool inThisData = ((row_n+dataSize) > static_cast<size_t>(row));

    if( inThisData && isScaledColum )
    {
      cerr << "Sourry cant remove data from Free plasma insulin :(" << endl;
      return false;
    }

    if( inThisData )
    {
      int localRow = row - row_n;
      GraphIter first = data.begin() + localRow;
      GraphIter last = data.end();
      if( (localRow+count) <= static_cast<int>(data.size()) )
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



void NLSimpleDisplayModel::useColumn( Columns col )
{
  if( col == TimeColumn ) return;

  m_columns.push_back( col );
  m_columnHeaderData.push_back( HeaderData() );

  WString title;
  switch( col )
  {
    case TimeColumn:            title = "Time";                       break;
    case CgmsData:              title = "CGMS Readings";              break;
    case FreePlasmaInsulin:     title = "Free Plasma Insulin (pred.)";break;
    case GlucoseAbsorbtionRate: title = "Glucose Abs. Rate (pred.)";  break;
    case MealData:              title = "Consumed Carbohydrates";     break;
    case FingerMeterData:       title = "Finger Stick Readings";      break;
    case CustomEvents:          title = "User Defined Events";        break;
    case PredictedBloodGlucose: title = "Predicted Blood Glucose";    break;
    case PredictedInsulinX:     title = "Insulin X (pred.)";          break;
    case NumColumns: break;
  };//switch( col )

  setHeaderData( m_columnHeaderData.size(), Horizontal, title );
}//void NLSimpleDisplayModel::useColumn( Columns col )



boost::any NLSimpleDisplayModel::headerData( int section,
                                             Orientation orientation,
                                             int role) const
{
  if( role == LevelRole ) return 0;

  if(orientation != Horizontal)
    return WAbstractItemModel::headerData( section, orientation, role );

  if( section == TimeColumn ) return boost::any( WString("Time") );

  assert( static_cast<size_t>(section-1) < m_columnHeaderData.size() );
  const HeaderData &info = m_columnHeaderData[section-1];

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
                                    NLSimpleDisplayModel::Columns column,
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
  ConsentrationGraph &g = NLSimpleDisplayModel::graph( ptr.get(), m_column );
  return static_cast<int>( g.size() );
}

Wt::WModelIndex WtConsGraphModel::parent( const Wt::WModelIndex & ) const
{
  return WModelIndex();
}

boost::any WtConsGraphModel::data( const Wt::WModelIndex& index, int role ) const
{
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = NLSimpleDisplayModel::graph( ptr.get(), m_column );

  if( role != Wt::DisplayRole ) return boost::any();

  const int row = index.row();
  const int column = index.column();
  const int size = static_cast<int>( g.size() );
  if( row >= size || column >= 2 || row < 0 || column < 0 )
    return boost::any();

  const GraphElement &el = g[row];
  if( column == 0 ) return boost::any( WDateTime::fromPosixTime(el.m_time) );
  return boost::any( el.m_value );
}//data()

WModelIndex WtConsGraphModel::index( int row, int column, const Wt::WModelIndex & ) const
{
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = NLSimpleDisplayModel::graph( ptr.get(), m_column );

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
  ConsentrationGraph &g = NLSimpleDisplayModel::graph( ptr.get(), m_column );
  return boost::any( WString( g.getGraphTypeStr() ) );
}//headerData


bool WtConsGraphModel::removeRows( int row, int count,
                                   const Wt::WModelIndex & )
{
  cerr << "WtConsGraphModel::removeRows...): Removing rows "
       << row << " to " << row+count-1 << endl;
  beginRemoveRows( WModelIndex(), row, row+count-1 );
  NLSimplePtr ptr( m_wApp );
  ConsentrationGraph &g = NLSimpleDisplayModel::graph( ptr.get(), m_column );
  g.erase( g.begin()+row, g.begin()+row+count );
  endRemoveRows();
  return true;
}//removeRows


