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
Str       WindowText     (                             SysWindow window=App.window()); // get      window title text
void      WindowMinimize (Bool force=false,            SysWindow window=App.window()); // minimize window
void      WindowMaximize (Bool force=false,            SysWindow window=App.window()); // maximize window
void      WindowReset    (Bool force=false,            SysWindow window=App.window()); // reset    window from    maximized/minimized to normal state (maximized/minimized -> normal)
void      WindowToggle   (Bool force=false,            SysWindow window=App.window()); // toggle   window between maximized          and normal state (maximized          <-> normal)
void      WindowActivate (                             SysWindow window=App.window()); // activate window
void      WindowClose    (                             SysWindow window=App.window()); // close    window
void      WindowMove     (Int dx, Int dy,              SysWindow window=App.window()); // move     window by delta
void      WindowPos      (Int  x, Int  y,              SysWindow window=App.window()); // set      window position
void      WindowSize     (Int  w, Int  h, Bool client, SysWindow window=App.window()); // set      window size
VecI2     WindowSize     (                Bool client, SysWindow window=App.window()); // get      window size     , 'client'=if take only the client size      (not including the borders)
RectI     WindowRect     (                Bool client, SysWindow window=App.window()); // get      window rectangle, 'client'=if take only the client rectangle (not including the borders)
Bool      WindowMaximized(                             SysWindow window=App.window()); // if       window is maximized
Bool      WindowMinimized(                             SysWindow window=App.window()); // if       window is minimized
SysWindow WindowActive   (                                                          ); // get      window of active application
SysWindow WindowMouse    (                                                          ); // get      window under mouse cursor
SysWindow WindowParent   (SysWindow window                                          ); // get      window          parent
SysWindow WindowParentTop(SysWindow window                                          ); // get      window most top parent
UInt      WindowProc     (SysWindow window                                          ); // get      window process ID
void      WindowList     (MemPtr<SysWindow> windows                                 ); // get all  window handles in the System
void      WindowSendData (SysWindow window, CPtr data, Int size                     ); // send binary data to application of the specified window, that application can receive the data using "App.receive_data" callback function
/******************************************************************************/
