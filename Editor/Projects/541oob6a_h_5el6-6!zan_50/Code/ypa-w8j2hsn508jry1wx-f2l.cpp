/******************************************************************************

   !! BEFORE RUNNING THIS TUTORIAL, PLEASE READ THE INSTRUCTIONS ON HOW TO SETUP XBOX LIVE !!
   !! INSTRUCTIONS ARE IN "Esenthel Engine\Net\Xbox Live" Header File                      !!

/******************************************************************************/
const bool IHaveReadInstructionsAtTheTop=false; // read the instructions above, then set this to true

InternetCache IC; // cache for images from the internet
Threads       Workers; // worker threads for importing images in the background thread
/******************************************************************************/
void XboxCallback(XBOXLive.EVENT event, ULong user_id)
{
   switch(event)
   {
      case XBOXLive.STATUS_CHANGED:
      {
         if(user_id==XboxLive.userID() && XboxLive.loggedIn())XboxLive.getFriends(); // after successful log in, request list of friends
      }break;
   }
}
/******************************************************************************/
void InitPre()
{
   ASSERT(IHaveReadInstructionsAtTheTop);
   EE_INIT();
   XboxLive.callback=XboxCallback;
   XboxLive.logIn();
}
/******************************************************************************/
bool Init()
{
   Workers.create(true, 1); // create workers
   IC.create(S, &Workers); // create internet cache
   Images.delayRemove(30); // this will delay automatic unloading of cached images
   return true;
}
void Shut()
{
}
/******************************************************************************/
bool Update()
{
   return true;
}
/******************************************************************************/
void Draw()
{
   D.clear(WHITE);
   
   Str status;
   switch(XboxLive.status())
   {
      case XBOXLive.LOGGED_OUT: status="Logged Out"; break;
      case XBOXLive.LOGGING_IN: status="Logging In"; break;
      case XBOXLive.LOGGED_IN : status="Logged In" ; break;
   }
   D.text(0, 0.9, status);
   if(XboxLive.status()==XBOXLive.LOGGED_IN)
   {
      D.text(0, 0.8, S+"Welcome "+XboxLive.userName());
      if(ImagePtr photo=IC.getImage(XboxLive.userImageURL()))photo->drawFit(Rect_U(0, 0.7, 0.3, 0.3));
      D.text(0, 0.3, S+"Cloud supported:"+XboxLive.cloudSupported()+", available size:"+XboxLive.cloudAvailableSize());
      D.text(0, 0.2, S+"Friends:");
      flt y=0.1;
      Memt<XBOXLive.Friend> friends; if(XboxLive.getFriends(friends))
      {
         TextStyleParams ts; ts.align.x=1;
         FREPA(friends)
         {
          C XBOXLive.Friend &user=friends[i];
            D.text(ts, -D.w()/2, y, S+i+", "+user.id+", "+user.name+(user.favorite ? "(*)" : null));
         }
      }else D.text(0, y, "Unknown");
   }
}
/******************************************************************************/
