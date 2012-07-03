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
#include "ResponseModel.hh"

class WtGui;


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
  void setWtResizeJsForOverlay();
  void setLegendOffsetFromTop( const int &offset );   //in pixels
  void setLegendOffsetFromRight( const int &offset ); //in pixels
  virtual void paint( Wt::WPainter& painter, const Wt::WRectF& rectangle = Wt::WRectF() ) const;
  virtual void paintEvent( Wt::WPaintDevice *paintDevice );
};//class WChartWithLegend


class NLSimpleDisplayModel : public Wt::WAbstractItemModel
{
  //Right now this class is a model that meets some of the purposes I wanted:
  //Right now the last column is the time axis,
  //  and it is implicitly always defined (eg no entry in m_columnHeaderData
  //  or m_cachedData exists for it), and its header data cannot be set.
  //The upside to using this model is it's easy to only use the columns
  //  you want, and have them in the same order you add them in (using
  //  useColumn(NLSimple::DataGraphs) ).
  //updateData() only emits the dataChanged() signal

public:
  const static boost::posix_time::time_duration sm_plasmaInsulinDt;  //15 minutes

  typedef NLSimple::DataGraphs DataGraphs;
  //NLSimple::kNumDataGraphs is used for the time series

protected:
  WtGui *m_wApp;

  typedef std::map<int, boost::any> HeaderData;
  std::vector<HeaderData> m_columnHeaderData;

  std::vector<NLSimple::DataGraphs> m_columns;

  PosixTime m_beginDisplayTime;
  PosixTime m_endDisplayTime;


  boost::mutex m_dataBeingChangedMutex;

public:
  NLSimpleDisplayModel(  WtGui *wapp, Wt::WObject *parent = 0 );
  ~NLSimpleDisplayModel();
  virtual int columnCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual int rowCount( const PosixTime &start, const PosixTime &end ) const;
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
  void useColumn( NLSimple::DataGraphs col );

  virtual bool removeRows( int row, int count,
                           const Wt::WModelIndex& parent = Wt::WModelIndex() );

  //virtual bool insertRow(	int row, const WModelIndex & 	parent = WModelIndex() )	;
  virtual bool addData( const CgmsDataImport::InfoType,
                        const PosixTime &time,
                        const double &value );

  NLSimple::DataGraphs dataType( const int row ) const;

  void aboutToSetNewModel();  //call before setting a new model (sends out notifcation all rows are being removed)
  void doneSettingNewModel(); //call after setting a new model (sends out notifcation rows have been added)

  int begginingRow( NLSimple::DataGraphs col );  //the row of the zeroth element graph(col)

//  void updateData();
  void dataExternallyChanged();

  const PosixTime &beginDisplayTime() const { return m_beginDisplayTime; }
  const PosixTime &endDisplayTime() const { return m_endDisplayTime; }


  PosixTime earliestData() const;
  PosixTime latestData() const;


  void setDisplayedTimeRange( const PosixTime &begin, const PosixTime &end );
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
  NLSimple::DataGraphs m_column;

public:
  WtConsGraphModel( WtGui *app, NLSimple::DataGraphs, Wt::WObject *parent = 0  );
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
  bool removeRow( const boost::posix_time::ptime &t );
  virtual void refresh();
  NLSimple::DataGraphs type() const { return m_column; }
};//class WtGeneralArrayModel


//TODO: WtConsGraphModel WtNotesVectorModel (and maybe WtTimeRangeVecModel)
//      could be combined into one (templated class) via something like the
//      following but where the data() funtion is template specialized
// template <class vectorType>
//class WtVectorModel : public Wt::WAbstractItemModel
//{
//  WtGui *m_wApp;
//  ptrdiff_t m_mOffset;
 //..
//};//class WtVectorModel : public Wt::WAbstractItemModel


class WtTimeRangeVecModel : public Wt::WAbstractItemModel
{
  //A class to show a NLSimple::m_doNotUseTimeRanges object
  //  that is of type std::vector<boost::posix_time::time_period>
  WtGui *m_wApp;

public:
  WtTimeRangeVecModel( WtGui *app, Wt::WObject *parent = 0  );
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
  bool addRow( const PosixTime &start, const PosixTime &end );
  virtual void refresh();
};//class WtTimeRangeVecModel



class WtNotesVectorModel : public Wt::WAbstractItemModel
{
  //A class to show/add-to/edit a NLSimple::m_userNotes object that
  //  is of type std::vector<TimeTextPair>
  WtGui *m_wApp;

  static const size_t sm_maxStrLen = 25;

public:
  WtNotesVectorModel( WtGui *app, Wt::WObject *parent = 0  );
  virtual int columnCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual int rowCount( const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  virtual Wt::WModelIndex parent( const Wt::WModelIndex& index) const;
  virtual boost::any data( const Wt::WModelIndex& index, int role = Wt::DisplayRole) const;
  virtual Wt::WModelIndex index( int row, int column, const Wt::WModelIndex& parent = Wt::WModelIndex() ) const;
  size_t vectorIndex( TimeTextPair *pos );
  Wt::WModelIndex index( TimeTextPair *ptr );
  Wt::WModelIndex index( NLSimple::NotesVector::iterator iter );
  virtual boost::any headerData( int section,
                                 Wt::Orientation orientation = Wt::Horizontal,
                                 int role = Wt::DisplayRole ) const;
  virtual bool removeRows( int row, int count,
                           const Wt::WModelIndex& parent = Wt::WModelIndex() );
  bool removeRow( TimeTextPair *toBeRemoved );
  TimeTextPair *find( const TimeTextPair &data );
  TimeTextPair *dataPointer( const Wt::WModelIndex &index );
  NLSimple::NotesVector::iterator addRow( const PosixTime &time, const std::string &text );
  virtual void refresh();
};//class WtNotesVectorModel


#endif // WTCHARTCLASSES_HH
