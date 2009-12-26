#include "DataInputGui.hh"
#include "ui_DataInputGui.h"

#include <iostream>

#include "MiscGuiUtils.hh"
#include "CgmsDataImport.hh"
#include "ArtificialPancrease.hh"

using namespace std;

DataInputGui::DataInputGui( ConsentrationGraph *graph,
                            QString message,
                            int type,
                            QWidget *parent ) :
    QDialog(parent),
    m_ui(new Ui::DataInputGui),
    m_graph(graph),
    m_type(type)
{
  assert(m_graph);
  m_ui->setupUi(this);

  //m_ui->importButton->setText("Get Data\nFrom File" );
  connect( m_ui->importButton, SIGNAL(clicked()), this, SLOT(readFromFile()) );
  connect( m_ui->okButton, SIGNAL(clicked()), this, SLOT(readSingleInputCloseWindow()) );
  connect( m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()) );

  switch( m_type )
  {
    case CgmsDataImport::CgmsReading:
    case CgmsDataImport::MeterReading:
    case CgmsDataImport::MeterCalibration:
    case CgmsDataImport::GlucoseEaten:
    case CgmsDataImport::GenericEvent:
      m_ui->valueEntry->setDecimals (0);
      m_ui->valueEntry->setSingleStep(1.0);
    break;

    case CgmsDataImport::ISig:
    case CgmsDataImport::BolusTaken:
      m_ui->valueEntry->setDecimals(1);
      m_ui->valueEntry->setSingleStep(0.1);
    break;
    assert(0);
  };//switch(type)

  ConstGraphIter lastElement = m_graph->getLastElement();

  const double defaultVlaue = m_graph->empty() ? 0.0 : lastElement->m_value;
  PosixTime defaultTime = m_graph->empty() ? m_graph->getT0() : lastElement->m_time;

  if( m_type == CgmsDataImport::CgmsReading ) defaultTime += m_graph->getDt();

  m_ui->valueEntry->setValue( defaultVlaue );
  m_ui->dateTimeEdit->setDateTime( posixTimeToQTime(defaultTime) );
}//DataInputGui constructor

DataInputGui::~DataInputGui()
{
    delete m_ui;
}//DataInputGui::~DataInputGui()

void DataInputGui::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}//void DataInputGui::changeEvent(QEvent *e)


void DataInputGui::readFromFile()
{
  const GraphType graphType = m_graph->getGraphType();

  //Following test will probably fail my first time through...
  switch( m_type )
  {
    case CgmsDataImport::CgmsReading:       assert( graphType == GlucoseConsentrationGraph ); break;
    case CgmsDataImport::MeterReading:      assert( graphType == GlucoseConsentrationGraph ); break;
    case CgmsDataImport::MeterCalibration:  assert( graphType == GlucoseConsentrationGraph ); break;
    case CgmsDataImport::GlucoseEaten:      assert( graphType == GlucoseConsumptionGraph );   break;
    case CgmsDataImport::BolusTaken:        assert( graphType == BolusGraph );                break;
    case CgmsDataImport::ISig:              assert( graphType == NumGraphType );              break;
    assert(0);
  };//switch(type)

  ConsentrationGraph *newData = openConsentrationGraph( this, graphType );
  if( !newData ) return;

  m_graph->addNewDataPoints( *newData );
  delete newData;

  done(1);
}//void readFromFile()


void DataInputGui::readSingleInputCloseWindow()
{
  const double value = m_ui->valueEntry->value();
  const PosixTime time = qtimeToPosixTime( m_ui->dateTimeEdit->dateTime() );
  m_graph->addNewDataPoint( time, value );

  done(1);
}//void DataInputGui::readSingleInputCloseWindow()


