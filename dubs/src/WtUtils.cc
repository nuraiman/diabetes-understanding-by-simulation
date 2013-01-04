#include "DubsConfig.hh"

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

#include <Wt/WText>
#include <Wt/WTime>
#include <Wt/WDate>
#include <Wt/WLabel>
#include <Wt/WDialog>
#include <Wt/WSpinBox>
#include <Wt/WComboBox>
#include <Wt/WDateTime>
#include <Wt/WPopupMenu>
#include <Wt/WTableView>
#include <Wt/WJavaScript>
#include <Wt/WDatePicker>
#include <Wt/WPushButton>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WIntValidator>
#include <Wt/WContainerWidget>
#include <Wt/WLengthValidator>
#include <Wt/WDoubleValidator>

#include "dubs/WtGui.hh"
#include "dubs/WtUtils.hh"
#include "dubs/DubsApplication.hh"
#include "dubs/ArtificialPancrease.hh"

using namespace Wt;
using namespace std;


#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH


#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

Div::Div( const std::string &styleClass, Wt::WContainerWidget *parent )
  : Wt::WContainerWidget( parent )
{
  setInline( false );
  if( styleClass != "" ) WContainerWidget::setStyleClass( styleClass );
}//Div Constructor



DateTimeSelect::DateTimeSelect( const std::string &labelText,
                                const Wt::WDateTime &initialTime,
                                Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_lineEdit( new WLineEdit( this ) ),
    m_timeSelected( new JSignal<std::string>( this, "timeSelected" ) ),
    m_sectionBoxClosed( new JSignal<std::string>( this, "sectionBoxClosed" ) ),
#if( USE_JSLOTS_WITH_DateTimeSelect )
    m_setTopSlot( NULL ),
    m_setBottomSlot( NULL ),
    m_setDateTimeSlot( NULL ),
#endif //USE_JSLOTS_WITH_DateTimeSelect
    m_top( WDate(3000,1,1), WTime(0,0,0) ),
    m_bottom( WDate(1901,1,1), WTime(0,0,0) ),
    m_dateTime( initialTime )
{
  m_app = wApp;
  if( !m_app )
    throw runtime_error( SRC_LOCATION + ": unable to get wApp" );

  setInline(true);
  setStyleClass( "DateTimeSelect" );

  if( !labelText.empty() )
  {
    WLabel *label = new WLabel( labelText );
    this->insertWidget( 0, label );
    label->setBuddy( m_lineEdit );
  }//if( !labelText.empty() )

  wApp->useStyleSheet( "local_resources/jquery-ui-1.8.16.custom.css" );
  wApp->require( "local_resources/jquery-ui-1.8.16.custom.min.js", "jquery-ui" );
  wApp->require( "local_resources/jquery-ui-sliderAccess.js",      "jquery-ui-sliderAccess" );
  wApp->require( "local_resources/jquery-ui-timepicker-addon.js",  "timepicker-addon" );

  m_lineEdit->setTextSize( 20 );

  const string jsref = "$('#" + m_lineEdit->id() + "')";

  m_lineEdit->setStyleClass( "hasDatePicker" );

  string datetime_options =
      "{"
        "showSecond: true"
        ",showMillisec: false"
        ",timeFormat: 'h:mm:ss tt'"
        ",addSliderAccess: true"
        ",sliderAccessArgs:{touchonly:false}"
        ",ampm: true";
  datetime_options += ",onSelect: function(dateText, inst){"
      "Wt.emit( '" + id() + "', {name: 'timeSelected'}, dateText ); }";
  datetime_options += ",onClose: function(dateText, inst){"
      "Wt.emit( '" + id() + "', {name: 'sectionBoxClosed'}, dateText ); }";

  datetime_options += "}";

  doJavaScript( jsref + ".datetimepicker(" + datetime_options + ");" );

#if( USE_JSLOTS_WITH_DateTimeSelect )
  const string topSlotJs = "function(s,date){" + jsref + ".datetimepicker('option','maxDate',date);}";
  const string bottomSlotJs = "function(s,date){" + jsref + ".datetimepicker('option','minDate',date);}";
  const string setDateTimeSlotJs = "function(s,date){" + jsref + ".datetimepicker('setDate',date);}";

  m_setTopSlot      = new JSlot( topSlotJs, this );
  m_setBottomSlot   = new JSlot( bottomSlotJs, this );
  m_setDateTimeSlot = new JSlot( setDateTimeSlotJs, this );
#endif  //USE_JSLOTS_WITH_DateTimeSelect


  if( m_dateTime.isValid() )
    set( m_dateTime );
  else
    setToCurrentTime();

  setTop( m_top );
  setBottom( m_bottom );

  m_timeSelected->connect( boost::bind( &DateTimeSelect::changedCallback, this, _1 ) );

}//DateTimeSelect constructor

DateTimeSelect::~DateTimeSelect(){}

void DateTimeSelect::set( const Wt::WDateTime &dt )
{
  if( !dt.isValid() )
    return;
  if( (dt < m_bottom)  || (dt > m_top) )
    return;

  m_dateTime = dt;
#if( USE_JSLOTS_WITH_DateTimeSelect )
  m_setDateTimeSlot->exec( "''", jsForDate( m_dateTime ) );
#else
  const string js = "$('#" + m_lineEdit->id() + "').datetimepicker('setDate'," + jsForDate( m_dateTime ) + ");";
  doJavaScript( js );
#endif  //USE_JSLOTS_WITH_DateTimeSelect
}//void set( const Wt::WDateTime *dateTime )

string DateTimeSelect::jsForDate( const WDateTime &datetime )
{
  stringstream datestrm;

  datestrm << "(new Date(" << datetime.date().year()
           << "," << datetime.date().month() - 1  //aparently javascript uses '0' based months
           << "," << datetime.date().day()
           << "," << datetime.time().hour()
           << "," << datetime.time().minute()
           << "," << datetime.time().second()
           << ") )";

  return datestrm.str();
}//string DateTimeSelect::jsForDate( const Wt::WDateTime &datetime )

void DateTimeSelect::setToCurrentTime()
{
  set(WDateTime::fromPosixTime(boost::posix_time::second_clock::local_time()));
}

Wt::WDateTime DateTimeSelect::dateTime() const
{
  return m_dateTime;
}//Wt::WDateTime DateTimeSelect::dateTime() const


//dateTimeFromStr(...) expects input like '07/23/2010 12:15:00 pm'
string DateTimeSelect::dateTimeToStr( const WDateTime &datetime ) const
{
  return datetime.toString( "MM/dd/yyyy hh:mm:ss ap" ).narrow();
}//string dateTimeToStr( const WDateTime &datetime ) const


//dateTimeToStr(...) returns answer in format like '07/23/2010 12:15:00 pm'
WDateTime DateTimeSelect::dateTimeFromStr( const string &datetime ) const
{
  return WDateTime::fromString( datetime, "MM/dd/yyyy hh:mm:ss ap" );
}//WDateTime dateTimeFromStr( const string &datetime ) const


void DateTimeSelect::setTop( const Wt::WDateTime &top )
{
  if( top == m_top )
    return;

  m_top = top;

#if( USE_JSLOTS_WITH_DateTimeSelect )
  m_setTopSlot->exec( "''", jsForDate( top ) );
#else
  const string js = "$('#" + m_lineEdit->id() + "').datetimepicker('option','maxDate'," + jsForDate( top ) + ");";
  doJavaScript( js );
#endif

  validate( false );
  m_topBottomChanged.emit();
}//setTop( const Wt::WDateTime &top )


void DateTimeSelect::setBottom( const Wt::WDateTime &bottom )
{
  if( bottom == m_bottom )
    return;

  m_bottom = bottom;

#if( USE_JSLOTS_WITH_DateTimeSelect )
  m_setBottomSlot->exec( "''", jsForDate( bottom ) );
#else
  const string js = "$('#" + m_lineEdit->id() + "').datetimepicker('option','minDate'," + jsForDate( bottom ) + ");";
  doJavaScript( js );
#endif //USE_JSLOTS_WITH_DateTimeSelect

  validate( false );
  m_topBottomChanged.emit();
}//setBottom( const Wt::WDateTime &bottom )


void DateTimeSelect::changedCallback( const std::string &datTimeText )
{
  const WDateTime newDateTime = dateTimeFromStr( datTimeText );

  if( newDateTime.isValid() )
  {
    if( newDateTime < m_bottom )
      set( m_bottom );
    else if( newDateTime > m_top )
      set( m_top );
    else
      m_dateTime = newDateTime;

//    cerr << "\n\nChanged time to " << m_dateTime.toString()
//         << " tried " << newDateTime.toString()
//         << ", m_top=" << m_top.toString()
//         << ", m_bottom=" << m_bottom.toString()
//         << endl << endl;
    m_changed.emit( m_dateTime );
  }else
  {
    cerr << "Time string '" << datTimeText << "' is an invalid dateTime" << endl;
  }//if( newDateTime.isValid() )
}//void changedCallback( const std::string &datTimeText )


void DateTimeSelect::validate( bool emitChanged )
{
  const WDateTime currentDT = dateTime();
  if( currentDT < m_bottom )
    set( m_bottom );
  else if( currentDT > m_top )
    set( m_top );

  if( emitChanged )
    m_changed.emit( m_dateTime );
}//void notify()

Wt::Signal<Wt::WDateTime> &DateTimeSelect::changed()
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
    m_spinBox( new WDoubleSpinBox() )
{
  m_app = wApp;  //Get WApplication now, so we can update things outside main event loop

  if( !m_app )
    throw runtime_error( SRC_LOCATION + ": unable to get wApp" );

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
  DubsApplication *app = dynamic_cast<DubsApplication *>(m_app);
  if( !app )
  {
    cerr << "\n\n!app, m_app=" << m_app << endl << endl << endl;
    return LockShrdPtr();
  }

  WtGui *gui = app->gui();

  if( !gui )
  {
    //shouldnt ever happen
    cerr << "\n\n!gui\n\n\n" << endl;
    return LockShrdPtr();
  }

  LockShrdPtr lock( new Lock(gui->modelMutex(), boost::try_to_lock ) );

  if( !(*lock) )
  {
    const string msg = "Sorry, you can't change this value while your computing model parameters";
    m_app->doJavaScript( "alert( \"" + msg + "\" )", true );
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
  if( !(*lock) )
    return;
  (*m_memVariable) = floor( m_spinBox->value() + 0.5 );
}//

void DoubleSpinBox::updateMemmoryFromGui()
{
  LockShrdPtr lock = getLock();
  assert( lock.get() );
  if( !(*lock) )
    return;
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

  WtGui *gui = dynamic_cast<WtGui *>(m_app);

  if( gui )
  {
    LockShrdPtr lock( new Lock(gui->modelMutex(), boost::try_to_lock ) );

    if( !(*lock) )
    {
      const string msg = "Sorry, you can't change this value while your computing model parameters";
      m_app->doJavaScript( "alert( \"" + msg + "\" )", true );
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





