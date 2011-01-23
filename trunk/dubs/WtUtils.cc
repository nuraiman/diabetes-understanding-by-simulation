#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/time_duration.hpp"
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WContainerWidget>
#include <Wt/WLabel>
#include <Wt/WSpinBox>
#include <Wt/WDatePicker>
#include <Wt/WPopupMenu>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WTime>
#include <Wt/WDate>
#include <Wt/WDateTime>
#include <Wt/WComboBox>
#include <Wt/WIntValidator>
#include <Wt/WDoubleValidator>
#include <Wt/WDialog>
#include <Wt/WLengthValidator>

#include "WtGui.hh"
#include "WtUtils.hh"
#include "ArtificialPancrease.hh"

using namespace Wt;
using namespace std;



Div::Div( const std::string &styleClass, Wt::WContainerWidget *parent )
  : Wt::WContainerWidget( parent )
{
  setInline( false );
  if( styleClass != "" ) WContainerWidget::setStyleClass( styleClass );
}//Div Constructor






DateTimeSelect::DateTimeSelect( const std::string &labelText,
                                const Wt::WDateTime &initialTime,
                                Wt::WContainerWidget *parent )
  : WContainerWidget(parent),
  m_datePicker( new WDatePicker() ),
  m_hourSelect( new WSpinBox() ),
  m_minuteSelect( new WSpinBox() ),
  m_top( WDate(3000,1,1), WTime(0,0,0) ),
  m_bottom( WDate(1901,1,1), WTime(0,0,0) )
{
  setInline(true);
  setStyleClass( "DateTimeSelect" );

  m_hourSelect->setSingleStep( 1.0 );
  m_hourSelect->setMinimum(0);
  m_hourSelect->setMaximum(23);
  m_hourSelect->setMaxLength(2);
  m_hourSelect->setTextSize(2);

  m_minuteSelect->setSingleStep( 1.0 );
  m_minuteSelect->setMinimum(0);
  m_minuteSelect->setMaximum(59);
  m_minuteSelect->setMaxLength(2);
  m_minuteSelect->setTextSize(2);

  m_datePicker->setGlobalPopup( true );
  m_datePicker->changed().connect(   boost::bind( &DateTimeSelect::validate, this, true ) );
  m_hourSelect->changed().connect(   boost::bind( &DateTimeSelect::validate, this, true ) );
  m_minuteSelect->changed().connect( boost::bind( &DateTimeSelect::validate, this, true ) );
  m_datePicker->changed().connect(  boost::bind( &WDatePicker::setPopupVisible, m_datePicker, false ) );


  if( labelText != "" )
  {
    WLabel *label = new WLabel( labelText, this );
    label->setBuddy( m_datePicker->lineEdit() );
  }//if( labelText != "" )

  addWidget( m_datePicker );
  addWidget( new WText("&nbsp;&nbsp;") );
  addWidget( m_hourSelect );
  addWidget( new WText(":") );
  addWidget( m_minuteSelect );

  set( initialTime );
}//DateTimeSelect constructor

DateTimeSelect::~DateTimeSelect(){}

void DateTimeSelect::set( const Wt::WDateTime &dt )
{
  if( !dt.isValid() ) return;
  if( (dt > m_top) || (dt < m_bottom) ) return;

  m_datePicker->setDate( dt.date() );
  const int hour = dt.time().hour();
  const int minute = dt.time().minute();
  m_hourSelect->setText( boost::lexical_cast<string>(hour) );
  m_minuteSelect->setText( boost::lexical_cast<string>(minute) );
  //validate();
}//void set( const Wt::WDateTime *dateTime )


Wt::WDateTime DateTimeSelect::dateTime() const
{
  const int hour = floor( m_hourSelect->value() + 0.5 );
  const int minute = floor( m_minuteSelect->value() + 0.5 );
  return WDateTime( m_datePicker->date(), WTime( hour, minute ) );
}//Wt::WDateTime DateTimeSelect::dateTime() const

void DateTimeSelect::setTop( const Wt::WDateTime &top )
{
  m_top = top;
  validate();
  m_topBottomChanged.emit();
}//setTop( const Wt::WDateTime &top )

void DateTimeSelect::setBottom( const Wt::WDateTime &bottom )
{
  m_bottom = bottom;
  validate();
  m_topBottomChanged.emit();
}//setBottom( const Wt::WDateTime &bottom )

void DateTimeSelect::validate( bool emitChanged )
{
  const WDateTime currentDT = dateTime();
  if( currentDT < m_bottom )   set( m_bottom );
  else if( currentDT > m_top ) set( m_top );

  if( emitChanged ) m_changed.emit();
}//void notify()

Wt::Signal<> &DateTimeSelect::changed()
{
  return m_changed;
}//Wt::Signal<> changed()

Wt::Signal<> &DateTimeSelect::topBottomUpdated()
{
  return m_topBottomChanged;
}

const Wt::WDateTime &DateTimeSelect::top() const
{
  return m_top;
}

const Wt::WDateTime &DateTimeSelect::bottom() const
{
  return m_bottom;
}



MemVariableSpinBox::MemVariableSpinBox( const string &label,
                                        const string &units,
                                        const double &lower,
                                        const double &upper,
                                        Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_spinBox( new WSpinBox() )
{
  setStyleClass( "MemVariableSpinBox" );
  if( label != "" ) (new WLabel( label, this ))->setBuddy( m_spinBox );
  addWidget( m_spinBox );
  if( units != "" ) new WLabel( units, this );

  m_spinBox->setRange( lower, upper );
  m_spinBox->setTextSize( 6 );
  m_spinBox->setMaxLength( 5 );
  //m_spinBox->setMaximumSize(  );
  m_spinBox->changed().connect( this, &MemVariableSpinBox::updateMemmoryFromGui );
}//MemVariableSpinBox

MemVariableSpinBox::LockShrdPtr MemVariableSpinBox::getLock()
{
  WtGui *gui = dynamic_cast<WtGui *>(wApp);
  if( !gui ) return LockShrdPtr();

  LockShrdPtr lock( new Lock(gui->modelMutex(), boost::try_to_lock ) );

  if( !(*lock) )
  {
    const string msg = "Sorry, you can't change this value while your computing model parameters";
    wApp->doJavaScript( "alert( \"" + msg + "\" )", true );
    cerr << endl << msg << endl;
    updateGuiFromMemmory();
    return LockShrdPtr();
  }//if( !lock )

  return lock;
}//LockShrdPtr getLock()


Wt::Signal<double> &MemVariableSpinBox::valueChanged() { return m_spinBox->valueChanged(); }
double MemVariableSpinBox::value() { return m_spinBox->value(); }

IntSpinBox::IntSpinBox( int *memVariable,
                        const string &label,
                        const string &units,
                        const double &lower,
                        const double &upper,
                        Wt::WContainerWidget *parent )
    : MemVariableSpinBox( label, units, lower, upper, parent),
       m_memVariable(memVariable)
{
  m_spinBox->setSingleStep( 1.0 );
  updateGuiFromMemmory();

}

DoubleSpinBox::DoubleSpinBox( double *memVariable,
               const std::string &label,
               const std::string &units,
               const double &lower,
               const double &upper,
               Wt::WContainerWidget *parent )
 : MemVariableSpinBox( label, units, lower, upper, parent),
   m_memVariable(memVariable)
{
  m_spinBox->setSingleStep( 0.1 );
  updateGuiFromMemmory();
}

TimeDurationSpinBox::TimeDurationSpinBox( TimeDuration *memVariable,
                     const std::string &label,
                     const double &lower,
                     const double &upper,
                     Wt::WContainerWidget *parent )
  : MemVariableSpinBox( label, "min", lower, upper, parent),
    m_memVariable(memVariable)
{
  m_spinBox->setSingleStep( 1.0/60.0 );
  updateGuiFromMemmory();
}

void IntSpinBox::updateGuiFromMemmory()
{
  m_spinBox->setText( boost::lexical_cast<string>(*m_memVariable) );
}
void DoubleSpinBox::updateGuiFromMemmory()
{
  m_spinBox->setText( boost::lexical_cast<string>(*m_memVariable) );
}
void TimeDurationSpinBox::updateGuiFromMemmory()
{
  const int nHours       = m_memVariable->hours();
  const int nMinutes     = m_memVariable->minutes();
  const int nSeconds     = m_memVariable->seconds();
  const int nFracSeconds = m_memVariable->fractional_seconds();
  const double value = 60.0*nHours + nMinutes + (nSeconds/60.0)
                       + (m_memVariable->ticks_per_second()*nFracSeconds/60.0);
  m_spinBox->setText( boost::lexical_cast<string>(value) );
}


void IntSpinBox::updateMemmoryFromGui()
{
  LockShrdPtr lock = getLock();
  assert( lock.get() );
  if( !(*lock) ) return;
  (*m_memVariable) = floor( m_spinBox->value() + 0.5 );
}//

void DoubleSpinBox::updateMemmoryFromGui()
{
  LockShrdPtr lock = getLock();
  assert( lock.get() );
  if( !(*lock) ) return;
  (*m_memVariable) = m_spinBox->value();
}//

void TimeDurationSpinBox::updateMemmoryFromGui()
{
  LockShrdPtr lock = getLock();
  assert( lock.get() );
  if( !(*lock) ) return;
  const int nMinutes = floor( m_spinBox->value() );
  const int nSeconds = floor( (m_spinBox->value()-nMinutes)*0.6 );
  (*m_memVariable) = TimeDuration( 0, nMinutes, nSeconds );
}


MemGuiTimeDate::MemGuiTimeDate( boost::posix_time::ptime *memVariable,
                const std::string &label,
                const boost::posix_time::ptime &lower,
                const boost::posix_time::ptime &upper,
                Wt::WContainerWidget *parent)
  : DateTimeSelect(  label, WDateTime::fromPosixTime(*memVariable), parent ),
  m_memVariable( memVariable )
{
  if( lower != boost::posix_time::min_date_time ) setBottom( WDateTime::fromPosixTime(lower) );
  if( upper != boost::posix_time::max_date_time ) setTop( WDateTime::fromPosixTime(upper) );
  changed().connect( this, &MemGuiTimeDate::updateMemmoryFromGui );
}//MemGuiTimeDate constructor

void MemGuiTimeDate::updateGuiFromMemmory()
{ DateTimeSelect::set( WDateTime::fromPosixTime( *m_memVariable ) ); }

void MemGuiTimeDate::updateMemmoryFromGui()
{
  typedef boost::recursive_mutex::scoped_lock Lock;
  typedef boost::shared_ptr<Lock> LockShrdPtr;

  WtGui *gui = dynamic_cast<WtGui *>(wApp);

  if( gui )
  {
    LockShrdPtr lock( new Lock(gui->modelMutex(), boost::try_to_lock ) );

    if( !(*lock) )
    {
      const string msg = "Sorry, you can't change this value while your computing model parameters";
      wApp->doJavaScript( "alert( \"" + msg + "\" )", true );
      cerr << endl << msg << endl;
      updateGuiFromMemmory();
      return;
    }//if( !lock )

    *m_memVariable = dateTime().toPosixTime();
  }else *m_memVariable = dateTime().toPosixTime();
}//updateMemmoryFromGui()

double MemGuiTimeDate::asNumber() const
{ return Wt::asNumber( dateTime() ); }

boost::posix_time::ptime MemGuiTimeDate::currentValue() const
{
  //return *m_memVariable;
  return dateTime().toPosixTime();
}

