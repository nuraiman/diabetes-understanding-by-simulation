#ifndef MISCGUIUTILS_HH
#define MISCGUIUTILS_HH

#include "ArtificialPancrease.hh"
#include <QDateTime>
#include <QString>
#include <string>

class TCanvas;
class QWidget;
class NLSimple;
class ConsentrationGraph;

NLSimple *openNLSimpleModelFile( QWidget *parent = 0 );
NLSimple *openNLSimpleModelFile( QString &name, QWidget *parent = 0 );

//below 'graphType' is of enum type CgmsDataImport::InfoType
ConsentrationGraph *openConsentrationGraph( QWidget *parent = 0, int graphType = -1 );

PosixTime qtimeToPosixTime( const QDateTime &qdt );
QDateTime posixTimeToQTime( const PosixTime &time );
QTime durationToQTime( const TimeDuration &duration );
TimeDuration qtTimeToDuration( const QTime &time );

void cleanCanvas( TCanvas *can, const std::string &classNotToDelete );


#endif // MISCGUIUTILS_HH
