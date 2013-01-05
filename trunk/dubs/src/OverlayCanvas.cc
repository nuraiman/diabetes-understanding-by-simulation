#include <fstream>
#include "DubsConfig.hh"

#include <iostream>
#include <iterator>

#include <Wt/WColor>
#include <Wt/WBorder>
#include <Wt/WResource>
#include <Wt/WJavaScript>
#include <Wt/WApplication>
#include <Wt/WPaintDevice>
#include <Wt/Http/Request>
#include <Wt/Http/Response>
#include <Wt/WPaintedWidget>
#include <Wt/WContainerWidget>
#include <Wt/WAbstractItemModel>
#include <Wt/WCssDecorationStyle>
#include <Wt/Chart/WAbstractChart>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "dubs/OverlayCanvas.hh"
#include "dubs/WtChartClasses.hh"

#include "js/OverlayCanvas.js"

using namespace Wt;
using namespace std;


//To make the code prettier
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH



/*
 * See also: http://www.webtoolkit.eu/wt/blog/2010/03/02/javascript_that_is_c__
 */
#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#if( TEST_OverlayDragEvent )
void OverlayDragEvent::clear()
{
  button = keyCode = charCode = -1;
  x0 = x1 = y0 = y1 = wheelDelta = keyModifiers = 0;
  //  std::vector<Touch> touches, targetTouches, changedTouches;
}//void OverlayDragEvent::clear()

bool operator>>( std::istream &is, OverlayDragEvent& t )
{
  t.clear();

  string arg;
  is >> arg;
  vector<string> fields;
  boost::algorithm::split( fields, arg, boost::algorithm::is_any_of("&") );

  if( fields.size() != 12 )
  {
    cerr << SRC_LOCATION
         << "\n\tbool operator>>( std::istream &is, OverlayDragEvent& t ):\n\t"
         << "Recieved an input with " << fields.size() << " fields; I expected 12"
         << endl;
    return is;
  }//if( fields.size() != 12 )

  OverlayDragEvent a;

  try
  {
    int i = 0;
    a.x0 = boost::lexical_cast<int>( fields[i++] );
    a.x1 = boost::lexical_cast<int>( fields[i++] );
    a.y0 = boost::lexical_cast<int>( fields[i++] );
    a.y1 = boost::lexical_cast<int>( fields[i++] );
    a.button = boost::lexical_cast<int>( fields[i++] );
    a.keyCode = boost::lexical_cast<int>( fields[i++] );
    a.charCode = boost::lexical_cast<int>( fields[i++] );
    bool altKey = boost::lexical_cast<bool>( fields[i++] );
    bool ctrlKey = boost::lexical_cast<bool>( fields[i++] );
    bool metaKey = boost::lexical_cast<bool>( fields[i++] );
    bool shiftKey = boost::lexical_cast<bool>( fields[i++] );

    a.keyModifiers = 0;
    if( altKey )
      a.keyModifiers |= Wt::AltModifier;
    if( ctrlKey )
      a.keyModifiers |= Wt::ControlModifier;
    if( metaKey )
      a.keyModifiers |= Wt::MetaModifier;
    if( shiftKey )
      a.keyModifiers |= Wt::ShiftModifier;

    a.wheelDelta = boost::lexical_cast<int>( fields[i++] );
    //  std::vector<Touch> touches, targetTouches, changedTouches;
  }catch(...)
  {
    cerr << SRC_LOCATION
         << "\n\tbool operator>>( std::istream &is, OverlayDragEvent& t ):\n\t"
         << "There was an error converting the stream to a OverlayDragEvent"
         << endl;
    return is;
  }//try / catch

  t = a;

  return is;
}//std::istream& operator<<( std::istream&, OverlayDragEvent& t)





void testOverlayDragEvent( OverlayDragEvent a )
{
}
#endif  //#if( TEST_OverlayDragEvent )


void dummyMouseEvent( WMouseEvent a )
{
  cout << "dummyMouseEvent" << endl;
}

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
    m_alignWithParentSlot( NULL ),
    m_overlayEvent( NULL ),
    m_parent( parent )
{
  WChartWithLegend *chartWithLeg = dynamic_cast<WChartWithLegend *>( parent );
  if( chartWithLeg )
    chartWithLeg->setWtResizeJsForOverlay();

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

  doJavaScript( "$('#c" + id() + "').data('drawMode'," + drawMode + ");" );
  doJavaScript( "$('#c" + id() + "').data('chartPadding'," + chartPadding + ");" );

  //make it so we can use control-drag and right click on the cavas without
  //  the browsers context menu poping up
  setAttributeValue( "oncontextmenu", "return false;" );

  m_userDraggedSignal             = new JSignal<int,int,WMouseEvent>( this, "userDragged" );
  m_userSingleClickedSignal       = new JSignal<WMouseEvent>( this, "userSingleClicked" );
  m_keyPressWhileMousedOverSignal = new JSignal<WKeyEvent>( this, "keyPressWhileMousedOver" );
  m_controlMouseDown              = new Wt::JSignal<int>( this, "cntrlMouseDown" );
  m_controlMouseMove              = new Wt::JSignal<int>( this, "cntrlMouseMove" );
  m_jsException                   = new JSignal<std::string>( this, "jsException" );

  m_userSingleClickedSignal->connect( boost::bind( &dummyMouseEvent, _1 ) );

#if( TEST_OverlayDragEvent )
  m_overlayEvent = new JSignal<OverlayDragEvent>( this , "OverlayDragEvent" );
  m_overlayEvent->connect( boost::bind( &testOverlayDragEvent, _1 ) );
#endif  //TEST_OverlayDragEvent

  clicked().connect(        boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->clicked ()), _1 ) );
  doubleClicked().connect(  boost::bind( &EventSignal< WMouseEvent >::emit,   &(parent->doubleClicked ()), _1 ) );


  loadInitOverlayCanvasJs();

  WApplication *app = WApplication::instance();
  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsAlignOverlay);

//  setJavaScriptMember( "wtResize", "function(self, w, h) { return false; };" );
}//OverlayCanvas constructos


void OverlayCanvas::alignWithParent()
{
  m_alignWithParentSlot->exec();
}

void OverlayCanvas::loadInitOverlayCanvasJs()
{
  WApplication *app = WApplication::instance();
  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsEncodeOverlayEvent);

  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnClick);
  const string onclickSlotJS = "function(s,e){ this.WT.OverlayOnClick(s,e);}";
  boost::shared_ptr<JSlot> onclickSlot( new JSlot( onclickSlotJS, this ) );
  clicked().connect( *onclickSlot );
  m_jslots.push_back( onclickSlot );

  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnMouseDown);
  const string onMouseDownSlotJS = "function(s,e){ this.WT.OverlayOnMouseDown(s,e);}";
  boost::shared_ptr<JSlot> onmousedown( new JSlot( onMouseDownSlotJS, this ) );
  mouseWentDown().connect( *onmousedown );
  m_jslots.push_back( onmousedown );

  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnMouseUp);
  const string onMouseUpSlotJS = "function(s,e){ this.WT.OverlayOnMouseUp(s,e);}";
  boost::shared_ptr<JSlot> onmouseup( new JSlot( onMouseUpSlotJS, this ) );
  mouseWentUp().connect( *onmouseup );
  m_jslots.push_back( onmouseup );

  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnMouseOut);
  const string onMouseOutSlotJS = "function(s,e){ this.WT.OverlayOnMouseOut(s.id); }";
  boost::shared_ptr<JSlot> onmouseout( new JSlot( onMouseOutSlotJS, this ) );
  mouseWentOut().connect( *onmouseout );
  m_jslots.push_back( onmouseout );

  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnMouseOver);
  const string onMouseOverSlotJS = "function(s,e){ this.WT.OverlayOnMouseOver(s,e); }";
  boost::shared_ptr<JSlot> onmouseoverSlot( new JSlot( onMouseOverSlotJS, this ) );
  mouseWentOver().connect( *onmouseoverSlot );
  m_jslots.push_back( onmouseoverSlot );

  // Now need to do onmousemove... and then connect all the signals...
  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnMouseMove);
  const string onMouseMoveSlotJS = "function(s,e){ this.WT.OverlayOnMouseMove(s,e); }";
  boost::shared_ptr<JSlot> onmousemoveSlot( new JSlot( onMouseMoveSlotJS, this ) );
  mouseMoved().connect( *onmousemoveSlot );
  m_jslots.push_back( onmousemoveSlot );

  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnKeyPress);
//  const string onKeyPressSlotJS = "function(s,e){ this.WT.OverlayOnKeyPress('" + id() + "',s,e); }";
//  JSlot *onpressSlot = new JSlot( onKeyPressSlotJS, this );
//  app->globalKeyPressed().connect( onpressSlot );  //<<---This statment doesnt compile, therefore the next few lines are a workarount
  //BEGIN WORKAROUND
//  LOAD_JAVASCRIPT(app, "js/OverlayCanvas.js", "OverlayCanvas", wtjsOverlayOnKeyPressJQuery);
  const string onKeyPressSlotJQuery = "$(document).keyup( function(e){ " + app->javaScriptClass() + ".WT.OverlayOnKeyPress('" + id() + "',e.target,e); } );";
  doJavaScript( onKeyPressSlotJQuery );
  //END WORKAROUND

  if( !m_alignWithParentSlot )
    m_alignWithParentSlot = new JSlot( this );

  string js = "function(s,e){ Wt.WT.AlignOverlay('" + id() + "','" + m_parent->id() + "'); }";
  m_alignWithParentSlot->setJavaScript( js );
}//void OverlayCanvas::loadInitOverlayCanvasJs()


void OverlayCanvas::connectSignalsToParent( Wt::Chart::WAbstractChart *parent )
{
  //Now we have to propogate all signals down to the canvas we are covering up
  //  Note: this is brittle to future versions of Wt (we might miss some signals
  //        in the future).
  //It would be best to prpogate all of these signals client side!
  //as doing this server side causes all the signals to be transmitted to the
  //  server!
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
  if( m_userDraggedSignal )
    delete m_userDraggedSignal;
  if( m_userSingleClickedSignal )
    delete m_userSingleClickedSignal;
  if( m_keyPressWhileMousedOverSignal )
    delete m_keyPressWhileMousedOverSignal;
  if( m_controlMouseDown )
    delete m_controlMouseDown;
  if( m_controlMouseMove )
    delete m_controlMouseMove;
  if( m_jsException )
    delete m_jsException;
  if( m_alignWithParentSlot )
    delete m_alignWithParentSlot;
  if( m_overlayEvent )
    delete m_overlayEvent;
}//~WPaintedWidget()


void OverlayCanvas::paintEvent( Wt::WPaintDevice *paintDevice )
{

}//void paintEvent( Wt::WPaintDevice *paintDevice )




/*
var initializeLegend = function( id, parentId, chart_top_padding, chart_right_padding ) {
  try{
    var leg = $('#'+id);
    var parent = $('#'+parentId);

    if(!leg || !parent)
      return;

    leg.draggable();
    leg.data('legTopMargin',     chart_top_padding*1 );
    leg.data('legRightMargin',   chart_right_padding*1 +15 );
    leg.data('widgetRelativeTo', parentId);
    leg.get(0).style.position = 'absolute';
    leg.get(0).style.visibility='';
    leg.css( { 'user-select': 'none', '-o-user-select': 'none', '-moz-user-select': 'none', '-khtml-user-select': 'none', '-webkit-user-select': 'none' } );
    leg.find('*').css( { 'user-select': 'none', '-o-user-select': 'none', '-moz-user-select': 'none', '-khtml-user-select': 'none', '-webkit-user-select': 'none' } );

    doLegendAlignment( id );
    doLegendAlignment( id );  //have to call twice for some reason
  }catch(error){
    //console.log("Failed in initializeLegend");
  }
};




var doLegendAlignment = function( id )
{
  try{
    var legend = $('#'+id);
    var spectrum = $('#'+legend.data('widgetRelativeTo'));
    var toppx, leftpx;

    var hiddenParent = Wt.WT.isHidden(spectrum.get(0));
//    var hiddenParent = spectrum.is(':hidden');  //should be same as above?

    if( hiddenParent )
    {
      toppx = 0.0;
      leftpx = 0.0;
    }else
    {
      toppx = spectrum.offset().top + legend.data('legTopMargin');
      leftpx = spectrum.offset().left + spectrum.outerWidth()
               - legend.outerWidth() -legend.data('legRightMargin');
    }

    legend.offset({ top: toppx, left: leftpx, bottom: '', right: '' });

    if( hiddenParent )
      legend.hide();
    else
      legend.show();
//    alert( 'doLegendAlignment ' + id + ' - top: ' + toppx + ' left: ' + leftpx );
  }catch(error){
    //console.log("Failed in doLegendAlignment");
  };
};



//var g_scrollY = 0;

// From http://stackoverflow.com/questions/442404/dynamically-retrieve-html-element-x-y-position-with-javascript
function getOffset( el ) {
    var _x = 0;
    var _y = 0;
    while( el && !isNaN( el.offsetLeft ) && !isNaN( el.offsetTop ) ) {
        //_x += el.offsetLeft - el.scrollLeft;
        _x += el.offsetLeft;
        //_y += el.offsetTop - el.scrollTop;
        _y += el.offsetTop;
        el = el.offsetParent;
    }
    return { top: _y, left: _x };
}



var alignPaintedWidgets = function(childId,parentId)
{
//    console.log('calling alignPaintedWidgets(' + childId + ', ' + parentId + ')');
  try {
    var child = $('#' + childId );
    var parent = $('#' + parentId );
    var parentEl = parent.get(0);
    var childEl = child.get(0);
    var can = $('#c'+childId);
    var childCan = can.get(0);
    var parentCan = $('#c'+parentId).get(0);

//    console.log('parentId is ' + parentId + ' childId is ' + childId);

    var scrollParentEl = null;
    var scrollParent = null;
    var scrollParentId = child.data('scrollParent');
    if( !scrollParentId || scrollParentId == "" )
      scrollParentId = parent.data('scrollParent');

// Note that scrollParentId is really the id for the "outer div", not the
// id for the widget represented by the scrollParent C++ class

    if (scrollParentId != "")
    {
      scrollParent = $('#' + scrollParentId );
      scrollParentEl = scrollParent.get(0);
    }


    // parent.offset().top and parent.offset().left are something
    // See http://api.jquery.com/offset/
    // Sometimes parent.offset().top is 0 - but if it's non zero,
    // and less than about 180 or so, then we'll want to clip the top
    // off of canvas.
    // console.log("parent.offset().top is " + parent.offset().top);

    // We only copy the parent's offset for
    // the x axis.  We don't scroll the canvas
    // about the y axis, because this might
    // overlap with the tabs.
    // Instead, we will later on compensate for
    // the y-scrolled amount in the C++ callback function
    // (WtTh1Chart::handleDrag()


    child.offset( parent.offset() );


    var w, h;
    if( parentEl && parentEl.wtWidth )
      w = parentEl.wtWidth;
    else
      w = parent.width();

//    console.log('g_scrollY is ' + g_scrollY);
//    console.log('parentEl.scrollTop is ' + parentEl.scrollTop);
//    console.log('childEl.scrollTop is ' + childEl.scrollTop);
//    console.log('child id is ' + childId);
//    console.log('parent id is ' + parentId);
//    console.log('scrollParentId is ' + scrollParentId);

    // This only make sense for the Anthony app
    var outerDivScrollAmount = 0;
    if ( scrollParentId && scrollParentId != "")
    {
//      console.log('scrollParentEl.scrollTop is ' + scrollParentEl.scrollTop);
      outerDivScrollAmount = scrollParentEl.scrollTop;
    }

    if( parentEl && parentEl.wtHeight )
      {
          // This is where you want to set the height accordingly, depending
          // on how much the user has scrolled
//      h = parentEl.wtHeight - g_scrollY;
      h = parentEl.wtHeight - outerDivScrollAmount;
      }
    else
      {
//      h = parent.height() - g_scrollY;
      h = parent.height() - outerDivScrollAmount;
      }

    childEl.style.width =  w+'px';
    childEl.style.height = h+'px';

    childCan.setAttribute('width',w);
    childCan.setAttribute('height',h);

    can.data('startDragX',null);
    can.data('startDragY',null);

    childEl.wtWidth = w;
    childEl.wtHeight = h;
    if( childEl.wtResize )
      childEl.wtResize( childEl, w, h );
  }catch(error){
    console.log("Failed in alignPaintedWidgets");
  }
};



var drawContEst = function( id, new_coord )
{
  try
  {
    var canElement = Wt.WT.getElement( 'c' + id );
    var can = $('#c'+id);
    var cntrldwn = can.data('cntrldwn');
    if( !cntrldwn )
      return;

    var context = canElement.getContext("2d");
    context.beginPath();
    context.moveTo(cntrldwn.x, cntrldwn.y);
    context.lineTo(new_coord.x, new_coord.y);
    context.strokeStyle = "grey";
    context.lineWidth = 2;
    context.stroke();

    context.lineWidth = 1;
    context.save();
    context.translate(cntrldwn.x, cntrldwn.y);
    var lineAngle = Math.atan( (new_coord.y-cntrldwn.y)/(new_coord.x-cntrldwn.x) );
    context.rotate( lineAngle );
    context.strokeText('continuum to use', +5, -5 );
    context.restore();
  }catch(error)
  {
    //console.log("Failed in drawContEst");
  }
};
*/




