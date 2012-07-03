/* Note: this is at the same time valid JavaScript and C++. */


WT_DECLARE_WT_MEMBER
(1, Wt::JavaScriptFunction, "AlignOverlay",
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
    console.log( 'OverlayCanvas m_alignWithParentSlot Caught Exception' );
  }
}
);


WT_DECLARE_WT_MEMBER
(2, Wt::JavaScriptFunction, "OnOverlayMouseUp",
function( s, e )
{
  var thisID = s.id;
//  console.log( 'OnOverlayMouseUp: id=' + thisID + ' while sender=' + s.id );

  var can = $('#c'+thisID);
  var canElement = Wt.WT.getElement('c'+thisID);
  if( !can || !canElement )
  {
    if( console && console.log )
      console.log('onMouseUpSlotJS Error');
    return;
  }//if( !can || !canElement )
//  Wt.WT.cancelEvent(e, Wt.WT.CancelPropagate);
  canElement.width = canElement.width;
  var x0 = can.data('startDragX');
  var y0 = can.data('startDragY');
  var x1 = e.pageX - can.offset().left;
  var y1 = e.pageY - can.offset().top;
  if( x0!==null && y0!==null && can.data('mouseWasDrugged') )
    Wt.emit( thisID, {name: 'userDragged', event: e, eventObject: canElement}, Math.round(x0), Math.round(y0) );
  can.data('startDragX',null);
  can.data('startDragY',null);
  can.data('cntrldwn',null);
  can.data('altdrag',null);
  var result = '' + x0 + '&' + x1 + '&' + y0 + '&' + y1;
  var button = Wt.WT.button(e);
  if(!button)
  {
    if (Wt.WT.buttons & 1)
      button = 1;
    else if (Wt.WT.buttons & 2)
      button = 2;
    else if (Wt.WT.buttons & 4)
      button = 4;
    else
      button = -1;
  }
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
  var delta = Wt.WT.wheelDelta(e);
  result += '&' + delta;
  Wt.emit( thisID, {name: 'OverlayDragEvent'}, result );
  }
);


WT_DECLARE_WT_MEMBER
(3, Wt::JavaScriptFunction, "OnOverlayMouseOut",
function( thisID )
{
  var can = $('#c' + thisID );
  var canElement = Wt.WT.getElement( 'c' + thisID );
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
  can.data('cntrldwn',null);
  can.data('altdrag',null);
}
);
