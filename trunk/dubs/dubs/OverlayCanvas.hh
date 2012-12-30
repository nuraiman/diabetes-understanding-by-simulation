#ifndef OverlayCanvas_h
#define OverlayCanvas_h

#include "DubsConfig.hh"

#include <boost/filesystem/path.hpp>

#include <Wt/WEvent>
#include <Wt/WResource>
#include <Wt/Http/Request>
#include <Wt/Http/Response>
#include <Wt/WPaintedWidget>


class OverlayCanvas;

namespace Wt
{
  class WWidget;
  class WPaintDevice;
  namespace Chart{ class WAbstractChart; }
}//namespace Wt

#define TEST_OverlayDragEvent 1

#if( TEST_OverlayDragEvent )
struct OverlayDragEvent
{
  //I have modified the JSlot onMouseUpSlot to actually be able to emit this
  //  event - just no where else in the code.
  int x0, x1, y0, y1;
  int button, keyCode, charCode;
  Wt::WFlags<Wt::KeyboardModifier> keyModifiers;
  int wheelDelta;
//  std::vector<Touch> touches, targetTouches, changedTouches;
  void clear();
};//struct OverlayDragEvent

bool operator>>( std::istream &, OverlayDragEvent& t);
#endif

class OverlayCanvas : public Wt::WPaintedWidget
{
public:
  //This HTML5 Canvas object will be initialized to be same size/location
  //  as 'parent', and will have an id of parent->id()+"Cover".
  //Passing in 'highlight' = true, will mean when the user drags of over the
  //  canvas, the dragged area will be highlighted in yellow; 'false'outline'=true
  //  means some grey lines will be creaded to outline an area while its being
  //  dragged.
  //
  //In part to minimize network traffic, this widget disconnects the standard
  //  Wt mouseMoved(), mouseWentUp(), and mouseWentDown() signals, and instead
  //  replaces them with the userDragged and userSingleClicked signals that
  //  are only fired when a user does a drag (or rather zoom in) operation,
  //  or a single click (in the standard sense, not HTML5 where any mouseup/down
  //  combo is a click {e.g. where the mouse stayed in one position}).
  //  All other signals are propogated through to parent widget passed in.
  //  Additionally, there is a signal for if a key is pressed while the mouse
  //  is over the widget.

  OverlayCanvas( Wt::Chart::WAbstractChart *parent,
                 bool outline, bool highlight );
  virtual ~OverlayCanvas();

  Wt::JSignal<int,int,Wt::WMouseEvent> &userDragged();
  Wt::JSignal<Wt::WMouseEvent> &userSingleClicked();
  Wt::JSignal<Wt::WKeyEvent> &keyPressWhileMousedOver();
  Wt::JSignal<int> &controlMouseDown();
  Wt::JSignal<int> &controlMouseMove();
  //jsException() is mostly for debugging and will probably be removed in the
  //  future
  Wt::JSignal<std::string> *jsException();  //[only partially implemented client side] notifies you of javascript exceptions - probably is not needed for non-debug releases


  void alignWithParent();  //depreciated - should be removed or something

  //By default the clicked() and doubleClicked() EventSignals from the this
  //  OverlayCanvas are propogated server side to the parent WAbstractChart
  //  signals, but none of the other user signals such as mouseWheel,
  //  keyWentDown, touchStarted, etc. are.  Calling connectSignalsToParent(...)
  //  will connect all the other signals.
  //  Also note that some of the signals are killed client side as well.
  void connectSignalsToParent( Wt::Chart::WAbstractChart *parent );

protected:

  void loadInitOverlayCanvasJs();


  virtual void paintEvent( Wt::WPaintDevice *paintDevice );

  Wt::JSignal<int/*x0*/,int/*y0*/,Wt::WMouseEvent /*mouseup event*/> *m_userDraggedSignal;
  Wt::JSignal<Wt::WMouseEvent> *m_userSingleClickedSignal;
  Wt::JSignal<Wt::WKeyEvent> *m_keyPressWhileMousedOverSignal;
  Wt::JSignal<int> *m_controlMouseDown;
  Wt::JSignal<int> *m_controlMouseMove;
  Wt::JSignal<std::string> *m_jsException;

  Wt::JSlot *m_alignWithParentSlot;  //depreciated - should be removed or something
private:
  Wt::Chart::WAbstractChart* m_parent;
};//class OverlayCanvas

#endif
