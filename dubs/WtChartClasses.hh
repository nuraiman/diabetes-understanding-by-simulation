#ifndef WTCHARTCLASSES_HH
#define WTCHARTCLASSES_HH

#include <algorithm>

#include <boost/any.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#include <Wt/Chart/WCartesianChart>
#include <Wt/Chart/WChart2DRenderer>
#include <Wt/WPainter>
#include <Wt/WRectF>
#include <Wt/WModelIndex>


#include "ConsentrationGraph.hh"


class NLSimple;

namespace Wt
{
  class WApplication;
}//namespace Wt

class WChartWithLegend: public Wt::Chart::WCartesianChart
{
protected:
  int m_width;           //The width of the chart in pixels
  int m_height;          //The height of the chart in pixels
  int m_legTopOffset;    //legend offset from top in pixels
  int m_legRightOffset;  //legend offset from right in pixels

public:
  WChartWithLegend( Wt::WContainerWidget *parent = NULL );
  virtual ~WChartWithLegend();

  int getHeight() const;
  int getWidth() const;
  void setLegendOffsetFromTop( const int &offset );   //in pixels
  void setLegendOffsetFromRight( const int &offset ); //in pixels
  void paint( Wt::WPainter& painter, const Wt::WRectF& rectangle = Wt::WRectF() ) const;
  void paintEvent( Wt::WPaintDevice *paintDevice );
};//class WChartWithLegend


class NLSimpleDisplayModel : public Wt::WAbstractItemModel
{
public:
  const static boost::posix_time::time_duration sm_plasmaInsulinDt;

  enum Columns
  {
    TimeColumn,
    CgmsData,
    GlucoseAbsorbtionRate,
    MealData,
    FingerMeterData,
    CustomEvents,
    PredictedInsulinX,
    PredictedBloodGlucose,
    FreePlasmaInsulin,
    NumColumns
  };//enum Columns

protected:
  NLSimple *m_diabeticModel;
  Wt::WApplication *m_wApp;

  typedef std::map<int, boost::any> HeaderData;
  std::vector<HeaderData> m_columnHeaderData;


public:
  NLSimpleDisplayModel( NLSimple *diabeticModel,
                      Wt::WApplication *wapp,
                      Wt::WObject *parent = 0 );
  ~NLSimpleDisplayModel();
  virtual int columnCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual int rowCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual Wt::WModelIndex parent( const Wt::WModelIndex& index) const;
  virtual boost::any data( const Wt::WModelIndex& index, int role = Wt::DisplayRole) const;
  virtual Wt::WModelIndex index( int row, int column, const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;


  virtual bool setHeaderData( int section, Wt::Orientation orientation,
                              const boost::any &value,
                              int role = Wt::EditRole );

  virtual boost::any headerData( int section,
                                 Wt::Orientation orientation = Wt::Horizontal,
                                 int role = Wt::DisplayRole ) const;
  void clear();

  void setDiabeticModel( NLSimple *diabeticModel );
  virtual const ConsentrationGraph &graph( const Columns row ) const;
};//class NLSimpleDisplayModel


#endif // WTCHARTCLASSES_HH
