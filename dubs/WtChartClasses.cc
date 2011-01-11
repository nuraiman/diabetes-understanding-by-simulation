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

#include "WtChartClasses.hh"
#include "ConsentrationGraph.hh"
#include "ResponseModel.hh"
#include "ArtificialPancrease.hh"

using namespace Wt;
using namespace std;
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

const boost::posix_time::time_duration NLSimpleDisplayModel::sm_plasmaInsulinDt( 0, 5, 0 );  //5 minutes


WChartWithLegend::WChartWithLegend( WContainerWidget *parent ) :
    Chart::WCartesianChart(Wt::Chart::ScatterPlot,parent), m_width(0),
  m_height(0), m_legTopOffset(30), m_legRightOffset(245)
{
  setMinimumSize( 250, 250 );
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

  //set how far legend is from the top of the graph, in pixels
void WChartWithLegend::setLegendOffsetFromTop( const int &offset )
{
  m_legTopOffset = offset;
}

  //set how far legend is from the top of the graph, in pixels
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
  m_columnHeaderData.insert( m_columnHeaderData.begin(), NumColumns, HeaderData() );
  setHeaderData( TimeColumn,            Horizontal, WString("Time") );
  setHeaderData( CgmsData,              Horizontal, WString("CGMS Readings") );
  setHeaderData( FreePlasmaInsulin,     Horizontal, WString("Free Plasma Insulin (pred.)") );
  setHeaderData( GlucoseAbsorbtionRate, Horizontal, WString("Glucose Abs. Rate (pred.)") );
  setHeaderData( MealData,              Horizontal, WString("Consumed Carbohydrates") );
  setHeaderData( FingerMeterData,       Horizontal, WString("Finger Stick Readings") );
  setHeaderData( CustomEvents,          Horizontal, WString("User Defined Events") );
  setHeaderData( PredictedBloodGlucose, Horizontal, WString("Predicted Blood Glucose") );
  setHeaderData( PredictedInsulinX,     Horizontal, WString("Insulin X (pred.)") );
}//NLSimpleDisplayModel( constructor )


 NLSimpleDisplayModel::~NLSimpleDisplayModel()
 {
 }


 void NLSimpleDisplayModel::setDiabeticModel( NLSimple *diabeticModel )
 {
   const int nOldRow = rowCount();
   //const int nOldCol = columnCount();
   NLSimple *old = m_diabeticModel;

   m_diabeticModel = diabeticModel;
return;
   //WApplication::UpdateLock appLock( m_wApp ); //we can get a deadlock without this
   if( old )
   {
     //columnsRemoved	().emit( WModelIndex(), 0, nOldCol );
     rowsRemoved().emit( WModelIndex(), 0, nOldRow );
   }else columnsInserted().emit( WModelIndex(), 0, columnCount() );

   if( m_diabeticModel ) rowsInserted().emit( WModelIndex(), 0, rowCount() );
 }//void NLSimpleDisplayModel::setDiabeticModel( NLSimple *diabeticModel )


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

    size_t nRows = 0;

    for( Columns col = CgmsData; col < NumColumns; col = Columns(col+1) )
    {
      const ConsentrationGraph &cg = graph(col);
      const bool isFreePlasmaIsulin = ((col==FreePlasmaInsulin) && cg.size());

      if( !isFreePlasmaIsulin ) nRows += cg.size();
      else
      {
        ConstGraphIter iter = cg.upper_bound( kGenericT0 );
        while( iter != cg.end() )
        {
          ++nRows;
          iter = cg.upper_bound( iter->m_time + sm_plasmaInsulinDt );
        }//
      }//if( not FreePlasmaInsulin ) / else
    }//for( loop over col/concentration graphs )

    return static_cast<int>( nRows );
}//rowCount

int NLSimpleDisplayModel::columnCount( const Wt::WModelIndex &) const
{
  return NumColumns;
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
  if( columnWanted >= NumColumns ) return boost::any();

  size_t row_n = 0;
  const GraphElement *element = NULL;
  const size_t wantedRow = static_cast<size_t>( index.row() );

  for( Columns col = CgmsData; col < NumColumns; col = Columns(col+1) )
  {
    if( (columnWanted!=TimeColumn) && (col>columnWanted) ) return boost::any();

    const ConsentrationGraph &cg = graph(col);
    const bool isFreePlasmaIsulin = ((col==FreePlasmaInsulin) && cg.size());

    if( !isFreePlasmaIsulin )
    {
      if( (row_n+cg.size()) > wantedRow )
      {
        if( (columnWanted!=TimeColumn) && (col != columnWanted) )  return boost::any();
        assert( row_n <= wantedRow );
        ConsentrationGraph::const_iterator iter;
        for( iter = cg.begin(); row_n < wantedRow; ++iter, ++row_n ){assert( iter != cg.end() );}
        element = &(*iter);
        break;
      }else row_n += cg.size();
    }else
    {
      size_t nRows = 0;
      const size_t index = wantedRow - row_n;
      ConstGraphIter iter = cg.upper_bound( kGenericT0 );
      while( (iter != cg.end()) && (nRows < index) )
      {
        ++nRows;
        iter = cg.upper_bound( iter->m_time + sm_plasmaInsulinDt );
      }//
      if( iter != cg.end() ) element = &(*iter);
    }//if( !isFreePlasmaIsulin ) / else
  }//for( loop over graphs )

  if( !element ) return boost::any();

  if( columnWanted != TimeColumn ) return boost::any( element->m_value );
  else return boost::any( WDateTime::fromPosixTime( element->m_time ) );
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

  if( role == EditRole ) role = DisplayRole;
  m_columnHeaderData[section][role] = value;


  headerDataChanged().emit(orientation, section, section);
  return true;
}//bool NLSimpleDisplayModel::setHeaderData(...)



void NLSimpleDisplayModel::clear()
{
  m_columnHeaderData.clear();
  m_columnHeaderData.insert( m_columnHeaderData.begin(),
                             NumColumns,
                             HeaderData() );
  reset();
}//void NLSimpleDisplayModel::clear()

boost::any NLSimpleDisplayModel::headerData( int section,
                                             Orientation orientation,
                                             int role) const
{
  if( role == LevelRole ) return 0;

  if(orientation != Horizontal)
    return WAbstractItemModel::headerData( section, orientation, role );

  HeaderData::const_iterator i = m_columnHeaderData[section].find(role);

  return i != m_columnHeaderData[section].end() ? i->second : boost::any();
}//boost::any NLSimpleDisplayModel::headerData(...)
