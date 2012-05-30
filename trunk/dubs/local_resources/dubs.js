

var initOverlayCanvas = function( parentDivId, chartPadding, drawMode )
{
  //It would be nice to only propogate to the server mouse movements when either a
  //  click happens, or a drag operation completes.  Right now this script
  //  blocks onmousemove, unless the user is dragging.
  //It would also be nice to catch keyboard events when the mouse is in this widget..


  //XXX - OverlayCanvas doesnt work on pages which might be scrolled
    

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
        Wt.emit( parentDivId, {name: 'userSingleClicked', event: event, eventObject: canElement} );
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

      if( event.ctrlKey )
        Wt.emit( parentDivId, { name: 'cntrlMouseDown' }, Math.round(x) );
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
        Wt.emit( parentDivId, {name: 'userDragged', event: event, eventObject: canElement}, Math.round(x0), Math.round(y0) );
      }

      can.data('startDragX',null);
      can.data('startDragY',null);
      can.data('cntrldwn',null);
    };



    canElement.onmouseout = function(event)
    {
      Wt.WT.cancelEvent(event, Wt.WT.CancelPropagate);
      canElement.width = canElement.width;
      can.data('startDragX',null);
      can.data('startDragY',null);
      can.data('hasMouse', false);
      can.data('cntrldwn',null);
      can.data('altdrag',null);
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
        Wt.emit( parentDivId, { name: 'cntrlMouseMove' }, Math.round(currentX) );

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
          context.lineTo(startX, ctop /*+5*/);
          context.moveTo(currentX, canElement.height-cbottom );
          context.lineTo(currentX, ctop /*+5*/);
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

//              context.strokeText('Will Search Within', -50 + (cntrldwn.x+currentX)/2.0, yheight-15);
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
//        context.strokeText('Will Erase Peaks In Range', -45 + (startX+currentX)/2.0, (ctop+5+canElement.height-cbottom-5)/2.0 );
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
        Wt.emit( parentDivId, {name: 'keyPressWhileMousedOver', event: e, eventObject: canElement} );
    };

    $(document).keyup( keyPressWhileMousedOver );
  }catch(e)
  {
    if( typeof(e)=='string')
      Wt.emit(parentDivId, 'jsException', '[initCanvasForDragging exception]: ' + e );
    else 
      Wt.emit(parentDivId, 'jsException', '[initCanvasForDragging exception]: ');
  }
}//var initCanvasForDragging = functiion(...)


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

    // parent.offset().top and parent.offset().left are something
    // See http://api.jquery.com/offset/
    // Sometimes parent.offset().top is 0 - but if it's non zero,
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
  }catch(error){
    console.log("Failed in alignPaintedWidgets");
  }
};




