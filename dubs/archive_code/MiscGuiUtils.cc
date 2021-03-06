#include "MiscGuiUtils.hh"

#include <iostream>
#include <iomanip>
//test
#include <QFileDialog>
#include <QString>
#include <QWidget>
#include <QMessageBox>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QDir>

#include "ConsentrationGraph.hh"
#include "CgmsDataImport.hh"
#include "ResponseModel.hh"

#include "ArtificialPancrease.hh"

#include "TCanvas.h"

using namespace std;

NLSimple *openNLSimpleModelFile( QString &name, QWidget *parent )
{
  name = QFileDialog::getOpenFileName( QDir::currentPath(), "Dub Model (*.dubm)", parent );
  if ( name.isEmpty() ) return NULL;

  return new NLSimple( name.toStdString() );
}//NLSimple *openNLSimpleModelFile( QString &name, QWidget *parent )


NLSimple *openNLSimpleModelFile( QWidget *parent )
{
    QString fileToOpen( QFileDialog::getOpenFileName( QDir::currentPath(), "Dub Model (*.dubm)", parent ) );
    if ( fileToOpen.isEmpty() ) return NULL;

    return new NLSimple( fileToOpen.toStdString() );
}//NLSimple *openNLSimpleModelFile( QWidget *parent = 0 )


ConsentrationGraph *openConsentrationGraph( QWidget *parent, int graphType )
{
  GraphType graphTypeWanted = NumGraphType;
  QString mesage;

    if( graphType >= 0 )
    {
      switch( graphType )
      {
         case CgmsDataImport::CgmsReading:
          mesage = "Select CGMS Data File";
          graphTypeWanted = GlucoseConsentrationGraph;
         break;

         case CgmsDataImport::MeterReading:
          mesage = "Select CGMS Data File";
          graphTypeWanted = GlucoseConsentrationGraph;
         break;
         case CgmsDataImport::MeterCalibration:
           mesage = "Select Meter Calibration Data File";
           graphTypeWanted = GlucoseConsentrationGraph;
         break;

         case CgmsDataImport::GlucoseEaten:
           mesage = "Select Clucose Consumption Data File";
           graphTypeWanted = GlucoseConsumptionGraph;
         break;

         case CgmsDataImport::BolusTaken:
           mesage = "Select Bolus Data File";
           graphTypeWanted = BolusGraph;
         break;

         case CgmsDataImport::GenericEvent:
           mesage = "Select Custom Event Data File";
           graphTypeWanted = CustomEvent;
         break;

         case CgmsDataImport::ISig:
           QMessageBox::warning( parent, "CreateGraphGui: I can not import CgmsDataImport::ISig graphs yet, sorry Dying Now", "Retry", NULL, 0, 0 );
           exit(-1);
         break;
       }//switch( m_graphType )
  }else
  {
      mesage = "Select Data File";
  }
//cout << "About to open a file with parent=" << parent << " and message " << mesage.toStdString() << endl;
    //QString fileToOpen( QFileDialog::getOpenFileName( "../../../../data/", "Input (*.TAB *.dub *.csv *.txt)", parent, name ) );
    QString fileToOpen( QFileDialog::getOpenFileName ( parent, mesage, QDir::currentPath(), "Input (*.TAB *.dub *.csv *.txt)" ) );

    if ( fileToOpen.isEmpty() ) return NULL;

    if( !fileToOpen.endsWith(".dub", Qt::CaseInsensitive) )
    {
      //cout << "About to import spreadsheet " << fileToOpen.toStdString() << endl;
      ConsentrationGraph graph = CgmsDataImport::importSpreadsheet( fileToOpen.toStdString(),  CgmsDataImport::InfoType(graphType) );
      return new ConsentrationGraph( graph );
    }//if( a predifined graph )

    ConsentrationGraph *graph = new ConsentrationGraph( fileToOpen.toStdString() );

    if( graphType >= 0 )
    {
       if( graphTypeWanted != graph->getGraphType() )
       {
         QString message = "CreateGraphGui: I was told to expect graph of type ";
         message += graphType;
         message += " but you opened one of type ";
         message += graph->getGraphType();

         switch( QMessageBox::warning( parent, message, "Retry", "Cancel", 0, 0, 1 ) )
         {
           case 0: return openConsentrationGraph( parent, graphType );
           case 1: return NULL;
         }
         assert(0);
       }//if( isWrongType )
   }// if( graphType >= 0 )

   return graph;
}//void ConsentrationGraphCreate::init()


PosixTime qtimeToPosixTime( const QDateTime &qdt )
  {
      boost::gregorian::date d( qdt.date().year(), qdt.date().month(), qdt.date().day() );
      boost::posix_time::time_duration t( qdt.time().hour(), qdt.time().minute(), qdt.time().second(), 0 );
      t += boost::posix_time::milliseconds( qdt.time().msec() );
      return PosixTime( d, t );
  }


QDateTime posixTimeToQTime( const PosixTime &time )
{
    QDate d( time.date().year(),time.date().month(), time.date().day() );
    QTime t( time.time_of_day().hours(), time.time_of_day().minutes(), time.time_of_day().seconds(), time.time_of_day().total_milliseconds()%1000 );

    return QDateTime(d,t);
}


QTime durationToQTime( const TimeDuration &duration )
{
  return QTime( duration.hours(), duration.minutes(), duration.seconds(), duration.total_milliseconds()%1000 );
}//QTime durationToQTime( const TimeDuration &duration )

TimeDuration qtTimeToDuration( const QTime &time )
{
  TimeDuration dur(time.hour(), time.minute(), time.second(), 0);
  dur += boost::posix_time::milliseconds( time.msec() );
  return dur;
}//TimeDuration qtTimeToDuration( const QTime &time );

void cleanCanvas( TCanvas *can, const std::string &classNotToDelete )
{
  if( !can ) return;
  can->cd();
  can->SetEditable( kTRUE );
  can->Update();

  TObject *obj;
  TList *list = can->GetListOfPrimitives();
  for( TIter nextObj(list); (obj = nextObj()); )
  {
    if( classNotToDelete != obj->ClassName() ) delete obj;
  }//for(...)

  can->Update();
}//void cleanCanvas( TCanvas *can, std::string classNotToDelete )

