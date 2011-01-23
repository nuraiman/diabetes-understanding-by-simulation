#ifndef WTUTILS_HH
#define WTUTILS_HH
#include <string>
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/thread/recursive_mutex.hpp>
#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WDateTime>

#include <Wt/WSpinBox>
#include "ArtificialPancrease.hh"


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
};//namespace Wt


class Div : public Wt::WContainerWidget
{
  public:
  Div( const std::string &styleClass = "",
       Wt::WContainerWidget *parent = NULL );
  virtual ~Div() {}
};//class Div : Wt::WContainerWidget



class DateTimeSelect : public Wt::WContainerWidget
{
  Wt::WDatePicker *m_datePicker;
  Wt::WSpinBox    *m_hourSelect;
  Wt::WSpinBox    *m_minuteSelect;
  Wt::WDateTime    m_top;
  Wt::WDateTime    m_bottom;
  Wt::Signal<>     m_changed;
  Wt::Signal<>     m_topBottomChanged;

  void validate( bool emitChanged = false );

public:
  DateTimeSelect( const std::string &label,
                  const Wt::WDateTime &initialTime,
                  Wt::WContainerWidget *parent = NULL );
  virtual ~DateTimeSelect();

  void set( const Wt::WDateTime &dateTime );
  Wt::WDateTime dateTime() const;
  void setTop( const Wt::WDateTime &top );
  void setBottom( const Wt::WDateTime &bottom );
  const Wt::WDateTime &top() const;
  const Wt::WDateTime &bottom() const;

  Wt::Signal<> &changed();
  Wt::Signal<> &topBottomUpdated();
};//class DateTimeSelect


class MemVariableSpinBox : public Wt::WContainerWidget
{
protected:
  typedef boost::recursive_mutex::scoped_lock Lock;
  typedef boost::shared_ptr<Lock> LockShrdPtr;

  Wt::WSpinBox *m_spinBox;

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
  Wt::WSpinBox *spinBox() { return m_spinBox; }
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
