/******************************************************************************

   Use 'WindowCapture' to capture visual contents of a system window.

   Use 'Window' functions to handle OS window management.

/******************************************************************************/
struct WindowCapture // System Window Image Capture
{
   void del    (                                   ); // delete manually
   Bool capture(Image &image, SysWindow window=NULL); // capture 'window' client screen to 'image' (use null for 'window' to capture full desktop), false on fail

  ~WindowCapture() {del();}
   WindowCapture() {data=null;}

private:
   Ptr data;
   NO_COPY_CONSTRUCTOR(WindowCapture);
};
/******************************************************************************/
void      WindowSetText    (C Str &text     ,            SysWindow window=App.window()); // set window text title
Str       WindowGetText    (                             SysWindow window=App.window()); // get window text title
void      WindowMinimize   (Bool force=false,            SysWindow window=App.window()); // minimize window
void      WindowMaximize   (Bool force=false,            SysWindow window=App.window()); // maximize window
void      WindowReset      (Bool force=false,            SysWindow window=App.window()); // reset    window from    maximized/minimized to normal state (maximized/minimized -> normal)
void      WindowToggle     (Bool force=false,            SysWindow window=App.window()); // toggle   window between maximized          and normal state (maximized          <-> normal)
void      WindowActivate   (                             SysWindow window=App.window()); // activate window
void      WindowHide       (                             SysWindow window=App.window()); // hide     window
Bool      WindowHidden     (                             SysWindow window=App.window()); // if       window is hidden
void      WindowShow       (Bool activate,               SysWindow window=App.window()); // show     window, 'activate'=if also activate
void      WindowClose      (                             SysWindow window=App.window()); // close    window
void      WindowFlash      (                             SysWindow window=App.window()); // flash    window
void      WindowSetNormal  (                             SysWindow window=App.window()); // set      window taskbar to be displayed as normal                             (this will work only on Window 7 or newer)
void      WindowSetWorking (                             SysWindow window=App.window()); // set      window taskbar to be displayed as working with unknown progress      (this will work only on Window 7 or newer)
void      WindowSetProgress(Flt progress,                SysWindow window=App.window()); // set      window taskbar to be displayed as working with 'progress' 0..1 value (this will work only on Window 7 or newer)
void      WindowSetPaused  (Flt progress,                SysWindow window=App.window()); // set      window taskbar to be displayed as paused  with 'progress' 0..1 value (this will work only on Window 7 or newer)
void      WindowSetError   (Flt progress,                SysWindow window=App.window()); // set      window taskbar to be displayed as error   with 'progress' 0..1 value (this will work only on Window 7 or newer)
Byte      WindowGetAlpha   (                             SysWindow window=App.window()); // get      window opacity (0=transparent, 255=opaque)
void      WindowAlpha      (Byte alpha  ,                SysWindow window=App.window()); // set      window opacity (0=transparent, 255=opaque)
void      WindowMove       (Int dx, Int dy,              SysWindow window=App.window()); // move     window by delta
void      WindowPos        (Int  x, Int  y,              SysWindow window=App.window()); // set      window position
void      WindowSize       (Int  w, Int  h, Bool client, SysWindow window=App.window()); // set      window size
VecI2     WindowSize       (                Bool client, SysWindow window=App.window()); // get      window size     , 'client'=if take only the client size      (not including the borders)
RectI     WindowRect       (                Bool client, SysWindow window=App.window()); // get      window rectangle, 'client'=if take only the client rectangle (not including the borders)
Bool      WindowMaximized  (                             SysWindow window=App.window()); // if  window is maximized
Bool      WindowMinimized  (                             SysWindow window=App.window()); // if  window is minimized
SysWindow WindowActive     (                                                          ); // get active window
SysWindow WindowMouse      (                                                          ); // get window under mouse cursor
SysWindow WindowParent     (                             SysWindow window             ); // get          parent of window
SysWindow WindowParentTop  (                             SysWindow window             ); // get most top parent of window
void      WindowSendData   (CPtr data, Int size,         SysWindow window             ); // send binary data to an application of the specified window, that application can receive the data using "App.receive_data" callback function
UInt      WindowProc       (                             SysWindow window             ); // get process ID of window
void      WindowList       (MemPtr<SysWindow> windows                                 ); // get list of all window handles in the System

void      WindowMsgBox(C Str &title, C Str &text, Bool error=false); // show OS message box, 'error'=if display as error or message

#if EE_PRIVATE
Ptr WindowMonitor(SysWindow window); // return HMONITOR for 'window'

void PostEvent();

void InitWindow();
void ShutWindow();
#endif
/******************************************************************************/
