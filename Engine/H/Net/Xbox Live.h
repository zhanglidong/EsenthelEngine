/******************************************************************************

   Xbox Live has a 16 MB limit for individual cloud file sizes.
   Individual calls to 'cloudSave' may not exceed that limit.

/******************************************************************************/
struct XBOXLive
{
   enum EVENT
   {
      STATUS_CHANGED, // user log in status has changed
      USER_PROFILE  , // received full profile information for a user
      USER_FRIENDS  , // received list of friends, however their info is not complete (only 'id' and 'favorite' members will be valid, remaining members should be received soon through 'USER_PROFILE' events for each friend separately)
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
   #if EE_PRIVATE
      void clear();
   #endif
   };
   struct Friend : User
   {
      Bool favorite=false;
   #if EE_PRIVATE
      void clear();
   #endif
   };

   void (*callback)(EVENT event, ULong user_id)=null; // pointer to a custom function that will be called with processed events, 'event'=event occurring at the moment, 'user_id'=user ID affected by this event

   STATUS  status()C {return _status;} // get current log in status
   Bool  loggedIn()C {return _status==LOGGED_IN;} // if currently logged in
   void     logIn(); // initiate log in process, result will be reported through the 'callback' function
   
   ULong userID      ()C {return _me.id       ;} // get user ID           ,  0  on fail. This is valid only after being logged in
   Long  userScore   ()C {return _me.score    ;} // get user score        , -1  on fail. This is valid only after being logged in and after USER_PROFILE
 C Str&  userName    ()C {return _me.name     ;} // get user name/gamertag,  "" on fail. This is valid only after being logged in
 C Str&  userImageURL()C {return _me.image_url;} // get user image url from which you can download his/her photo, for example by using the 'Download' class. This is valid only after being logged in and after USER_PROFILE

   // friends
   void          getFriends(                        ) ; // initiate process of obtaining friend list, result will be reported through the 'callback' function with USER_FRIENDS event, only after that event methods below will return valid results
   Bool          getFriends(MemPtr<ULong> friend_ids)C; // get list of friend ID's, false on fail (this will always fail if 'getFriends' was not yet called or has not yet completed with a USER_FRIENDS event)
   Str           userName  (       ULong  user_id   )C; // get user name          , ""    on fail (this will always fail if 'getFriends' was not yet called or has not yet completed with a USER_FRIENDS event)
 C Memc<Friend>& friends   (                        )C; // get friend list        , empty on fail (this will always fail if 'getFriends' was not yet called or has not yet completed with a USER_FRIENDS event)

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
   STATUS       _status=LOGGED_OUT;
   Bool         _friends_known=false, _friends_getting=false;
   User         _me;
   Memc<Friend> _friends;
   SyncLock     _lock;
#if EE_PRIVATE
   void setStatus(STATUS status);
   void logInOk();
   void getUserProfile(); // request extra profile information for current user (such as score and image url), result will be reported through the 'callback' function
   void getUserProfile(C MemPtr<ULong> &user_ids);
#endif
}extern
   XboxLive;
/******************************************************************************/
