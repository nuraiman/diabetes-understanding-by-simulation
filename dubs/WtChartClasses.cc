#include <algorithm>

#include "boost/any.hpp"
#include "boost/foreach.hpp"

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
#include "ArtificialPancrease.hh"

using namespace Wt;
using namespace std;
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

const boost::posix_time::time_duration NLSimpleDisplayModel::sm_plasmaInsulinDt( 0, 5, 0 );  //5 minutes


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








NLSimpleDisplayModel::NLSimpleDisplayModel( NLSimple *diabeticModel,
                                            WApplication *wapp,
                                            WObject *parent )
  : WAbstractItemModel( parent ),
    m_diabeticModel( diabeticModel ),
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


 void NLSimpleDisplayModel::setDiabeticModel( NLSimple *diabeticModel )
 {
   //const int nOldRow = rowCount();
   NLSimple *old = m_diabeticModel;
   m_diabeticModel = diabeticModel;

   if( m_diabeticModel )
   {
     foreach( ColumnDataPair &t, m_cachedData )
     {
       const bool isFreePlasmaIsulin = (t.first == FreePlasmaInsulin );
       const ConsentrationGraph &cg = graph( t.first );
       std::vector<GraphElement> &data = t.second;
       data.clear();

       if( !isFreePlasmaIsulin ) data.insert( data.begin(), cg.begin(), cg.end() );
       else
       {
         ConstGraphIter iter = cg.lower_bound( kGenericT0 );
         while( iter != cg.end() )
         {
           data.push_back( *iter );
           iter = cg.lower_bound( iter->m_time + sm_plasmaInsulinDt );
         }//
       }//if( not FreePlasmaInsulin ) / else
     }//foreach( row used )
   }//if( m_diabeticModel )

   WApplication::UpdateLock appLock( m_wApp ); //we can get a deadlock without this
   if( old )
   {
     // rowsRemoved().emit( WModelIndex(), 0, nOldRow );
   }else columnsInserted().emit( WModelIndex(), 0, columnCount() );

   if( m_diabeticModel ) rowsInserted().emit( WModelIndex(), 0, rowCount() );
 }//void NLSimpleDisplayModel::setDiabeticModel( NLSimple *diabeticModel )



ConsentrationGraph &NLSimpleDisplayModel::graphFromColumn( const int col )
{
  assert( col > 0 );
  assert( col < static_cast<int>(m_cachedData.size()) );

  const Columns colums = m_cachedData[col-1].first;

  return graph(colums);
}//ConsentrationGraph &NLSimpleDisplayModel::graphFromRow( const int row )



ConsentrationGraph &NLSimpleDisplayModel::graph( const Columns col )
{
  static ConsentrationGraph errorGraph (kGenericT0, TimeDuration(0,5,0), CustomEvent );
  if( !m_diabeticModel ) return errorGraph;

  switch( col )
  {
    case TimeColumn:            assert(0); //return errorGraph;
    case CgmsData:              return m_diabeticModel->m_cgmsData;
    case FreePlasmaInsulin:     return m_diabeticModel->m_freePlasmaInsulin;
    case GlucoseAbsorbtionRate: return m_diabeticModel->m_glucoseAbsorbtionRate;
    case MealData:              return m_diabeticModel->m_mealData;
    case FingerMeterData:       return m_diabeticModel->m_fingerMeterData;
    case CustomEvents:          return m_diabeticModel->m_customEvents;
    case PredictedInsulinX:     return m_diabeticModel->m_predictedInsulinX;
    case PredictedBloodGlucose: return m_diabeticModel->m_predictedBloodGlucose;
    case NumColumns:            assert(0); //return errorGraph;
  };//switch( col )

  return errorGraph;
}//ConsentrationGraph &graph( const Columns row )


const ConsentrationGraph &NLSimpleDisplayModel::graph( const Columns col ) const
{
   const static ConsentrationGraph errorGraph (kGenericT0, TimeDuration(0,5,0), CustomEvent );
   if( !m_diabeticModel ) return errorGraph;

   switch( col )
   {
     case TimeColumn:            assert(0); //return errorGraph;
     case CgmsData:              return m_diabeticModel->m_cgmsData;
     case FreePlasmaInsulin:     return m_diabeticModel->m_freePlasmaInsulin;
     case GlucoseAbsorbtionRate: return m_diabeticModel->m_glucoseAbsorbtionRate;
     case MealData:              return m_diabeticModel->m_mealData;
     case FingerMeterData:       return m_diabeticModel->m_fingerMeterData;
     case CustomEvents:          return m_diabeticModel->m_customEvents;
     case PredictedInsulinX:     return m_diabeticModel->m_predictedInsulinX;
     case PredictedBloodGlucose: return m_diabeticModel->m_predictedBloodGlucose;
     case NumColumns:            assert(0); //return errorGraph;
   };//switch( col )

   return errorGraph;
 }//ConsentrationGraph &graph( Columns row ) const

 int NLSimpleDisplayModel::rowCount( const Wt::WModelIndex & ) const
 {
   if( !m_diabeticModel ) return 0;
   if( m_cachedData.empty() ) return 0;

   size_t nRows = 0;
   foreach( const ColumnDataPair &t, m_cachedData ) nRows += t.second.size();

   return static_cast<int>( nRows );
 }//rowCount

int NLSimpleDisplayModel::columnCount( const Wt::WModelIndex &) const
{
  return static_cast<int>( m_cachedData.size() + 1 );
  //return NumColumns;
}//columnCount

Wt::WModelIndex NLSimpleDisplayModel::parent( const Wt::WModelIndex & ) const
{
  return WModelIndex();
}

boost::any NLSimpleDisplayModel::data( const WModelIndex& index, int role ) const
{
  //role = Wt::DisplayRole;
  //DisplayRole, DecorationRole, EditRole, StyleClassRole, CheckStateRole
  //ToolTipRole, InternalPathRole, UrlRole, LevelRole, MarkerPenColorRole
  //MarkerBrushColorRole, UserRole
  if( !m_diabeticModel ) return boost::any();
  if( role != Wt::DisplayRole ) return boost::any();

  const int columnWanted = index.column();
  if( columnWanted >= columnCount() ) return boost::any();

  const bool wantTime = (columnWanted==TimeColumn);
  const size_t wantedRow = static_cast<size_t>( index.row() );

  size_t row_n = 0;

  //foreach( const ColumnDataPair &t, m_cachedData )
  for( size_t col = 0; col < m_cachedData.size(); ++col )
  {
    const ColumnDataPair &t = m_cachedData[col];
    const vector<GraphElement> &data = t.second;
    const size_t dataSize = data.size();

    if( !wantTime && (row_n>wantedRow) ) return boost::any();

    const bool isCorrColl = ((col+1) == static_cast<size_t>(columnWanted));
    const bool inThisData = ((row_n+dataSize) > wantedRow);

    if( isCorrColl && inThisData )     return boost::any( data[wantedRow-row_n].m_value );
    else if( inThisData && !wantTime ) return boost::any();
    else if( inThisData && wantTime )  return boost::any( WDateTime::fromPosixTime(data[wantedRow-row_n].m_time) );
    row_n += dataSize;
  }//

  return boost::any();
}//boost::any data( const WModelIndex& index, int role ) const


WModelIndex NLSimpleDisplayModel::index( int row, int column, const WModelIndex & ) const
{
  if( !m_diabeticModel ) return WModelIndex();
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
  m_cachedData.clear();
  reset();
}//void NLSimpleDisplayModel::clear()


void NLSimpleDisplayModel::useColumn( Columns col )
{
  if( col == TimeColumn ) return;

  const ColumnDataPair val(col,vector<GraphElement>(0));
  //DataVec::const_iterator pos = find( m_cachedData.begin(), m_cachedData.end(), val );

  //if( pos == m_cachedData.end() )
  //{

    m_cachedData.push_back( val );
    m_columnHeaderData.push_back( HeaderData() );

    assert( m_cachedData.size() == m_columnHeaderData.size() );

    if( m_diabeticModel )
    {
      const ConsentrationGraph &cg = graph( col );
      m_cachedData.back().second = vector<GraphElement>(cg.begin(), cg.end());
    }//if( m_diabeticModel )

  //}

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


void NLSimpleDisplayModel::updateData()
{
  const int nOriginalRow = rowCount();
  foreach( ColumnDataPair &t, m_cachedData ) t.second.clear();

  if( !m_diabeticModel ) return;

  foreach( ColumnDataPair &t, m_cachedData )
  {
    const bool isFreePlasmaIsulin = (t.first == FreePlasmaInsulin );
    const ConsentrationGraph &cg = graph( t.first );
    std::vector<GraphElement> &data = t.second;
    data.clear();

    if( !isFreePlasmaIsulin ) data.insert( data.begin(), cg.begin(), cg.end() );
    else
    {
      ConstGraphIter iter = cg.lower_bound( kGenericT0 );
      while( iter != cg.end() )
      {
        data.push_back( *iter );
        iter = cg.lower_bound( iter->m_time + sm_plasmaInsulinDt );
      }//
    }//if( not FreePlasmaInsulin ) / else
  }//foreach( row used )

  WApplication::UpdateLock appLock( m_wApp ); //we can get a deadlock without this

  //should we emitt the rowsAboutToBeRemoved() signal as well?
  if( nOriginalRow > rowCount() )
    rowsRemoved().emit( WModelIndex(), rowCount(), nOriginalRow );
  else if( nOriginalRow < rowCount() )
    rowsInserted().emit( WModelIndex(), nOriginalRow, rowCount() );
  dataChanged().emit( index(0,0), index( rowCount(), columnCount() ) );
}//void NLSimpleDisplayModel::updateData()


void NLSimpleDisplayModel::dataExternallyChanged( Wt::WModelIndex upperLeft, Wt::WModelIndex lowerRight )
{
  //We need to avoid a recursive calling of the dataChanged.emit() function
  //  (eg if we hook two NLSimpleDisplayModel's together to keep in sync)
  //  so well just use a mutex that allows only one data change to happen
  //  at a time, I think as long as we only change the model in one thread
  //  at a time (like we should), all should be fine.
  boost::mutex::scoped_lock lock( m_dataBeingChangedMutex, boost::try_to_lock );
  if( lock.owns_lock() )  return;

  dataChanged().emit( upperLeft, lowerRight );
}//



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
