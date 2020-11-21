/******************************************************************************/
#include "stdafx.h"

#define SUPPORT_XBOX_LIVE (WINDOWS_NEW && 1)

#if SUPPORT_XBOX_LIVE
#include "../ThirdPartyLibs/begin.h"
#define XSAPI_CPP 1
#define _NO_ASYNCRTIMP
#define _NO_PPLXIMP
#define _NO_XSAPIIMP

#define XBOX_LIVE_CREATORS_SDK
#include "../ThirdPartyLibs/Xbox Live/2018.6.20181010.2/xsapi/services.h" // !! if changing this to another version/path then also have to change Visual Studio project properties for Include Directories for all Configurations/Platforms !! this is because those LIB files are too big to be stored on GitHub and thus must be extracted manually, because of that to avoid compatibility issues between headers/libraries the paths to headers/libs must match so they must include version numbers
#include "../ThirdPartyLibs/end.h"
#endif

namespace EE{
/******************************************************************************
#if EE_PRIVATE
   extern std::shared_ptr<xbox::services::system::xbox_live_user> XboxUser;
   extern std::shared_ptr<xbox::services::xbox_live_context     > XboxCtx;
#endif
/******************************************************************************/
XBOXLive XboxLive;

#if SUPPORT_XBOX_LIVE
std::shared_ptr<xbox::services::system::xbox_live_user> XboxUser;
std::shared_ptr<xbox::services::xbox_live_context     > XboxCtx;
Windows::Gaming::XboxLive::Storage::GameSaveProvider   ^GameSaveProvider;
#endif
/******************************************************************************/
void XBOXLive::User::clear()
{
   id   = 0;
   score=-1;
   name     .clear();
   image_url.clear();
}
void XBOXLive::getUserProfile(ULong user_id)
{
#if SUPPORT_XBOX_LIVE
   if(user_id && XboxCtx)
   {
      string_t temp=TextInt(user_id);
      SyncLockerEx lock(_lock); if(XboxCtx)
      {
         auto task=XboxCtx->profile_service().get_user_profile(temp);
         lock.off();
         task.then([this](xbox::services::xbox_live_result<xbox::services::xbox_live_result<xbox::services::social::xbox_user_profile>> results)
         {
            if(!results.err())
            {
               auto result=results.payload(); if(!result.err())
               {
                  auto profile=result.payload();
                  auto user_id=TextULong(WChar(profile.xbox_user_id().c_str()));
                  if(user_id==T.userID())
                  {
                     // store in temporaries so later we can move/swap into profile fast
                     ULong score=TextULong(WChar(profile.gamerscore().c_str()));
                     Str image_url=S+profile.game_display_picture_resize_uri().to_string().c_str()+"&w=424"; // default size is around 1080 which is not needed, limit to 424 (other options are 208, 64)
                     /*auto user_app_name =profile. app_display_name().c_str();
                     auto user_game_name=profile.game_display_name().c_str();
                     auto tag=profile.gamertag().c_str();
                     auto user_app_pic=profile.app_display_picture_resize_uri().to_string().c_str();*/

                     {
                        SyncLocker lock(_lock);
                       _me.score=score;
                        Swap(_me.image_url, image_url);
                     }
                  }
               }
            }
            if(callback)(*callback)(USER_PROFILE);
         });
      }
   }
#endif
}
void XBOXLive::getUserProfile()
{
   getUserProfile(userID());
}
void XBOXLive::setStatus(STATUS status)
{
   if(T._status!=status){T._status=status; if(callback)(*callback)(STATUS_CHANGED);}
}
void XBOXLive::logInOk()
{
#if SUPPORT_XBOX_LIVE
   SyncLockerEx lock(_lock);
   if(!XboxCtx)XboxCtx=std::make_shared<xbox::services::xbox_live_context>(XboxUser);
  _me.id  =TextULong(WChar(XboxUser->xbox_user_id().c_str()));
  _me.name=XboxUser->gamertag().c_str();
 //Str age=XboxUser->age_group().c_str(); // can return: "Adult", ..
   //getUserProfile(userID());
   //auto storage=XboxCtx->title_storage_service();
   if(auto config=XboxCtx->application_config())
   {
      /*auto app_id=config->title_id();
      auto scid=config->scid().c_str();
      auto env =config->environment().c_str();
      auto sandbox=config->sandbox().c_str();*/

      auto task=Windows::Gaming::XboxLive::Storage::GameSaveProvider::GetForUserAsync(OSUser.get(), ref new Platform::String(config->scid().c_str()));
      lock.off();
      task->Completed=ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Gaming::XboxLive::Storage::GameSaveProviderGetResult^>([=](Windows::Foundation::IAsyncOperation<Windows::Gaming::XboxLive::Storage::GameSaveProviderGetResult^> ^operation, Windows::Foundation::AsyncStatus status)
      {
         if(status==Windows::Foundation::AsyncStatus::Completed)GameSaveProvider=operation->GetResults()->Value;

         setStatus(LOGGED_IN); // call after 'XboxCtx', setting members and having 'GameSaveProvider'

         xbox::services::system::xbox_live_user::add_sign_out_completed_handler([this](const xbox::services::system::sign_out_completed_event_args&) // setup auto-callback
         {
            // this will get called when game exits or user signs-out
            SyncLocker lock(_lock);
            GameSaveProvider=null;
            XboxCtx =null;
            XboxUser=null;
           _me.clear();
            setStatus(LOGGED_OUT);
         });
      });
   }
#endif
}
/******************************************************************************/
void XBOXLive::logIn()
{
#if SUPPORT_XBOX_LIVE
   if(_status==LOGGED_OUT)
   {
      OSUser.get(); // !! get 'OSUser' here because we will need it inside the 'logInOk', and we can't obtain it there because it's called inside system callbacks and getting it would require callbacks again (nested calls are not allowed)
      SyncLocker lock(_lock); if(_status==LOGGED_OUT)
      {
        _status=LOGGING_IN; // don't call 'setStatus' to avoid setting callback because we don't need it here
         if(!XboxUser)XboxUser=std::make_shared<xbox::services::system::xbox_live_user>();
         // try silent sign in first
         XboxUser->signin_silently().then([this](xbox::services::xbox_live_result<xbox::services::system::sign_in_result> result)
         {
            if(result.err())setStatus(LOGGED_OUT);else
            {
               auto payload=result.payload(); switch(payload.status())
               {
                  default: setStatus(LOGGED_OUT); break;
                  case xbox::services::system::sign_in_status::success: logInOk(); break;
                  case xbox::services::system::sign_in_status::user_interaction_required: // sign-in with UI
                  {
                     XboxUser->signin(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher).then([this](xbox::services::xbox_live_result<xbox::services::system::sign_in_result> loudResult) // use task_continuation_context::use_current() to make the continuation task running in current apartment 
                     {
                        if(loudResult.err())setStatus(LOGGED_OUT);else
                        {
                           auto payload=loudResult.payload(); switch(payload.status())
                           {
                              default: setStatus(LOGGED_OUT); break;
                              case xbox::services::system::sign_in_status::success: logInOk(); break;
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
Bool XBOXLive::cloudSupported()C {return GameSaveProvider!=null;}
/******************************************************************************/
} // namespace EE
/******************************************************************************/
