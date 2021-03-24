/******************************************************************************

   Use 'Joypads' container to access Joypads input.

/******************************************************************************/
enum JOYPAD_BUTTON // button indexes as defined for XInput/Xbox/NintendoSwitch controllers
{
   JB_A     , // A
   JB_B     , // B
   JB_X     , // X
   JB_Y     , // Y
   JB_L1    , // Left  Shoulder
   JB_R1    , // Right Shoulder
   JB_L2    , // Left  Trigger
   JB_R2    , // Right Trigger
   JB_LTHUMB, // Left  Thumb
   JB_RTHUMB, // Right Thumb
   JB_BACK  , // Back
   JB_START , // Start
   JB_NUM   , // number of buttons for XInput/Xbox controllers

   JB_LSL=JB_NUM  , // Nintendo Switch Left  SL
   JB_LSR         , // Nintendo Switch Left  SR
   JB_RSL         , // Nintendo Switch Right SL
   JB_RSR         , // Nintendo Switch Right SR
   JB_NINTENDO_NUM, // number of buttons for NintendoSwitch controllers

   // alternative names
   JB_SELECT=JB_BACK ,
   JB_MINUS =JB_BACK ,
   JB_PLUS  =JB_START,
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
   struct Sensor
   {
      Vec    accel, // accelerometer
             gyro ; // gyroscope
      Orient orn  ; // orientation

      Sensor& reset() {accel.zero(); gyro.zero(); orn.identity(); return T;}
   };

   Vec2 dir       , //        direction
        dir_a  [2]; // analog direction, [0]=left, [1]=right
   Flt  trigger[2]; // trigger         , [0]=left, [1]=right

   Bool b (Int x)C {return InRange(x, _button) ? ButtonOn(_button[x]) : false;} // if button 'x' is on
   Bool bp(Int x)C {return InRange(x, _button) ? ButtonPd(_button[x]) : false;} // if button 'x' pushed   in this frame
   Bool br(Int x)C {return InRange(x, _button) ? ButtonRs(_button[x]) : false;} // if button 'x' released in this frame
   Bool bd(Int x)C {return InRange(x, _button) ? ButtonDb(_button[x]) : false;} // if button 'x' double clicked

   Bool supportsVibrations()C; // if supports vibrations
   Bool supportsSensors   ()C; // if supports sensors, available only if 'JoypadSensors' was enabled

   UInt         id(     )C {return _id  ;} // get unique ID of this Joypad
 C Str&       name(     )C {return _name;} // get Joypad name
   Str  buttonName(Int x)C;                // get button name, buttonName(0) -> "Joypad1", buttonName(1) -> "Joypad2", ..

   Joypad& vibration(C Vec2 &vibration                    ); // set vibrations, 'vibration.x'=left motor intensity (0..1), 'vibration.y'=right motor intensity (0..1)
   Joypad& vibration(C Vibration &left, C Vibration &right); // set vibrations

 C Vec   & accel ()C {return _sensor_right.accel;} // get accelerometer value, available only if 'supportsSensors'
 C Vec   & gyro  ()C {return _sensor_right.gyro ;} // get gyroscope     value, available only if 'supportsSensors'
 C Orient& orient()C {return _sensor_right.orn  ;} // get orientation   value, available only if 'supportsSensors'

 C Sensor& sensorLeft ()C {return _sensor_left ;} // get sensor of the left  part of the Joypad, available only if 'supportsSensors'
 C Sensor& sensorRight()C {return _sensor_right;} // get sensor of the right part of the Joypad, available only if 'supportsSensors'

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
   void sensors(Bool calculate);
#endif

#if !EE_PRIVATE
private:
#endif
   Byte   _button[32];
#if WINDOWS
   Byte   _xinput;
#endif
#if WINDOWS_OLD
   Byte   _offset_x, _offset_y;
#endif
   Bool   _connected;
   Flt    _last_t[32];
   UInt   _id;
#if SWITCH
   UInt   _vibration_handle[2], _sensor_handle[2];
#endif
   Color2 _color_left, _color_right;
   Sensor _sensor_left, _sensor_right;
   Str    _name;
#if WINDOWS_OLD
#if EE_PRIVATE && JP_DIRECT_INPUT
   IDirectInputDevice8 *_device;
#else
   Ptr    _device;
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
void JoypadSensors(Bool calculate); // if want Joypad sensors to be calculated (accelerometer, gyroscope, orientation)
Bool JoypadSensors();               // if want Joypad sensors to be calculated (accelerometer, gyroscope, orientation)

Joypad* FindJoypad(UInt id); // find Joypad in 'Joypads' container according to its 'id', null on fail
#if EE_PRIVATE
Joypad& GetJoypad(UInt id, Bool &added);

inline Int Compare(C Joypad &a, C Joypad &b) {return Compare(a.id(), b.id());}

void ListJoypads();
void InitJoypads();
void ShutJoypads();
#endif
/******************************************************************************/
