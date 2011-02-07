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
#include "CgmsDataImport.hh" //for the CgmsDataImport::InfoType enum

class WtGui;
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

  WtGui *m_parentGui;

public:
  WChartWithLegend( WtGui *parentGui, Wt::WContainerWidget *parent = NULL );
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
  //Right now this class is a model that meets some of the purposes I wanted:
  //  I wanted to just use the NLSimple model to supply the data, but
  //  accessing any given element of a std::set is so slow this isn't
  //  practicle.
  //Right now the 0th axis is the time axis, and it is implicitly always
  //  defined (eg no entry in m_columnHeaderData or m_cachedData exists
  //  for it), and its header data cannot be set.
  //The upside to using this model is it's easy to only use the columns
  //  you want, and have them in the same order you add them in (using
  //  useColumn(Columns) ).
  //updateData() only emits the dataChanged() signal

public:
  const static boost::posix_time::time_duration sm_plasmaInsulinDt;  //15 minutes

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
  WtGui *m_wApp;

  typedef std::map<int, boost::any> HeaderData;
  std::vector<HeaderData> m_columnHeaderData;

  //typedef std::pair<Columns,std::vector<GraphElement> &> ColumnDataPair;
  //typedef std::vector<ColumnDataPair > DataVec;
  //DataVec m_cachedData;
  std::vector<Columns> m_columns;

  boost::mutex m_dataBeingChangedMutex;

public:
  NLSimpleDisplayModel(  WtGui *wapp, Wt::WObject *parent = 0 );
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

  void useAllColums();
  void useColumn( Columns col );

  virtual bool removeRows( int row, int count,
                           const Wt::WModelIndex& parent = Wt::WModelIndex() );

  //virtual bool insertRow(	int row, const WModelIndex & 	parent = WModelIndex() )	;
  virtual bool addData( const CgmsDataImport::InfoType,
                        const PosixTime &time,
                        const double &value );


  void aboutToSetNewModel();  //call before setting a new model (sends out notifcation all rows are being removed)
  void doneSettingNewModel(); //call after setting a new model (sends out notifcation rows have been added)

  int begginingRow( Columns col );  //the row of the zeroth element graph(col)

  //For graphFromColumn(), the first column added (with useColumn())
  //  cooresponds to column 1; therefore columNumber must be >=1
  virtual ConsentrationGraph &graphFromColumn( const int columNumber );

  static ConsentrationGraph &graph( NLSimple *model, const Columns row );
  static const ConsentrationGraph &graph( const NLSimple *model, const Columns row );

  virtual ConsentrationGraph &graph( const Columns row );
  virtual const ConsentrationGraph &graph( const Columns row ) const;

  void updateData();
  void dataExternallyChanged();

};//class NLSimpleDisplayModel


class WtGeneralArrayModel : public Wt::WAbstractItemModel
{
  //This class is meant to display the class EventDef in ResponseModel.hh
  //  and will likely change to be either vector or TSpline3 based in the
  //  future.
  //This class is also not thread safe or blah-blah-blah

  int m_nbins;
  const double *m_x;
  const double *m_y;

public:
  WtGeneralArrayModel( const int nbin,
                     const double *x_values,
                     const double *y_values, Wt::WObject *parent = 0  );
  void setNBins( const int nbins );
  void setArrayAddresses( const int nbins, const double *x_values, const double *y_values );

  virtual int columnCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual int rowCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual Wt::WModelIndex parent( const Wt::WModelIndex& index) const;
  virtual boost::any data( const Wt::WModelIndex& index, int role = Wt::DisplayRole) const;
  virtual Wt::WModelIndex index( int row, int column, const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
};//class WtGeneralArrayModel



class WtConsGraphModel : public Wt::WAbstractItemModel
{

  WtGui *m_wApp;
  NLSimpleDisplayModel::Columns m_column;

public:
  WtConsGraphModel( WtGui *app, NLSimpleDisplayModel::Columns,
                    Wt::WObject *parent = 0  );
  virtual int columnCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual int rowCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual Wt::WModelIndex parent( const Wt::WModelIndex& index) const;
  virtual boost::any data( const Wt::WModelIndex& index, int role = Wt::DisplayRole) const;
  virtual Wt::WModelIndex index( int row, int column, const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual boost::any headerData( int section,
                                 Wt::Orientation orientation = Wt::Horizontal,
                                 int role = Wt::DisplayRole ) const;
  virtual bool removeRows( int row, int count,
                           const Wt::WModelIndex& parent = Wt::WModelIndex() );
  virtual void refresh();
  NLSimpleDisplayModel::Columns type() const { return m_column; }
};//class WtGeneralArrayModel


#endif // WTCHARTCLASSES_HH
