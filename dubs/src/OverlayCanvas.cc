#include <fstream>
#include <iostream>
#include <iterator>

#include <Wt/WColor>
#include <Wt/WBorder>
#include <Wt/WJavaScript>
#include <Wt/WApplication>
#include <Wt/WPaintDevice>
#include <Wt/WPaintedWidget>
#include <Wt/WContainerWidget>
#include <Wt/WCssDecorationStyle>
#include <Wt/Chart/WAbstractChart>
#include <Wt/Http/Request>
#include <Wt/Http/Response>
#include <Wt/WResource>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "dubs/OverlayCanvas.hh"


using namespace Wt;
using namespace std;



OverlayCanvas::OverlayCanvas( Wt::Chart::WAbstractChart *parent,
                              bool outline,
                              bool highlight )
  : WPaintedWidget(),
    m_userDraggedSignal( NULL ),
    m_userSingleClickedSignal( NULL ),
    m_keyPressWhileMousedOverSignal( NULL ),
    m_controlMouseDown( NULL ),
    m_controlMouseMove( NULL ),
    m_jsException( NULL ),
    m_parent( parent )
{
  setLoadLaterWhenInvisible( false );
  setPreferredMethod( WPaintedWidget::HtmlCanvas );

  wApp->require( "local_resources/dubs.js" );

  setId( parent->id() + "Cover" );

  wApp->domRoot()->addWidget( this );
  show();
  setPositionScheme( Absolute );

//  WCssDecorationStyle &style = decorationStyle();
//  WBorder border( WBorder::Solid, WBorder::Thin, WColor(black) );
//  style.setBorder( border );

  using boost::lexical_cast;

  // Left off here: 3/26/12 - change the absolute css position (bottom, top) if
  // the user scrolls - need to do this to not occlude the tabs from being
  // selected.
  const string cbottom = lexical_cast<string>(parent->plotAreaPadding(Bottom));
  const string ctop    = lexical_cast<string>(parent->plotAreaPadding(Top));
  const string cleft   = lexical_cast<string>(parent->plotAreaPadding(Left));
  const string cright  = lexical_cast<string>(parent->plotAreaPadding(Right));
  const string chartPadding = "{ top: " + ctop + ", bottom: " + cbottom
                             + ", left: " + cleft + ", right: " + cright + " }";
  const string drawMode = "{ highlight: " + string(highlight?"true":"false")
                          + ", outline: " + (outline?"true":"false") + " }";


  //make it so we can use control-drag and right click on the cavas without
  //  the browsers context menu poping up
  setAttributeValue( "oncontextmenu", "return false;" );

  m_userDraggedSignal             = new JSignal<int,int,WMouseEvent>( this, "userDragged" );
  m_userSingleClickedSignal       = new JSignal<WMouseEvent>( this, "userSingleClicked" );
  m_keyPressWhileMousedOverSignal = new JSignal<WKeyEvent>( this, "keyPressWhileMousedOver" );
  m_controlMouseDown              = new Wt::JSignal<int>( this, "cntrlMouseDown" );
  m_controlMouseMove              = new Wt::JSignal<int>( this, "cntrlMouseMove" );
  m_jsException                   = new JSignal<std::string>( this, "jsException" );

  clicked().connect(        boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->clicked ()), _1 ) );
  doubleClicked().connect(  boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->doubleClicked ()), _1 ) );


  string js = "initOverlayCanvas('"+id()+"', "+chartPadding+", "+drawMode+ ");";
  wApp->doJavaScript( js );

//  setJavaScriptMember( "wtResize", "function(self, w, h) { return false; };" );
}//OverlayCanvas constructos



void OverlayCanvas::connectSignalsToParent( Wt::Chart::WAbstractChart *parent )
{
  //Now we have to propogate all signals down to the canvas we are covering up
  //  Note: this is brittle to future versions of Wt (we might miss some signals
  //        in the future).
  //It might be best to prpogate all of these signals client side!
  //Also, I need to verify doing this isnt bad for performance.
  //  --It may be best just to do this client side via javascript...
  keyWentDown().connect(    boost::bind( &EventSignal<WKeyEvent>::emit,       &(parent->keyWentDown()), _1 ) );
  keyPressed().connect(     boost::bind( &EventSignal<WKeyEvent>::emit,       &(parent->keyPressed()), _1 ) );
  keyWentUp().connect(      boost::bind( &EventSignal< WKeyEvent >::emit,     &(parent->keyWentUp ()), _1 ) );
  enterPressed().connect(   boost::bind( &EventSignal<>::emit,                &(parent->enterPressed ()), _1 ) );
  escapePressed().connect(  boost::bind( &EventSignal<>::emit,                &(parent->escapePressed ()), _1 ) );
  mouseWentDown().connect(  boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseWentDown ()), _1 ) );
  mouseWentUp().connect(    boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseWentUp ()), _1 ) );
  mouseWentOut().connect(   boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseWentOut ()), _1 ) );
  mouseWentOver().connect(  boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseWentOver ()), _1 ) );
  mouseMoved().connect(     boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseMoved ()), _1 ) );
  mouseDragged().connect(   boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseDragged ()), _1 ) );
  mouseWheel().connect(     boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->mouseWheel ()), _1 ) );
#if( WT_VERSION >= 0x3010800 )
  touchStarted().connect(   boost::bind( &EventSignal< WTouchEvent >::emit,   &(parent->touchStarted ()), _1 ) );
  touchEnded().connect(     boost::bind( &EventSignal< WTouchEvent >::emit,   &(parent->touchEnded ()), _1 ) );
  touchMoved().connect(     boost::bind( &EventSignal< WTouchEvent >::emit,   &(parent->touchMoved ()), _1 ) );
  gestureStarted().connect( boost::bind( &EventSignal< WGestureEvent >::emit, &(parent->gestureStarted ()), _1 ) );
  gestureChanged().connect( boost::bind( &EventSignal< WGestureEvent >::emit, &(parent->gestureChanged () ), _1 ) );
  gestureEnded().connect(   boost::bind( &EventSignal< WGestureEvent >::emit, &(parent->gestureEnded ()), _1 ) );
#endif

}//void connectSignalsToParent( Wt::Chart::WAbstractChart *parent )




Wt::JSignal<int,int,WMouseEvent> &OverlayCanvas::userDragged()
{
  return *m_userDraggedSignal;
}


Wt::JSignal<WMouseEvent> &OverlayCanvas::userSingleClicked()
{
  return *m_userSingleClickedSignal;
}

Wt::JSignal<Wt::WKeyEvent> &OverlayCanvas::keyPressWhileMousedOver()
{
  return *m_keyPressWhileMousedOverSignal;
}

Wt::JSignal<int> &OverlayCanvas::controlMouseDown()
{
  return *m_controlMouseDown;
}

Wt::JSignal<int> &OverlayCanvas::controlMouseMove()
{
  return *m_controlMouseMove;
}

Wt::JSignal<std::string> *OverlayCanvas::jsException()
{
  return m_jsException;
}





OverlayCanvas::~OverlayCanvas()
{

}//~WPaintedWidget()


void OverlayCanvas::paintEvent( Wt::WPaintDevice *paintDevice )
{

}//void paintEvent( Wt::WPaintDevice *paintDevice )

