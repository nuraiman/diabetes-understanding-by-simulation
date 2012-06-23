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



/*
 * See also: http://www.webtoolkit.eu/wt/blog/2010/03/02/javascript_that_is_c__
 */
#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

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

var scrollOverlayIdArray;

var initCanvasForDragging = function( parentDivId, chartPadding, drawMode, originalID, scrollOverlay )
{
  //It would be nice to only propogate to the server mouse movements when either a
  //  click happens, or a drag operation completes.  Right now this script
  //  blocks onmousemove, unless the user is dragging.
  //It would also be nice to catch keyboard events when the mouse is in this widget..

  //XXX - originalID is needed due to bug Wt, see CanvasForDragging for notes

  //XXX - CanvasForDragging doe not work on pages which might be scrolled - this
  //      can be fixed

//    console.log("originalID is " + originalID + " setting scrollOverlay to " + scrollOverlay);

    if ( scrollOverlayIdArray == undefined )
    {
        scrollOverlayIdArray = [ originalID ];
    }
    else
    {
        if ( scrollOverlay )
        {
            scrollOverlayIdArray.push( originalID );
        }
    }


  try
  {
    var canElement = Wt.WT.getElement( 'c' + parentDivId );
    var can = $('#c'+parentDivId);

    if( !canElement )
      throw ('Couldnt find parent div canvas element c'+parentDivId);
        if( !can )
      throw ('Couldnt find parent div canvas c'+parentDivId);


    can.get(0).style.top = '0';
    can.get(0).style.left = '0';

    //we only want to send click events to the server when it was not a drag event
    //  so we will use the mouseWasDrugged to keep frack of this
    can.data('mouseWasDrugged', false);
    can.data('hasMouse', false);

    canElement.onclick = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);

      if( !can.data('mouseWasDrugged') )
        Wt.emit( originalID, {name: 'userSingleClicked', event: event, eventObject: canElement} );
    };

    canElement.onmousedown = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);
      canElement.width = canElement.width;
      var x = event.pageX - can.offset().left;
      var y = event.pageY - can.offset().top;
      can.data('startDragX',x);
      can.data('startDragY',y);
      can.data('mouseWasDrugged', false);

      tip.hide();

      if( event.ctrlKey )
        Wt.emit( originalID, { name: 'cntrlMouseDown' }, Math.round(x) );
    }

    canElement.onmouseup = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);

      canElement.width = canElement.width;

      var x0 = can.data('startDragX');
      var y0 = can.data('startDragY');
      var x1 = event.pageX - can.offset().left;
      var y1 = event.pageY - can.offset().top;

      if( x0!==null && y0!==null && can.data('mouseWasDrugged') )
      {
        Wt.emit( originalID, {name: 'userDragged', event: event, eventObject: canElement}, Math.round(x0), Math.round(y0) );
      }

      can.data('startDragX',null);
      can.data('startDragY',null);
      can.data('cntrldwn',null);
    };

    $('<div id="' + parentDivId + '_tip" class=\"peakinfo\"></div>').appendTo(can.parent());
      var tip = $('#' + parentDivId + '_tip');
    tip.hide();

//    canElement.setAttribute('title', 'my popup' );
//    can.tooltip({tip: '#mytip', position: 'center center', offset: [0,15], delay: 0}).dynamic();



    canElement.onmouseout = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);
      canElement.width = canElement.width;
      can.data('startDragX',null);
      can.data('startDragY',null);
      can.data('hasMouse', false);
      can.data('cntrldwn',null);
      can.data('altdrag',null);
      tip.hide();
      tip.data('curMean',null);
    };

    canElement.onmouseover = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);
      can.data('hasMouse', true);
    };



    canElement.onmousemove = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);

      var currentX = event.pageX - can.offset().left;
      var currentY = event.pageY - can.offset().top;

      if( can.data('startDragX')===null )
      {
        try
        {
          var peaks = can.data('peaks');
          var dist = 99999.9;
          var nearpeak = -1;

          for( var i in peaks )
          {
            var peak = peaks[i];
            var d = Math.abs(currentX-peak.mean);
            if( d < dist && (currentX > peak.xminp && currentX < peak.xmaxp) )
            {
              dist = d;
              nearpeak = i;
            }
          }

          if( nearpeak < 0 )
          {
            tip.hide();
            tip.data('curMean',null);
          }else if( peaks[nearpeak].mean === tip.data('curMean') )
          {
            if( tip.isHidden() )
              tip.show();
          }else
          {
            var peak = peaks[nearpeak];
            tip.data( 'curMean', peak.mean );
            var htmlTxt = ''
                         +'<b>mean</b>: ' + peak.mean + ' keV<br>'
                         +'<b>&sigma;</b>: ' + peak.sigma + ', <b>FWHM</b>: ' + (2.35482*peak.sigma).toFixed(2) + '<br>'
                         +'<b>peak area</b>: ' + peak.amp + '<br>'
                         +'<b>cont. area</b>: ' + peak.cont + '<br>'
                         +'';

            tip.html( htmlTxt );
            //need to set the peak properties text hear
            //should transfer total area info, and continuum area info
            // to display as well

            tip.show();

            var xpos = peak.xmaxp + 15;
            var ypos = peak.yminp - 0.75*(peak.yminp - peak.ymaxp);
            ypos = Math.max( ypos, 10 );
            ypos = Math.min( ypos, can.height() );
            xpos = Math.max( xpos, 10 );
            xpos = Math.min( xpos, can.width() );


            var leftpos = can.offset().left + xpos;
//            var toppos = can.offset().top + ypos;
            var toppos = Math.round(event.clientY);

            if( (toppos + tip.height()+10) > $(window).height() )
              toppos = $(window).height() - tip.height() - 10;

            tip.offset({ top: toppos, left: leftpos });
          }
        }catch(e)
        {
          //console.log( "Caught exception with peaks" )
        }
      }

      var highlight = drawMode.highlight;
      var outline = drawMode.outline;

      var cbottom = chartPadding.bottom;
      var ctop    = chartPadding.top;
      var cright  = chartPadding.right;
      var cleft   = chartPadding.left;

      var startX = can.data('startDragX');
      var startY = can.data('startDragY');
      if( startX===null || startY===null )
        return;

      if( !can.data('mouseWasDrugged') )
        can.data('mouseWasDrugged', true);


      canElement.width = canElement.width;
      //might try the following for QWebView...:


      var dx = currentX - startX;
      var dy = currentY - startY;
      var absDx = Math.abs(dx);
      var absDy = Math.abs(dy);
      var context = canElement.getContext("2d");
      context.clearRect(0, 0, canElement.width, canElement.height);


      if( event.ctrlKey && event.altKey )
        return;

      if( event.ctrlKey )
        Wt.emit( originalID, { name: 'cntrlMouseMove' }, Math.round(currentX) );

      if( event.altKey )
      {
        var drawArrow = function(y)
        {
          context.beginPath();
          context.moveTo(startX, y);
          context.lineTo(currentX, y);
          context.stroke();

          var mult = -1;
          if( dx < 0 )
            mult = 1;

          context.beginPath();
          context.moveTo(currentX,y);
          context.lineTo(currentX + mult*10, y-4);
          context.lineTo(currentX + mult*10, y+4);
          context.moveTo(currentX,y);
          context.fill();
        };

        context.beginPath();
        context.moveTo(startX, 0.05*canElement.height);
        context.lineTo(startX, 0.95*canElement.height);
        context.stroke();

        drawArrow( 0.1*canElement.height );
        drawArrow( 0.33*canElement.height );
//        drawArrow( 0.5*canElement.height );
        drawArrow( 0.66*canElement.height );
        drawArrow( 0.9*canElement.height );

        context.strokeText('Changing Displayed X-axis Range', startX+5, 0.5*canElement.height );

        return;
      }


      if( highlight )
        context.fillStyle='rgba(255, 255, 0, 0.605)';

// The following paints a green square on all of the canvas.
// Used for debugging.
//      context.fillStyle='rgba(0, 255, 0, 0.605)';
//      context.fillRect(0, 0, 2000, 2000);

      if( absDx==absDy && !event.ctrlKey && !event.shiftKey )
      {
        if( highlight )
         context.fillRect(startX,startY,dx, dy);

        if(outline)
        {
          context.beginPath();
          context.moveTo(startX, startY);
          context.lineTo(startX, currentY);
          context.lineTo(currentX, currentY);
          context.lineTo(currentX, startY);
          context.lineTo(startX, startY);
          context.stroke();
        }//if( outline )
      }else if( ((absDx > absDy) || event.ctrlKey) && !event.shiftKey )
      {
        if( highlight )
          context.fillRect(startX, ctop +5,dx,canElement.height-ctop-cbottom -5);

        if(outline)
        {
          context.moveTo(startX, canElement.height- cbottom );
          context.lineTo(startX, ctop );
          context.moveTo(currentX, canElement.height-cbottom );
          context.lineTo(currentX, ctop );
          context.stroke();
        }//if( outline )

        if( !event.ctrlKey && !highlight && absDx>10 )
        {
          if( dx>0 )
            context.strokeText('Zoom In', -25 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom)/2.0 );
          else
          {
            if( absDx < 0.02*canElement.width )
              context.strokeText('Zoom Out x2', -30 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom)/2.0 );
            else if( absDx < 0.04*canElement.width )
              context.strokeText('Zoom Out x4', -30 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom)/2.0 );
            else
              context.strokeText('Zoom Out Completely', -50 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom-5)/2.0 );
          }
        }

        if( event.ctrlKey  )
        {
          var cntrldwn = can.data('cntrldwn');
          if( cntrldwn )
          {
        // Now well draw some arrows to indicate something besides a zoom-in will happen
              context.strokeStyle = 'black';

              var yheight = 15 + canElement.height/2.0;

              context.beginPath();
              context.moveTo(cntrldwn.x-35, yheight);
              context.lineTo(cntrldwn.x, yheight);
              context.stroke();

              context.beginPath();
              context.moveTo(cntrldwn.x,yheight);
              context.lineTo(cntrldwn.x-10,yheight-4);
              context.lineTo(cntrldwn.x-10,yheight+4);
              context.moveTo(cntrldwn.x,yheight);
              context.fill();

              context.beginPath();
              context.moveTo(currentX+35, yheight);
              context.lineTo(currentX, yheight);
              context.stroke();

              context.beginPath();
              context.moveTo(currentX,yheight);
              context.lineTo(currentX+10,yheight-4);
              context.lineTo(currentX+10,yheight+4);
              context.moveTo(currentX,yheight);
              context.fill();

              context.strokeText('Will Search Within', -50 + (cntrldwn.x+currentX)/2.0, yheight-15);
          }
        }

      }else if( !event.shiftKey )
      {
        if( highlight )
          context.fillRect(cleft,startY,canElement.width-cright-cleft, dy);

        if( outline )
        {
          context.moveTo( cleft, startY);
          context.lineTo(canElement.width-cright, startY);
          context.moveTo( cleft, currentY);
          context.lineTo(canElement.width-cright, currentY);
        }//if( outline )

        if( absDy>10 )
        {
          if( dy > 0 )
            context.strokeText('Zoom-in on Y-axis', -30 + canElement.width/2.0, 5 + startY + 0.5*dy );
          else
          {
            if( absDy < 0.05*canElement.height )
              context.strokeText('Zoom-out on Y-axis x2', -30 + canElement.width/2.0, 5 + startY + 0.5*dy );
            else if( absDy < 0.075*canElement.height )
              context.strokeText('Zoom-out on Y-axis x4', -30 + canElement.width/2.0, 5 + startY + 0.5*dy );
            else
              context.strokeText('Zoom-out on Y-axis full', -30 + canElement.width/2.0, 5 + startY + 0.5*dy );
          }
        }

      }else if( event.shiftKey )
      {
        context.fillStyle='rgba(61, 61, 61, 0.25)';
        context.fillRect(startX, ctop +5,dx,canElement.height-ctop-cbottom-10);
        context.moveTo(startX, canElement.height- cbottom -5);
        context.lineTo(startX, ctop+5);
        context.moveTo(currentX, canElement.height-cbottom-5);
        context.lineTo(currentX, ctop+5);
        context.strokeText('Will Erase Peaks In Range', -45 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom-5)/2.0 );
      }

      if( outline )
      {
        context.strokeStyle = "#544E4F";
        context.stroke();
      }//if( !highlight )
    };

    var keyPressWhileMousedOver = function(e)
    {
      if( (e.keyCode === 27) && (can.data('startDragX') !== null) )
      {
        canElement.width = canElement.width;

        can.data('startDragX',null);
        can.data('startDragY',null);
        Wt.WT.cancelEvent(e, Wt.WT.CancelPropagate);
        return;
      }

      if( can.data('hasMouse') )
        Wt.emit( originalID, {name: 'keyPressWhileMousedOver', event: e, eventObject: canElement} );
    };

    $(document).keyup( keyPressWhileMousedOver );


    function handleDrop(evt)
    {
//    Wt.WT.cancelEvent(evt, Wt.WT.CancelPropagate);
      evt.stopPropagation();
      evt.preventDefault();

      var files = evt.dataTransfer.files; // FileList object.

      if( files.length !== 1 )
        return;

      var uploadURL = null;
      try
      {
        uploadURL = can.data('UploadURL');
        if( !uploadURL ) return;
      }catch(e){return; }

      var file  = files[0];
      var xhr = new XMLHttpRequest();
      xhr.open("POST", uploadURL, false);
      xhr.setRequestHeader("Content-type", "application/x-spectrum");
      xhr.setRequestHeader("Cache-Control", "no-cache");
      xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest");
      xhr.setRequestHeader("X-File-Name", file.name);
      xhr.send(file);

      var data = xhr.responseText;
      var status = xhr.status;
      if( status !== 200 && data )
        alert( data + "\nResponse code " + status );
    };//function handleDrop(evt)

    function handleDragEnter(evt)
    {
  //      Wt.WT.cancelEvent(evt, Wt.WT.CancelPropagate);
      evt.stopPropagation();
      evt.preventDefault();
      evt.dataTransfer.dropEffect = 'copy'; // Explicitly show this is a copy.
//     console.log("handleDragEnter - " );
    };//function handleDragOver(evt)

    function handleDragExit(evt)
    {
      evt.stopPropagation();
      evt.preventDefault();
    }
    function handleDragOver(evt)
    {
      evt.stopPropagation();
      evt.preventDefault();
    }

    canElement.addEventListener('dragenter', handleDragEnter, false);
    canElement.addEventListener("dragexit", handleDragExit, false);
    canElement.addEventListener("dragover", handleDragOver, false);
    canElement.addEventListener("drop", handleDrop, false);
  }catch(e)
  {
    if( typeof(e)=='string')
      Wt.emit(originalID, 'jsException', '[initCanvasForDragging exception]: ' + e );
    else
      Wt.emit(originalID, 'jsException', '[initCanvasForDragging exception]: ');
  }
}//var initCanvasForDragging = functiion(...)

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

    if ( ( scrollOverlayIdArray != undefined ) &&
         ( scrollOverlayIdArray.indexOf( childId ) != -1 ) )
    {
      //upper chart
      var newOffset = parent.offset();

      if (scrollParentId != "")
      {
        // Only do this for the Anthony app
        // The new y position should be lined up with the
        // original (unscrolled) y position of the chart
        var y = getOffset( parentEl ).top;
        newOffset.top = y;
      }

      child.offset( newOffset );
    }
    else
    {
      // lower chart
      // console.log('default behavior for lower chart childId ' + childId);
      child.offset( parent.offset() );
    }


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




