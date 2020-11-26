/******************************************************************************/
#include "stdafx.h"

#define SUPPORT_XBOX_LIVE (WINDOWS_NEW && 1)

#if SUPPORT_XBOX_LIVE
#include <robuffer.h> // needed for 'IBufferByteAccess'
#include "../ThirdPartyLibs/begin.h"
#define XSAPI_CPP 1
#define _NO_ASYNCRTIMP
#define _NO_PPLXIMP
#define _NO_XSAPIIMP

//#define XBOX_LIVE_CREATORS_SDK
#include "../ThirdPartyLibs/Xbox Live/2018.6.20181010.2/xsapi/services.h" // !! if changing this to another version/path then also have to change Visual Studio project properties for Include Directories for all Configurations/Platforms, and also update Esenthel Builder source code to extract files !! this is because those LIB files are too big to be stored on GitHub and thus must be extracted manually, because of that to avoid compatibility issues between headers/libraries the paths to headers/libs must match so they must include version numbers
#include "../ThirdPartyLibs/end.h"
#endif

namespace EE{
/******************************************************************************/
XBOXLive XboxLive;

#if SUPPORT_XBOX_LIVE
static std::shared_ptr<xbox::services::system::xbox_live_user> XboxUser;
static std::shared_ptr<xbox::services::xbox_live_context     > XboxCtx;
static Windows::Gaming::XboxLive::Storage::GameSaveProvider   ^GameSaveProvider;
static Windows::Gaming::XboxLive::Storage::GameSaveContainer  ^GameSaveContainer;
#endif
/******************************************************************************/
void XBOXLive::User::clear()
{
   id   = 0;
   score=-1;
   name     .clear();
   image_url.clear();
}
void XBOXLive::Friend::clear()
{
   super::clear();
   favorite=false;
}
/******************************************************************************/
static C XBOXLive::Friend* CFind(C CMemPtr<XBOXLive::Friend> &users, ULong id)
{
   REPA(users)
   {
    C XBOXLive::Friend &user=users[i]; if(user.id==id)return &user;
   }
   return null;
}
static XBOXLive::Friend* Find(MemPtr<XBOXLive::Friend> users, ULong id) {return ConstCast(CFind(users, id));}
/******************************************************************************/
// LOG IN
/******************************************************************************/
void XBOXLive::getUserProfile() {getUserProfile(userID());}
void XBOXLive::getUserProfile(C CMemPtr<ULong> &user_ids)
{
#if SUPPORT_XBOX_LIVE
   if(user_ids.elms() && XboxCtx)
   {
      std::vector<string_t> user_ids_str; user_ids_str.resize(user_ids.elms()); for(Int i=0; i<user_ids_str.size(); i++)user_ids_str[i]=TextInt(user_ids[i]);
      SyncLockerEx locker(lock); if(XboxCtx)
      {
         auto task=XboxCtx->profile_service().get_user_profiles(user_ids_str);
         locker.off();
         task.then([this](xbox::services::xbox_live_result<xbox::services::xbox_live_result<std::vector<xbox::services::social::xbox_user_profile>>> results)
         {
            if(!results.err())
            {
             C auto &result=results.payload(); if(!result.err())
               {
                C auto &profiles=result.payload();
                  for(Int i=0; i<profiles.size(); i++)
                  {
                   C auto &profile=profiles[i];
                     ULong user_id=TextULong(WChar(profile.xbox_user_id().c_str()));
                     // store in temporaries so later we can move/swap into profile fast
                     Long score    =TextLong(WChar(profile.gamerscore().c_str()));
                     Str  image_url=S+profile.game_display_picture_resize_uri().to_string().c_str()+"&w=424"; // default size is around 1080 which is not needed, limit to 424 (other options are 208, 64)
                   /*auto user_app_name=profile.app_display_name().c_str();
                     auto tag=profile.gamertag().c_str();
                     auto user_app_pic=profile.app_display_picture_resize_uri().to_string().c_str();*/
                     if(user_id==T.userID())
                     {
                        SyncLocker locker(lock);
                       _me.score=score;
                        Swap(_me.image_url, image_url);
                     }else
                     {
                        Str user_game_name=profile.game_display_name().c_str();
                        SyncLocker locker(lock);
                        if(Friend *user=Find(_friends, user_id))
                        {
                           user->score=score;
                           Swap(user->name     , user_game_name);
                           Swap(user->image_url, image_url     );
                        }
                     }
                     if(callback)callback(USER_PROFILE, user_id);
                  }
               }
            }
         });
      }
   }
#endif
}
void XBOXLive::setStatus(STATUS status)
{
   if(T._status!=status){T._status=status; if(callback)callback(STATUS_CHANGED, userID());}
}
void XBOXLive::logInOK()
{
#if SUPPORT_XBOX_LIVE
   SyncLockerEx locker(lock);
   if(!XboxCtx)XboxCtx=std::make_shared<xbox::services::xbox_live_context>(XboxUser);
   if(C auto &config=XboxCtx->application_config())
   {
    /*auto app_id=config->title_id();
      auto scid=config->scid().c_str();
      auto env =config->environment().c_str();
      auto sandbox=config->sandbox().c_str();*/

     _me.id  =TextULong(WChar(XboxUser->xbox_user_id().c_str()));
     _me.name=XboxUser->gamertag().c_str();
    //Str age=XboxUser->age_group().c_str(); // can return: "Adult", ..
      getUserProfile(); // request extra info as soon as we have ID

      auto op=Windows::Gaming::XboxLive::Storage::GameSaveProvider::GetForUserAsync(OSUser.get(), ref new Platform::String(config->scid().c_str()));
      locker.off();
      op->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Gaming::XboxLive::Storage::GameSaveProviderGetResult^>([this](Windows::Foundation::IAsyncOperation<Windows::Gaming::XboxLive::Storage::GameSaveProviderGetResult^> ^op, Windows::Foundation::AsyncStatus status)
      {
         if(status==Windows::Foundation::AsyncStatus::Completed)
         {
            SyncLocker locker(lock);
            if(GameSaveProvider=op->GetResults()->Value)GameSaveContainer=GameSaveProvider->CreateContainer("Data");
            setStatus(LOGGED_IN); // call once everything is ready ('XboxCtx', members, 'GameSaveProvider', 'GameSaveContainer')

            xbox::services::system::xbox_live_user::add_sign_out_completed_handler([this](const xbox::services::system::sign_out_completed_event_args&) // setup auto-callback
            {
               // this will get called when game exits or user signs-out
               {
                  SyncLocker locker(lock);
                  GameSaveContainer=null;
                  GameSaveProvider=null;
                  XboxCtx =null;
                  XboxUser=null;
                 _me.clear();
                 _friends.clear(); _friends_known=_friends_getting=false;
               }
               setStatus(LOGGED_OUT);
            });
         }else setStatus(LOGGED_OUT);
      });
   }else setStatus(LOGGED_OUT);
#endif
}
/******************************************************************************/
void XBOXLive::logIn()
{
#if SUPPORT_XBOX_LIVE
   if(_status==LOGGED_OUT)
   {
      OSUser.get(); // !! get 'OSUser' here because we will need it inside the 'logInOK', and we can't obtain it there because it's called inside system callbacks and getting it would require callbacks again (nested calls are not allowed)
      SyncLocker locker(lock); if(_status==LOGGED_OUT)
      {
        _status=LOGGING_IN; // don't call 'setStatus' to avoid setting callback because we don't need it here
         if(!XboxUser)XboxUser=std::make_shared<xbox::services::system::xbox_live_user>();
         // try silent sign in first
         XboxUser->signin_silently().then([this](xbox::services::xbox_live_result<xbox::services::system::sign_in_result> result)
         {
            if(result.err())setStatus(LOGGED_OUT);else
            {
             C auto &payload=result.payload(); switch(payload.status())
               {
                  default: setStatus(LOGGED_OUT); break;
                  case xbox::services::system::sign_in_status::success: logInOK(); break;
                  case xbox::services::system::sign_in_status::user_interaction_required: // sign-in with UI
                  {
                     XboxUser->signin(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher).then([this](xbox::services::xbox_live_result<xbox::services::system::sign_in_result> loudResult)
                     {
                        if(loudResult.err())setStatus(LOGGED_OUT);else
                        {
                         C auto &payload=loudResult.payload(); switch(payload.status())
                           {
                              default: setStatus(LOGGED_OUT); break;
                              case xbox::services::system::sign_in_status::success: logInOK(); break;
                           }
                        }
                     }, concurrency::task_continuation_context::use_current());
                  }break;
               }
            }
         });
      }
   }
#endif
}
/******************************************************************************/
// CLOUD SAVES
/******************************************************************************/
Bool XBOXLive::cloudSupported()C
{
#if SUPPORT_XBOX_LIVE
   return GameSaveContainer!=null;
#else
   return false;
#endif
}
Long XBOXLive::cloudAvailableSize()C
{
#if SUPPORT_XBOX_LIVE
   if(GameSaveProvider)
   {
      SyncLockerEx locker(lock); if(GameSaveProvider)
      {
         auto op=GameSaveProvider->GetRemainingBytesInQuotaAsync();
         locker.off();
         Long size;
         SyncEvent event;
         op->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Long>([&](Windows::Foundation::IAsyncOperation<Long> ^op, Windows::Foundation::AsyncStatus status)
         {
            size=((status==Windows::Foundation::AsyncStatus::Completed) ? op->GetResults() : -1);
            event.on();
         });
         App.wait(event);
         return size;
      }
   }
#endif
   return -1;
}
Bool XBOXLive::cloudDel(C Str &file_name)
{
#if SUPPORT_XBOX_LIVE
   if(GameSaveContainer && file_name.is())
   {
      auto blobs=ref new Platform::Collections::Vector<Platform::String^>();
      blobs->Append(ref new Platform::String(file_name));
      SyncLockerEx locker(lock); if(GameSaveContainer)
      {
         auto op=GameSaveContainer->SubmitUpdatesAsync(null, blobs, null);
         locker.off();
         Bool ok;
         SyncEvent event;
         op->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Gaming::XboxLive::Storage::GameSaveOperationResult^>([&](Windows::Foundation::IAsyncOperation<Windows::Gaming::XboxLive::Storage::GameSaveOperationResult^> ^op, Windows::Foundation::AsyncStatus status)
         {
            ok=(status==Windows::Foundation::AsyncStatus::Completed && op->GetResults()->Status==Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok);
            event.on();
         });
         App.wait(event);
         return ok;
      }
   }
#endif
   return false;
}
#if SUPPORT_XBOX_LIVE
static Ptr GetBufferData(Windows::Storage::Streams::IBuffer ^buffer)
{
   byte *data=null;
   IUnknown *unknown=reinterpret_cast<IUnknown*>(buffer);
   Windows::Storage::Streams::IBufferByteAccess *bufferByteAccess=null;
   unknown->QueryInterface(_uuidof(Windows::Storage::Streams::IBufferByteAccess), (Ptr*)&bufferByteAccess);
   if(bufferByteAccess)
   {
      bufferByteAccess->Buffer(&data);
      bufferByteAccess->Release();
   }
   return data;
}
static Windows::Storage::Streams::IBuffer^ CreateBuffer(UInt size)
{
   Windows::Storage::Streams::Buffer ^buffer=ref new Windows::Storage::Streams::Buffer(size);
   if(buffer)buffer->Length=size;
   return buffer;
}
#endif
Bool XBOXLive::cloudSave(C Str &file_name, File &f, Cipher *cipher)
{
#if SUPPORT_XBOX_LIVE
   if(GameSaveContainer && file_name.is())
   {
      Long size=f.left();
      if(size<=INT_MAX)
         if(auto buffer=CreateBuffer(size))
            if(Ptr buffer_data=GetBufferData(buffer))
               if(f.getFast(buffer_data, size))
      {
         if(cipher)cipher->encrypt(buffer_data, buffer_data, size, 0);
         auto blobs=ref new Platform::Collections::Map<Platform::String^, Windows::Storage::Streams::IBuffer^>();
         blobs->Insert(ref new Platform::String(file_name), buffer);
         SyncLockerEx locker(lock); if(GameSaveContainer)
         {
            auto op=GameSaveContainer->SubmitUpdatesAsync(blobs->GetView(), null, null);
            locker.off();
            Bool ok;
            SyncEvent event;
            op->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Gaming::XboxLive::Storage::GameSaveOperationResult^>([&](Windows::Foundation::IAsyncOperation<Windows::Gaming::XboxLive::Storage::GameSaveOperationResult^> ^op, Windows::Foundation::AsyncStatus status)
            {
               ok=(status==Windows::Foundation::AsyncStatus::Completed && op->GetResults()->Status==Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok);
               event.on();
            });
            App.wait(event);
            return ok;
         }
      }
   }
#endif
   return false;
}
Bool XBOXLive::cloudLoad(C Str &file_name, File &f, Bool memory, Cipher *cipher)
{
#if SUPPORT_XBOX_LIVE
   if(GameSaveContainer && file_name.is())
   {
      auto blobs=ref new Platform::Collections::Vector<Platform::String^>();
      blobs->Append(ref new Platform::String(file_name));
      SyncLockerEx locker(lock); if(GameSaveContainer)
      {
         auto op=GameSaveContainer->GetAsync(blobs);
         locker.off();
         Windows::Gaming::XboxLive::Storage::GameSaveBlobGetResult ^blobs;
         SyncEvent event;
         op->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Gaming::XboxLive::Storage::GameSaveBlobGetResult^>([&](Windows::Foundation::IAsyncOperation<Windows::Gaming::XboxLive::Storage::GameSaveBlobGetResult^> ^op, Windows::Foundation::AsyncStatus status)
         {
            if(status==Windows::Foundation::AsyncStatus::Completed)blobs=op->GetResults();
            event.on();
         });
         App.wait(event);

         if(blobs && blobs->Status==Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok)
            if(blobs->Value->Size==1)
               if(auto buffer=blobs->Value->First()->Current->Value)
         {
            Long size=buffer->Length;
            Ptr buffer_data;
            if(size) // require data only if we have some size, in case null is returned for 0 sized buffers
            {
               buffer_data=GetBufferData(buffer);
               if(!buffer_data)goto error;
               if(cipher)cipher->decrypt(buffer_data, buffer_data, size, 0);
            }
            if(memory)f.writeMemFixed(size);
            if(f.put(buffer_data, size))return true;
         error:;
         }
      }
   }
#endif
   return false;
}
Bool XBOXLive::cloudFiles(MemPtr<CloudFile> files)C
{
#if SUPPORT_XBOX_LIVE
   if(GameSaveContainer)
   {
      SyncLockerEx locker(lock); if(GameSaveContainer)
      if(auto query=GameSaveContainer->CreateBlobInfoQuery(null))
      {
         locker.off();
         auto op=query->GetBlobInfoAsync();
         Windows::Gaming::XboxLive::Storage::GameSaveBlobInfoGetResult ^blobs;
         SyncEvent event;
         op->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Gaming::XboxLive::Storage::GameSaveBlobInfoGetResult^>([&](Windows::Foundation::IAsyncOperation<Windows::Gaming::XboxLive::Storage::GameSaveBlobInfoGetResult^> ^op, Windows::Foundation::AsyncStatus status)
         {
            if(status==Windows::Foundation::AsyncStatus::Completed)blobs=op->GetResults();
            event.on();
         });
         App.wait(event);

         if(blobs && blobs->Status==Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok)
         {
            files.setNum(blobs->Value->Size); REPA(files) // from the end because we might remove
            {
               if(auto blob_info=blobs->Value->GetAt(i))
               {
                  CloudFile &file=files[i];
                  file.name=blob_info->Name->Data();
                  file.size=blob_info->Size;
               }else files.remove(i, true); // keep order in case results are sorted
            }
            return true;
         }
      }
   }
#endif
   files.clear(); return false;
}
/******************************************************************************/
// FRIENDS
/******************************************************************************/
void XBOXLive::getFriends()
{
#if SUPPORT_XBOX_LIVE
   if(XboxCtx)
   {
      SyncLockerEx locker(lock); if(XboxCtx && !_friends_getting)
      {
        _friends_getting=true;
         auto task=XboxCtx->social_service().get_social_relationships();
         locker.off();
         task.then([this](xbox::services::xbox_live_result<xbox::services::xbox_live_result<xbox::services::social::xbox_social_relationship_result>> results)
         {
            if(!results.err())
            {
             C auto &result=results.payload(); if(!result.err())
               {
                  Memt<Friend> old; {SyncLocker locker(lock); old=_friends;}
                C auto &profiles=result.payload().items();
                  Memt<ULong > friend_ids; friend_ids.setNum(profiles.size());
                  Mems<Friend> friends   ; friends   .setNum(profiles.size()); FREPA(friends) // operate on temporary to swap fast under lock
                  {
                     Friend &user=friends[i];
                   C xbox::services::social::xbox_social_relationship &relationship=profiles[i];
                     user.clear();
                     user.id      =TextULong(WChar(relationship.xbox_user_id().c_str()));
                     user.favorite=relationship.is_favorite();
                     friend_ids[i]=user.id;
                     if(Friend *o=Find(old, user.id)) // if was previously known, reuse its data
                     {
                        Swap(user.name     , o->name     );
                        Swap(user.image_url, o->image_url);
                        Swap(user.score    , o->score    );
                     }
                  }
                  {
                     SyncLocker locker(lock);
                     Swap(_friends, friends);
                    _friends_known=true;
                  }
                  getUserProfile(friend_ids); // get profiles of all friends, after setting '_friends'
               }
            }
           _friends_getting=false;
            if(callback)callback(USER_FRIENDS, userID()); // notify that get friends finished
         });
      }
   }
#endif
}
Bool XBOXLive::getFriends(MemPtr<ULong> friend_ids)C
{
#if SUPPORT_XBOX_LIVE
   if(_friends_known)
   {
      SyncLocker locker(lock); if(_friends_known)
      {
         friend_ids.setNum(_friends.elms());
         REPAO(friend_ids)=_friends[i].id;
         return true;
      }
   }
#endif
   friend_ids.clear(); return false;
}
Bool XBOXLive::getFriends(MemPtr<Friend> friends)C
{
#if SUPPORT_XBOX_LIVE
   if(_friends_known)
   {
      SyncLocker locker(lock); if(_friends_known)
      {
         friends=T._friends;
         return true;
      }
   }
#endif
   friends.clear(); return false;
}
Str XBOXLive::userName(ULong user_id)C
{
#if SUPPORT_XBOX_LIVE
   if(user_id)
   {
      if(user_id==userID())return userName();
      if(_friends_known)
      {
         SyncLocker locker(lock);
         if(C Friend *user=CFind(_friends, user_id))return user->name;
      }
   }
#endif
   return S;
}
/******************************************************************************/
} // namespace EE
/******************************************************************************/
