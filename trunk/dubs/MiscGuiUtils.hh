#ifndef MISCGUIUTILS_HH
#define MISCGUIUTILS_HH

#include "ArtificialPancrease.hh"
#include <QDateTime>
#include <QString>

class QWidget;
class NLSimple;
class ConsentrationGraph;

NLSimple *openNLSimpleModelFile( QWidget *parent = 0 );
NLSimple *openNLSimpleModelFile( QString &name, QWidget *parent = 0 );

//below 'graphType' is of enum type CgmsDataImport::InfoType
ConsentrationGraph *openConsentrationGraph( QWidget *parent = 0, int graphType = -1 );

PosixTime qtimeToPosixTime( const QDateTime &qdt );
QDateTime posixTimeToQTime( const PosixTime &time );

#endif // MISCGUIUTILS_HH
