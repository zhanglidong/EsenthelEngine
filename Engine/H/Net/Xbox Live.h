/******************************************************************************/
struct XBOXLive
{
   enum EVENT
   {
      STATUS_CHANGED, // current user log in status has changed
      USER_PROFILE  , // 'getUserProfile' finished
   };
   enum STATUS : Byte
   {
      LOGGED_OUT, // user currently logged out
      LOGGING_IN, // user currently logging in, but not finished yet
      LOGGED_IN , // user currently logged  in
   };

   struct User
   {
      ULong id=0;
      Str   name, image_url;
      Long  score=-1; // -1=unknown

      void clear();
   };

   void (*callback)(EVENT event)=null; // pointer to a custom function that will be called with processed events, 'event'=event occurring at the moment

   STATUS  status()C {return _status;} // get current log in status
   Bool   logedIn()C {return _status==LOGGED_IN;} // if currently logged in
   void     logIn(); // initiate log in process, result will be reported through the 'callback' function
   
   ULong userID      ()C {return _me.id       ;} // get user ID           ,  0  on fail. This is valid only after being logged in
   Long  userScore   ()C {return _me.score    ;} // get user score        , -1  on fail. This is valid only after being logged in and after 'getUserProfile' completed
 C Str&  userName    ()C {return _me.name     ;} // get user name/gamertag,  "" on fail. This is valid only after being logged in
 C Str&  userImageURL()C {return _me.image_url;} // get user image url from which you can download his/her photo, for example by using the 'Download' class. This is valid only after being logged in and after 'getUserProfile' completed

   void getUserProfile(); // request extra profile information for current user (such as score and image url), result will be reported through the 'callback' function

private:
   STATUS _status=LOGGED_OUT;
   User   _me;

   void setStatus(STATUS status);
   void logInOk();
   void getUserProfile(ULong user_id);
}extern
   XboxLive;
/******************************************************************************/
