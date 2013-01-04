/* Note: this is at the same time valid JavaScript and C++. */


WT_DECLARE_WT_MEMBER
(AlignOverlay, Wt::JavaScriptFunction, "AlignOverlay",
function( thisID, parentID )
{
 try{
     var child = $('#' + thisID  );
     var parent = $('#' + parentID );
     var parentEl = parent.get(0);
     var childEl = child.get(0);
     var can = $('#c'+ thisID );
     var childCan = can.get(0);

     child.offset( parent.offset() );
     var w, h;

     if( parentEl && parentEl.wtWidth )
       w = parentEl.wtWidth;
     else
       w = parent.width();

     if( parentEl && parentEl.wtHeight )
       h = parentEl.wtHeight;
     else
       h = parent.height();

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
  }catch(e)
  {
    if( console && console.log )
      console.log( 'OverlayCanvas m_alignWithParentSlot Caught Exception' );
  }
}
);

WT_DECLARE_WT_MEMBER
(EncodeOverlayEvent, Wt::JavaScriptFunction, "EncodeOverlayEvent",
function( x0, x1, y0, y1, e )
{
   //var n = Wt.WT.EncodeOverlayEvent(x0,x1,y0,y1,event).split('&');
   //  x0 = n[0];  //in pixels
   //  x1 = n[1];
   //  y0 = n[2];
   //  y1 = n[3];
   //  button   = n[4];  //'0' for no mouse button
   //  keycode  = n[5];  //'0' for no key pressed
   //  altkey   = n[6];
   //  ctrlkey  = n[7];
   //  metakey  = n[8];
   //  shiftkey = n[9];
   //  mouseWheelDelta = n[10];

    var result = "" + x0 + '&' + x1 + '&' + y0 + '&' + y1;
  var button = this.button(e);
  if(!button)
  {
    if (this.buttons & 1)
      button = 1;
    else if (this.buttons & 2)
      button = 2;
    else if (this.buttons & 4)
      button = 4;
    else
      button = 0;
  }//if(!button)
  result += '&' + button;
  if (typeof e.keyCode !== 'undefined')
    result += '&' + e.keyCode;
  else result += '&0';
  if (typeof e.charCode !== 'undefined')
    result += '&' + e.charCode;
  else result += '&0';
  if (e.altKey)   result += '&1';
  else            result += '&0';
  if (e.ctrlKey)  result += '&1';
  else            result += '&0';
  if (e.metaKey)  result += '&1';
  else            result += '&0';
  if (e.shiftKey) result += '&1';
  else            result += '&0';
  var delta = this.wheelDelta(e);
  result += '&' + delta;

  return result;
}//function( x0, x1, y0, y1, event )
);


WT_DECLARE_WT_MEMBER
(OverlayOnMouseUp, Wt::JavaScriptFunction, "OverlayOnMouseUp",
function( sender, e )
{
  var id = sender.id;
  var can = $('#c'+id).eq(0);

  if( !can )
  {
    if( console && console.log )
      console.log('OverlayOnMouseUp Error');
    return;
  }//if( !can )
//  this.cancelEvent(e, this.CancelPropagate);

  can.width = can.width;
  var x0 = can.data('startDragX');
  var y0 = can.data('startDragY');
  var x1 = e.pageX - can.offset().left;
  var y1 = e.pageY - can.offset().top;

  if( x0!==null && y0!==null && can.data('mouseWasDrugged') )
    Wt.emit( id, {name: 'userDragged', event: e, eventObject: sender}, Math.round(x0), Math.round(y0) );
  can.data('startDragX',null);
  can.data('startDragY',null);

  var result = this.EncodeOverlayEvent( x0, x1, y0, y1, e );
  Wt.emit( id, {name: 'OverlayDragEvent'}, result );
}//function( sender, e )
);


WT_DECLARE_WT_MEMBER
(OverlayOnMouseOut, Wt::JavaScriptFunction, "OverlayOnMouseOut",
function( thisID )
{
  var can = $('#c' + thisID );
  var canElement = this.getElement( 'c' + thisID );
  if( !can || !canElement )
  {
    if( console && console.log )
      console.log('onMouseOutSlotJS Error');
    return;
  }

  canElement.width = canElement.width;
  can.data('startDragX',null);
  can.data('startDragY',null);
  can.data('hasMouse', false);
}
);

WT_DECLARE_WT_MEMBER
(OverlayOnClick, Wt::JavaScriptFunction, "OverlayOnClick",
function(s,e)
{
  var id = s.id;
  var can = $('#c'+id);
  var canElement = this.getElement('c'+id);
  if( !can || !canElement )
  {
    if( console && console.log )
      console.log('OverlayOnClick Error');
    return;
  }

//this.cancelEvent(event, this.CancelPropagate);
  if( !can.data('mouseWasDrugged') )
    Wt.emit( id, {name: 'userSingleClicked', event: e, eventObject: canElement} );
}
);


WT_DECLARE_WT_MEMBER
(OverlayOnMouseDown, Wt::JavaScriptFunction, "OverlayOnMouseDown",
function(s,e)
{
  var id = s.id;
  var can = $('#c'+id);
  var canElement = this.getElement('c'+id);
  if( !can || !canElement )
  {
    if( console && console.log )
      console.log('OverlayOnMouseDown Error');
    return;
  }
//  this.cancelEvent(e, this.CancelPropagate);
  canElement.width = canElement.width;
  var x = e.pageX - can.offset().left;
  var y = e.pageY - can.offset().top;
  can.data('startDragX',x);
  can.data('startDragY',y);
  can.data('mouseWasDrugged', false);

  //console.log( 'OverlayOnMouseDown: x=' + x + ', y=' + y );

  if( e.ctrlKey )
    Wt.emit( id, { name: 'cntrlMouseDown' }, Math.round(x) );
}
);

WT_DECLARE_WT_MEMBER
(OverlayOnMouseOver, Wt::JavaScriptFunction, "OverlayOnMouseOver",
function(s,e)
{
  var c = $('#c' + s.id );
  if( !c )
  {
    if( console && console.log )
      console.log('OverlayOnMouseOver Error');
    return;
  }
//  this.cancelEvent(e, this.CancelPropagate);
  c.data('hasMouse', true);
}
);

WT_DECLARE_WT_MEMBER
(OverlayOnKeyPress, Wt::JavaScriptFunction, "OverlayOnKeyPress",
function(id,sender,e)
{
  var can = $('#c'+id).eq(0);

  if( !can || !can.data('hasMouse') )
    return;

  var canElement = can.get(0);

  if( !canElement )
  {
    console.log('OverlayOnKeyPress Error');
    return;
  }

  //Key_Escape == 27
  if( (e.keyCode === 27) && (can.data('startDragX') !== null) )
  {
    canElement.width = canElement.width;
    can.data('startDragX',null);
    can.data('startDragY',null);

    var canElement = this.getElement('c'+id);
    if( canElement )
      canElement.width = canElement.width;
//    this.cancelEvent(e, this.CancelPropagate);
//    return;
  }

  Wt.emit( id, {name: 'keyPressWhileMousedOver', event: e, eventObject: sender} );
}
);



WT_DECLARE_WT_MEMBER
(OverlayOnMouseMove, Wt::JavaScriptFunction, "OverlayOnMouseMove",
function(sender,event)
{
  var id = sender.id;
  var can = $('#c'+id);
  var canElement = this.getElement('c'+id);
  if( can.length===0 || !canElement )
  {
    if( console && console.log )
      console.log('OverlayOnMouseMove Error');
    return;
  }

  var drawMode = can.data('drawMode');
  var chartPadding = can.data('chartPadding');

  if( !drawMode )
  {
    if( console && console.log )
      console.log( 'OverlayOnMouseMove: Couldnt get drawMode' );
    return;
  }

  if( !chartPadding )
  {
    if( console && console.log )
      console.log( 'OverlayOnMouseMove: Couldnt get chartPadding' );
    return;
  }

//    this.cancelEvent(event, this.CancelPropagate);

  var currentX = event.pageX - can.offset().left;
  var currentY = event.pageY - can.offset().top;

  if( can.data('startDragX')===null )
  {
    try
    {
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
    Wt.emit( id, {name: 'cntrlMouseMove'}, Math.round(currentX) );

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
//      drawArrow( 0.5*canElement.height );
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
      context.lineTo(startX, ctop +5);
      context.moveTo(currentX, canElement.height-cbottom );
      context.lineTo(currentX, ctop +5);
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
      // Now well draw some arrows to indicate something besides a zoom-in will happen
      context.strokeStyle = 'black';

      var yheight = 15 + canElement.height/2.0;

      context.beginPath();
      context.moveTo(startX-35, yheight);
      context.lineTo(startX, yheight);
      context.stroke();

      context.beginPath();
      context.moveTo(startX,yheight);
      context.lineTo(startX-10,yheight-4);
      context.lineTo(startX-10,yheight+4);
      context.moveTo(startX,yheight);
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
//         context.strokeText('Will Search Within', -50 + (startX+currentX)/2.0, yheight-15);
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
//      context.strokeText('Will Erase Peaks In Range', -45 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom-5)/2.0 );
  }

  if( outline )
  {
    context.strokeStyle = "#544E4F";
    context.stroke();
  }//if( !highlight )
}
);
