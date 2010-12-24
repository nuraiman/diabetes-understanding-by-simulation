#include "ProgramOptionsGui.hh"
#include "ui_ProgramOptionsGui.h"

#include <iostream>

#include "ArtificialPancrease.hh"
#include "ProgramOptions.hh"
#include "MiscGuiUtils.hh"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace std;

ProgramOptionsGui::ProgramOptionsGui( ModelSettings *settings, QWidget *parent) :
    QFrame(parent),
    m_ui(new Ui::ProgramOptionsGui),
    m_settings(settings),
    m_useKgs(false)
{
    m_ui->setupUi(this);

    m_ui->m_weightInput->setValue( 2.20462262 * m_settings->m_personsWeight );
    m_ui->m_cgmsIndivReadingUncert->setValue(m_settings->m_cgmsIndivReadingUncert);
    m_ui->m_cgmsDelay->setTime( durationToQTime(m_settings->m_cgmsDelay) );
    m_ui->m_predictAhead->setTime( durationToQTime(m_settings->m_predictAhead) );
    m_ui->m_dt->setTime( durationToQTime(m_settings->m_dt) );
    m_ui->m_endTrainingTime->setDateTime( posixTimeToQTime(m_settings->m_endTrainingTime) );
    m_ui->m_startTrainingTime->setDateTime( posixTimeToQTime(m_settings->m_startTrainingTime) );
    m_ui->m_lastPredictionWeight->setValue( m_settings->m_lastPredictionWeight );
    m_ui->m_targetBG->setValue( m_settings->m_targetBG );
    m_ui->m_bgLowSigma->setValue( m_settings->m_bgLowSigma );
    m_ui->m_bgHighSigma->setValue( m_settings->m_bgHighSigma );
    m_ui->m_genPopSize->setValue( m_settings->m_genPopSize );
    m_ui->m_genConvergNsteps->setValue( m_settings->m_genConvergNsteps );
    m_ui->m_genNStepMutate->setValue( m_settings->m_genNStepMutate );
    m_ui->m_genNStepImprove->setValue( m_settings->m_genNStepImprove );
    m_ui->m_genSigmaMult->setValue( m_settings->m_genSigmaMult );
    m_ui->m_genConvergCriteria->setValue( m_settings->m_genConvergCriteria );

    connect( m_ui->m_unitButton, SIGNAL(clicked()), this, SLOT(changeMassUnits()) );
    connect( m_ui->m_weightInput,            SIGNAL(valueChanged(int)),          this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_cgmsIndivReadingUncert, SIGNAL(valueChanged(double)),       this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_cgmsDelay,              SIGNAL(timeChanged(QTime)),         this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_predictAhead,           SIGNAL(timeChanged(QTime)),         this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_dt,                     SIGNAL(timeChanged(QTime)),         this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_endTrainingTime,        SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_startTrainingTime,      SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_lastPredictionWeight,   SIGNAL(valueChanged(double)),       this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_targetBG,               SIGNAL(valueChanged(int)),          this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_bgLowSigma,             SIGNAL(valueChanged(double)),       this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_bgHighSigma,            SIGNAL(valueChanged(double)),       this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_genPopSize,             SIGNAL(valueChanged(int)),          this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_genConvergNsteps,       SIGNAL(valueChanged(int)),          this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_genNStepMutate,         SIGNAL(valueChanged(int)),          this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_genNStepImprove,        SIGNAL(valueChanged(int)),          this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_genSigmaMult,           SIGNAL(valueChanged(double)),       this, SLOT(getValuesFromGui()) );
    connect( m_ui->m_genConvergCriteria,     SIGNAL(valueChanged(double)),       this, SLOT(getValuesFromGui()) );
}//ProgramOptionsGui constructor

ProgramOptionsGui::~ProgramOptionsGui()
{
    delete m_ui;
}

void ProgramOptionsGui::changeEvent(QEvent *e)
{
    QFrame::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}



void ProgramOptionsGui::changeMassUnits()
{
  if( m_useKgs )
  {
    double kgs = (double) m_ui->m_weightInput->value();
    m_ui->m_weightInput->setValue(kgs * 2.20462262 + 2); //the 2 is cause of integer truncations
    m_ui->m_unitButton->setText("lbs");
  }else
  {
    double lbs = (double) m_ui->m_weightInput->value();
    m_ui->m_weightInput->setValue(lbs * 0.45359237 );
    m_ui->m_unitButton->setText("kg");
  }

  m_useKgs = !m_useKgs;
}//void ProgramOptionsGui::changeMassUnits()




void ProgramOptionsGui::getValuesFromGui()
{
  m_settings->m_personsWeight = m_ui->m_weightInput->value();
  if( !m_useKgs ) m_settings->m_personsWeight *= 0.45359237;

  m_settings->m_cgmsDelay    = qtTimeToDuration( m_ui->m_cgmsDelay->time() );
  m_settings->m_predictAhead = qtTimeToDuration( m_ui->m_predictAhead->time() );
  m_settings->m_dt           = qtTimeToDuration( m_ui->m_dt->time() );

  m_settings->m_endTrainingTime   = qtimeToPosixTime(m_ui->m_endTrainingTime->dateTime());
  m_settings->m_startTrainingTime = qtimeToPosixTime(m_ui->m_startTrainingTime->dateTime());

  m_settings->m_cgmsIndivReadingUncert = m_ui->m_cgmsIndivReadingUncert->value();
  m_settings->m_lastPredictionWeight   = m_ui->m_lastPredictionWeight->value();
  m_settings->m_targetBG               = m_ui->m_targetBG->value();
  m_settings->m_bgLowSigma             = m_ui->m_bgLowSigma->value();
  m_settings->m_bgHighSigma            = m_ui->m_bgHighSigma->value();
  m_settings->m_genPopSize             = m_ui->m_genPopSize->value();
  m_settings->m_genConvergNsteps       = m_ui->m_genConvergNsteps->value();
  m_settings->m_genNStepMutate         = m_ui->m_genNStepMutate->value();
  m_settings->m_genNStepImprove        = m_ui->m_genNStepImprove->value();
  m_settings->m_genSigmaMult           = m_ui->m_genSigmaMult->value();
  m_settings->m_genConvergCriteria     = m_ui->m_genConvergCriteria->value();

  QObject *caller = QObject::sender();
  if( caller == dynamic_cast<QObject *>(m_ui->m_weightInput) )            // emit valueChanged(0);
    cout << "caller=m_weightInput=" << m_settings->m_personsWeight << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_cgmsIndivReadingUncert) ) // emit valueChanged(1);
    cout << "caller=m_cgmsIndivReadingUncert=" << m_settings->m_cgmsIndivReadingUncert << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_cgmsDelay) )              // emit valueChanged(2);
    cout << "caller=m_cgmsDelay=" << m_settings->m_cgmsDelay << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_predictAhead) )           // emit valueChanged(3);
    cout << "caller=m_predictAhead=" << m_settings->m_predictAhead << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_dt) )                     // emit valueChanged(4);
    cout << "caller=m_dt=" << m_settings->m_dt << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_endTrainingTime) )        // emit valueChanged(5);
    cout << "caller=m_endTrainingTime=" << m_settings->m_endTrainingTime << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_startTrainingTime) )      // emit valueChanged(6);
    cout << "caller=m_startTrainingTime=" << m_settings->m_startTrainingTime << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_lastPredictionWeight) )   // emit valueChanged(7);
    cout << "caller=m_lastPredictionWeight=" << m_settings->m_lastPredictionWeight << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_targetBG) )               // emit valueChanged(8);
    cout << "caller=m_targetBG=" << m_settings->m_targetBG << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_bgLowSigma) )             // emit valueChanged(9);
    cout << "caller=m_bgLowSigma=" << m_settings->m_bgLowSigma << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_bgHighSigma) )            // emit valueChanged(10);
    cout << "caller=m_bgHighSigma=" << m_settings->m_bgHighSigma << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_genPopSize) )             // emit valueChanged(11);
    cout << "caller=m_genPopSize=" << m_settings->m_genPopSize << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_genConvergNsteps) )       // emit valueChanged(12);
    cout << "caller=m_genConvergNsteps=" << m_settings->m_genConvergNsteps << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_genNStepMutate) )         // emit valueChanged(13);
    cout << "caller=m_genNStepMutate=" << m_settings->m_genNStepMutate << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_genNStepImprove) )        // emit valueChanged(14);
    cout << "caller=m_genNStepMutate=" << m_settings->m_genNStepImprove << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_genSigmaMult) )           // emit valueChanged(15);
    cout << "caller=m_genNStepMutate=" << m_settings->m_genSigmaMult << endl;
  if( caller == dynamic_cast<QObject *>(m_ui->m_genConvergCriteria) )     // emit valueChanged(16);
    cout << "caller=m_genConvergCriteria=" << m_settings->m_genConvergCriteria << endl;

  emit valueChanged(-1);
}//void ProgramOptionsGui::getValuesFromGui()

void ProgramOptionsGui::setTrainingTimeLimits( boost::posix_time::ptime start,
                                               boost::posix_time::ptime end,
                                               bool setToLimits )
{
  //if( start >= qtimeToPosixTime(m_ui->m_endTrainingTime->dateTime()) )
  //   m_ui->m_endTrainingTime->setDateTime(posixTimeToQTime(end));

  //if( start >= qtimeToPosixTime(m_ui->m_startTrainingTime->dateTime()) )
   //  m_ui->m_endTrainingTime->setDateTime(posixTimeToQTime(start));

   m_ui->m_endTrainingTime->setDateTimeRange( posixTimeToQTime(start), posixTimeToQTime(end) );
   m_ui->m_startTrainingTime->setDateTimeRange( posixTimeToQTime(start), posixTimeToQTime(end) );

   if( m_ui->m_endTrainingTime->dateTime() == m_ui->m_startTrainingTime->dateTime() )
     m_ui->m_endTrainingTime->setDateTime(posixTimeToQTime(end));

   if( setToLimits )
   {
     m_ui->m_endTrainingTime->setDateTime(posixTimeToQTime(end));
     m_ui->m_startTrainingTime->setDateTime(posixTimeToQTime(start));
   }//if( setToLimits )
}//void ProgramOptionsGui::setTrainingTimeLimits
