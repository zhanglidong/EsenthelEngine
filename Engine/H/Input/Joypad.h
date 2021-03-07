/******************************************************************************

   Use 'Joypads' container to access Joypads input.

/******************************************************************************/
enum GAME_PAD_BUTTON // button indexes as defined for XInput/Xbox/NintendoSwitch GamePads
{
   GPB_A     , // A
   GPB_B     , // B
   GPB_X     , // X
   GPB_Y     , // Y
   GPB_L1    , // Left  Shoulder
   GPB_R1    , // Right Shoulder
   GPB_L2    , // Left  Trigger
   GPB_R2    , // Right Trigger
   GPB_LTHUMB, // Left  Thumb
   GPB_RTHUMB, // Right Thumb
   GPB_BACK  , // Back
   GPB_START , // Start
   GPB_NUM   , // number of buttons for XInput/Xbox GamePads

   GPB_LSL=GPB_NUM, // Nintendo Switch Left  SL
   GPB_LSR        , // Nintendo Switch Left  SR
   GPB_RSL        , // Nintendo Switch Right SL
   GPB_RSR        , // Nintendo Switch Right SR
   GPB_SWITCH_NUM , // number of buttons for NintendoSwitch GamePads

   // alternative names
   GPB_SELECT=GPB_BACK ,
   GPB_MINUS =GPB_BACK ,
   GPB_PLUS  =GPB_START,
};
/******************************************************************************/
struct Vibration
{
   struct Motor
   {
      Flt intensity, // intensity, 0..1
          frequency; // frequency, Hertz
   };
   Motor motor[2];
};
/******************************************************************************/
struct Joypad // Joypad Input
{
   struct Color2
   {
      Color main, sub;

      Color2& zero()  {       main.zero();   sub.zero(); return T;}
      Bool    any ()C {return main.any () || sub.any ();}
   };

   Vec2 dir       , //        direction
        dir_a  [2]; // analog direction
   Flt  trigger[2]; // trigger

   Bool b (Int x)C {return InRange(x, _button) ? ButtonOn(_button[x]) : false;} // if button 'x' is on
   Bool bp(Int x)C {return InRange(x, _button) ? ButtonPd(_button[x]) : false;} // if button 'x' pushed   in this frame
   Bool br(Int x)C {return InRange(x, _button) ? ButtonRs(_button[x]) : false;} // if button 'x' released in this frame
   Bool bd(Int x)C {return InRange(x, _button) ? ButtonDb(_button[x]) : false;} // if button 'x' double clicked

   Bool supportsVibrations()C; // if supports vibrations

   UInt         id(     )C {return _id  ;} // get unique id of this joypad
 C Str&       name(     )C {return _name;} // get joypad name
   Str  buttonName(Int x)C;                // get button name, buttonName(0) -> "Joypad1", buttonName(1) -> "Joypad2", ..

   Joypad& vibration(C Vec2 &vibration                    ); // set vibrations, 'vibration.x'=left motor intensity (0..1), 'vibration.y'=right motor intensity (0..1)
   Joypad& vibration(C Vibration &left, C Vibration &right); // set vibrations

 C Color2& colorLeft ()C {return _color_left ;} // get color of the left  part of the Joypad (TRANSPARENT if unknown)
 C Color2& colorRight()C {return _color_right;} // get color of the right part of the Joypad (TRANSPARENT if unknown)

#if EE_PRIVATE
   // manage
   void acquire(Bool on);
   void update (C Byte *on, Int elms);
   void update ();
   void clear  ();
   void zero   ();
   void push   (Byte b);
   void release(Byte b);
   Int  index  ()C;
#endif

#if !EE_PRIVATE
private:
#endif
   Byte   _button[32];
#if WINDOWS
   Byte   _xinput1;
#endif
#if WINDOWS_OLD
   Byte   _offset_x, _offset_y;
#endif
   Bool   _connected;
   Flt    _last_t[32];
   UInt   _id;
#if SWITCH
   UInt   _vibration_device_handle[2];
#endif
   Color2 _color_left, _color_right;
   Str    _name;
#if WINDOWS_OLD
   #if EE_PRIVATE
      IDirectInputDevice8 *_device;
   #else
      Ptr _device;
   #endif
#elif MAC
   Ptr    _device;
#endif

   static CChar *_button_name[32];

  ~Joypad();
   Joypad();

   NO_COPY_CONSTRUCTOR(Joypad);
};
extern MemtN<Joypad, 4> Joypads;
/******************************************************************************/
Joypad* FindJoypad(UInt id); // find joypad in 'Joypads' container according to its 'id', null on fail
#if EE_PRIVATE
Joypad& GetJoypad(UInt id, Bool &added);

inline Int Compare(C Joypad &a, C Joypad &b) {return Compare(a.id(), b.id());}

void ListJoypads();
void InitJoypads();
void ShutJoypads();
#endif
/******************************************************************************/
