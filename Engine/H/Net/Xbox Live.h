/******************************************************************************/
struct XBOXLive
{
   enum EVENT
   {
      STATUS_CHANGED, // user log in status has changed
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
   Bool  loggedIn()C {return _status==LOGGED_IN;} // if currently logged in
   void     logIn(); // initiate log in process, result will be reported through the 'callback' function
   
   ULong userID      ()C {return _me.id       ;} // get user ID           ,  0  on fail. This is valid only after being logged in
   Long  userScore   ()C {return _me.score    ;} // get user score        , -1  on fail. This is valid only after being logged in and after 'getUserProfile' completed
 C Str&  userName    ()C {return _me.name     ;} // get user name/gamertag,  "" on fail. This is valid only after being logged in
 C Str&  userImageURL()C {return _me.image_url;} // get user image url from which you can download his/her photo, for example by using the 'Download' class. This is valid only after being logged in and after 'getUserProfile' completed

   // cloud saves
   Bool cloudSupported    ()C; // if cloud saves are supported, this will always be false if currently not logged in
   Long cloudAvailableSize()C; // get number of available bytes for cloud storage, 0 on fail

   Bool cloudDel(C Str &file_name); // delete 'file_name', false on fail

   Bool cloudSave(C Str &file_name, File &file,              Cipher *cipher=null); // save data from 'file' to 'file_name' cloud file, false on fail, only data from current 'file' file position to the end of the file is saved. !! There is a 16 MB limit for file size in Xbox Live !!
   Bool cloudLoad(C Str &file_name, File &file, Bool memory, Cipher *cipher=null); // load file from 'file_name' cloud file to 'file', false on fail, 'file' should be already opened for writing if 'memory' is set to false, if 'memory' is set to true then 'file' will be first reinitialized with 'writeMemFixed' before loading, which means that load result will not be stored into original 'file' target, but instead into a dynamically allocated memory

   struct CloudFile
   {
      Str  name;
      Long size;
   };
   Bool cloudFiles(MemPtr<CloudFile> files)C; // get list of files that are currently stored in the cloud, false on fail

private:
   STATUS   _status=LOGGED_OUT;
   User     _me;
   SyncLock _lock;
#if EE_PRIVATE
   void setStatus(STATUS status);
   void logInOk();
   void getUserProfile() {getUserProfile(userID());} // request extra profile information for current user (such as score and image url), result will be reported through the 'callback' function
   void getUserProfile(ULong user_id);
#endif
}extern
   XboxLive;
/******************************************************************************/
