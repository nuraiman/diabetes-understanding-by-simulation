#ifndef WTUTILS_HH
#define WTUTILS_HH

#include "DubsConfig.hh"

#include <string>
#include <vector>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <Wt/WSpinBox>
#include <Wt/WDateTime>
#include <Wt/WJavaScript>
#include <Wt/WApplication>
#include <Wt/WDoubleSpinBox>
#include <Wt/WContainerWidget>

#include "dubs/ArtificialPancrease.hh"


namespace Wt
{
  class WText;
  class WLineF;
  class WRectF;
  class WDialog;
  class WComboBox;
  class WLineEdit;
  class WDateTime;
  class WPopupMenu;
  class WTableView;
  class WTabWidget;
  class WDatePicker;
  class WBorderLayout;
  class WStandardItemModel;
}//namespace Wt


class Div : public Wt::WContainerWidget
{
  public:
  Div( const std::string &styleClass = "",
       Wt::WContainerWidget *parent = NULL );
  virtual ~Div() {}
};//class Div : Wt::WContainerWidget

//Right now I am having trouble using JSlot's to communicate from the server to
//  the client to set the time or timeselect options; it seems to work fine
//  using doJavaScript(...), but I would preffer using JSlot's
#define USE_JSLOTS_WITH_DateTimeSelect 0

class DateTimeSelect : public Wt::WContainerWidget
{
protected:
  Wt::WApplication *m_app;
  Wt::WLineEdit *m_lineEdit;

  boost::scoped_ptr< Wt::JSignal<std::string> > m_timeSelected;  //string is in format '07/23/2010 12:15:00 pm'
  boost::scoped_ptr< Wt::JSignal<std::string> > m_sectionBoxClosed;  //string is in format '07/23/2010 12:15:00 pm'

#if( USE_JSLOTS_WITH_DateTimeSelect )
  boost::scoped_ptr<Wt::JSlot> m_setTopSlot;
  boost::scoped_ptr<Wt::JSlot> m_setBottomSlot;
  boost::scoped_ptr<Wt::JSlot> m_setDateTimeSlot;
#endif //USE_JSLOTS_WITH_DateTimeSelect

  Wt::WDateTime    m_top;
  Wt::WDateTime    m_bottom;
  Wt::WDateTime    m_dateTime;
  Wt::Signal<>     m_topBottomChanged;
  Wt::Signal<Wt::WDateTime> m_changed;

  void validate( bool emitChanged );
  void changedCallback( const std::string &datTimeText );

public:
  //If no initial time is given, will be set to current time
  DateTimeSelect( const std::string &label,
                  const Wt::WDateTime &initialTime = Wt::WDateTime(),
                  Wt::WContainerWidget *parent = NULL );

  virtual ~DateTimeSelect();

  //In the furuture if I make formatiing of date/time optional, dateTimeFromStr()
  //  and dateTimeToStr() will obey this formatting.

  //dateTimeFromStr(...) expects input like '07/23/2010 12:15:00 pm'
  Wt::WDateTime dateTimeFromStr( const std::string &datetime ) const;

  //dateTimeToStr(...) returns answer in format like '07/23/2010 12:15:00 pm'
  std::string dateTimeToStr( const Wt::WDateTime &datetime ) const;

  static std::string jsForDate( const Wt::WDateTime &datetime );

  void set( const Wt::WDateTime &dateTime );
  void setToCurrentTime();
  Wt::WDateTime dateTime() const;
  void setTop( const Wt::WDateTime &top );
  void setBottom( const Wt::WDateTime &bottom );
  const Wt::WDateTime &top() const;
  const Wt::WDateTime &bottom() const;


  Wt::Signal<Wt::WDateTime> &changed();
  Wt::Signal<> &topBottomUpdated();
};//class DateTimeSelect


class MemVariableSpinBox : public Wt::WContainerWidget
{
protected:
  typedef boost::recursive_mutex::scoped_lock Lock;
  typedef boost::shared_ptr<Lock> LockShrdPtr;

  Wt::WDoubleSpinBox *m_spinBox;
  Wt::WApplication *m_app;

  LockShrdPtr getLock();

public:
  MemVariableSpinBox( const std::string &label = "",
                      const std::string &units = "",
                      const double &lower = 0.0,
                      const double &upper = 0.0,
                      Wt::WContainerWidget *parent=0 );
  virtual ~MemVariableSpinBox(){}
  virtual void updateGuiFromMemmory() = 0;
  virtual void updateMemmoryFromGui() = 0;
  Wt::WDoubleSpinBox *spinBox() { return m_spinBox; }
  Wt::Signal<double> &valueChanged();
  virtual double value();
};//class MemVariableSpinBox


class IntSpinBox : public MemVariableSpinBox
{
  int *m_memVariable;

public:
  IntSpinBox( int *memVariable,
              const std::string &label = "",
              const std::string &units = "",
              const double &lower = 0.0,
              const double &upper = 0.0,
              Wt::WContainerWidget *parent = 0 );
  virtual void updateGuiFromMemmory();
  virtual void updateMemmoryFromGui();
};//class IntSpinBox


class DoubleSpinBox : public MemVariableSpinBox
{
  double *m_memVariable;

public:
  DoubleSpinBox( double *memVariable,
                 const std::string &label = "",
                 const std::string &units = "",
                 const double &lower = 0.0,
                 const double &upper = 0.0,
                 Wt::WContainerWidget *parent = 0 );
  virtual void updateGuiFromMemmory();
  virtual void updateMemmoryFromGui();
};//class DoubleSpinBox


class TimeDurationSpinBox : public MemVariableSpinBox
{
  TimeDuration *m_memVariable;

public:
  TimeDurationSpinBox( TimeDuration *memVariable,
                       const std::string &label = "",
                       const double &lower = 0.0,
                       const double &upper = 0.0,
                       Wt::WContainerWidget *parent = 0 );
  virtual void updateGuiFromMemmory();
  virtual void updateMemmoryFromGui();
};//class TimeDurationSpinBox

class MemGuiTimeDate : public DateTimeSelect
{
  boost::posix_time::ptime *m_memVariable;

public:
  MemGuiTimeDate( boost::posix_time::ptime *memVariable,
                  const std::string &label = "",
                  const boost::posix_time::ptime &lower = boost::posix_time::ptime(boost::posix_time::min_date_time),
                  const boost::posix_time::ptime &upper = boost::posix_time::ptime(boost::posix_time::max_date_time),
                  Wt::WContainerWidget *parent = 0 );
  virtual void updateGuiFromMemmory();
  virtual void updateMemmoryFromGui();
  double asNumber() const;
  boost::posix_time::ptime currentValue() const;
};//class TimeDurationSpinBox


#endif // WTUTILS_HH
