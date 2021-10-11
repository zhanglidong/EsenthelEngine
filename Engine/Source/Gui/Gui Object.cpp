﻿/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
// GUI PARENT CHILD
/******************************************************************************/
GuiPC::GuiPC(C GuiPC &old, Bool visible, Bool enabled)
{
   T=old;
   T.visible&=visible;
   T.enabled&=enabled;
}
/******************************************************************************/
// GUI OBJECT
/******************************************************************************/
static Int CompareLevel(C GuiObj &a, C GuiObj &b)
{
                                   if(Int c=Compare(a.         baseLevel(), b.         baseLevel()))return c;
   if(a.isWindow() && b.isWindow())if(Int c=Compare(a.asWindow().  level(), b.asWindow().  level()))return c;
   if(a.isMenu  () && b.isMenu  ())if(Int c=Compare(b.asMenu  ().parents(), a.asMenu  ().parents()))return c; // order swapped, sort Menu by parents, so Menus attached to Windows are processed before those attached to Desktop, so Window Menu's process keyboard shortcuts first, and unprocessed shortcuts are checked later by Desktop Menu's.
   return 0;
}
/******************************************************************************/
void GuiObj::zero()
{
   user      =null;
  _type      =GO_NONE;
  _visible   =false;
  _disabled  =false;
  _base_level=GBL_DEFAULT;
//_used      =0; don't clear this here, because it's a ref count, instead clear this in constructor
  _desc.clear();
  _parent=null;
  _rect.zero();
}
GuiObj& GuiObj::del()
{
   DEBUG_BYTE_LOCK(_used);
   deactivate();
   if(contains(Gui.msSrc()))Gui._ms_src=null;
   if(_parent)*_parent-=T; // detach from the parent first without clearing members as it may use them

   zero(); return T;
}
GuiObj::~GuiObj()
{
   DEBUG_ASSERT(_used==0, "'GuiObj' is being destroyed while being used");
   del();
}
GuiObj::GuiObj()
{
   zero();
   if(DEBUG)_used=0;
}
/******************************************************************************/
GuiObj& GuiObj::create(C GuiObj &src)
{
   // don't use 'type' as they may be set to GO_NONE if objects were not created yet
   // process extended classes first, because for example 'ComboBox' can be casted to both 'Button' and 'ComboBox'
   if(CAST(ComboBox , this) && CAST(C ComboBox , &src))asComboBox().create(src.asComboBox());else
   if(CAST(Tab      , this) && CAST(C Tab      , &src))asTab     ().create(src.asTab     ());else

   if(CAST(Button   , this) && CAST(C Button   , &src))asButton  ().create(src.asButton  ());else
   if(CAST(CheckBox , this) && CAST(C CheckBox , &src))asCheckBox().create(src.asCheckBox());else
   if(CAST(GuiCustom, this) && CAST(C GuiCustom, &src))asCustom  ().create(src.asCustom  ());else
   if(CAST(Desktop  , this) && CAST(C Desktop  , &src))asDesktop ().create(src.asDesktop ());else
   if(CAST(GuiImage , this) && CAST(C GuiImage , &src))asImage   ().create(src.asImage   ());else
   if(CAST(_List    , this) && CAST(C _List    , &src))asList    ().create(src.asList    ());else
   if(CAST(Menu     , this) && CAST(C Menu     , &src))asMenu    ().create(src.asMenu    ());else
   if(CAST(MenuBar  , this) && CAST(C MenuBar  , &src))asMenuBar ().create(src.asMenuBar ());else
   if(CAST(Progress , this) && CAST(C Progress , &src))asProgress().create(src.asProgress());else
   if(CAST(Region   , this) && CAST(C Region   , &src))asRegion  ().create(src.asRegion  ());else
   if(CAST(SlideBar , this) && CAST(C SlideBar , &src))asSlideBar().create(src.asSlideBar());else
   if(CAST(Slider   , this) && CAST(C Slider   , &src))asSlider  ().create(src.asSlider  ());else
   if(CAST(Tabs     , this) && CAST(C Tabs     , &src))asTabs    ().create(src.asTabs    ());else
   if(CAST(Text     , this) && CAST(C Text     , &src))asText    ().create(src.asText    ());else
   if(CAST(TextBox  , this) && CAST(C TextBox  , &src))asTextBox ().create(src.asTextBox ());else
   if(CAST(TextLine , this) && CAST(C TextLine , &src))asTextLine().create(src.asTextLine());else
   if(CAST(Viewport , this) && CAST(C Viewport , &src))asViewport().create(src.asViewport());else
   if(CAST(Window   , this) && CAST(C Window   , &src))asWindow  ().create(src.asWindow  ());
   return T;
}
void GuiObj::copyParams(C GuiObj &src)
{
   user      =src. user;
  _visible   =src._visible;
  _disabled  =src._disabled;
  _base_level=src._base_level;
//_type      =src._type; cannot copy type because this is called inside GuiObj create methods, and 'Button' can be called with create from for example 'ComboBox' which has GO_COMBOBOX
  _desc      =src._desc;
  _rect      =src._rect;
}
/******************************************************************************/
void GuiObj::operator+=(GuiObj &child)
{
   if(child.is()             // if child exists
   && this!=&child           // is not this
   && !child.contains(this)) // child doesn't contain us
   {
      DEBUG_BYTE_LOCK(_used);
      switch(type())
      {
         case GO_DESKTOP: asDesktop().addChild(child); break;
       //case GO_LIST   : asList   ().addChild(child); break; this requires specifying row and optionally column, so we can't do it here
         case GO_REGION : asRegion ().addChild(child); break;
         case GO_TAB    : asTab    ().addChild(child); break;
         case GO_WINDOW : asWindow ().addChild(child); break;
      }
   }
}
void GuiObj::operator-=(GuiObj &child)
{
   DEBUG_BYTE_LOCK(_used);
   switch(type())
   {
      case GO_DESKTOP: asDesktop().removeChild(child); break;
      case GO_LIST   : asList   ().removeChild(child); break;
      case GO_REGION : asRegion ().removeChild(child); break;
      case GO_TAB    : asTab    ().removeChild(child); break;
      case GO_TABS   : asTabs   ().removeChild(child); break;
      case GO_WINDOW : asWindow ().removeChild(child); break;
   }
}
void GuiObj::operator+=(GuiObjs &children)
{
   FREPA(children._objs) // add from start to preserve order
   {
      GuiObjs::Obj &goi=children._objs[i];
      if(!goi.parent_type)if(GuiObj *go=children.go(goi.type, goi.index))T+=*go; // add all main (which don't have a parent)
   }
}
void GuiObj::operator-=(GuiObjs &children)
{
   REPA(children._objs) // remove from end to minimize overhead (moving indexes)
   {
      GuiObjs::Obj &goi=children._objs[i];
      if(!goi.parent_type)if(GuiObj *go=children.go(goi.type, goi.index))T-=*go; // remove all main (which don't have a parent)
   }
}
GuiObjChildren* GuiObj::children()
{
   switch(_type)
   {
      case GO_DESKTOP: return &asDesktop()._children;
      case GO_LIST   : return &asList   ()._children;
      case GO_REGION : return &asRegion ()._children;
      case GO_TAB    : return &asTab    ()._children;
      case GO_WINDOW : return &asWindow ()._children;
   }
   return null;
}
GuiObj* GuiObj::child(Int i)
{
   if(GuiObjChildren *children=T.children())
      if(InRange(i, *children))
         return (*children)[i];

   if(isTabs() && InRange(i, asTabs()))return &asTabs().tab(i);

   return null;
}
Int GuiObj::childNum()
{
   if(GuiObjChildren *children=T.children())return children->children.elms();
   if(isTabs())return asTabs().tabs();
   return 0;
}
void GuiObj::notifyChildrenOfClientRectChange(C Rect *old_client, C Rect *new_client)
{
   DEBUG_BYTE_LOCK(_used);
   REP(childNum())if(GuiObj *go=child(i))go->parentClientRectChanged(old_client, new_client);
}
void GuiObj::notifyParentOfRectChange(C Rect &old_rect, Bool old_visible)
{
   DEBUG_BYTE_LOCK(_used);
   if(parent())parent()->childRectChanged(old_visible ? &old_rect : null, visible() ? &rect() : null, T);
}
/******************************************************************************/
Bool GuiObj::contains(C GuiObj *child)C // !! this method is safe to work with "this==null" upon changing that, fix all calls to this !!
{
   for(; child; child=child->owner())if(child==this)return true;
   return false;
}
GuiObj* GuiObj::last          (GUI_OBJ_TYPE type) {GuiObj *last=null; for(GuiObj *go=this; go; go=go->owner())if(go->type()==type)last=go; return last;}
GuiObj* GuiObj::first         (GUI_OBJ_TYPE type) {                   for(GuiObj *go=this; go; go=go->owner())if(go->type()==type)         return   go; return null;} // !! this method is safe to work with "this==null" upon changing that, fix all calls to this !!
GuiObj* GuiObj::firstNon      (GUI_OBJ_TYPE type) {                   for(GuiObj *go=this; go; go=go->owner())if(go->type()!=type)         return   go; return null;}
GuiObj* GuiObj::firstContainer(                 )
{
   for(GuiObj *go=this; go; go=go->owner())switch(go->type())
   {
      case GO_DESKTOP:
      case GO_REGION :
      case GO_WINDOW : return go;
   }
   return null;
}
GuiObj* GuiObj::firstKbParent()
{
   for(GuiObj *go=parent(); go; go=go->parent())switch(go->type())
   {
      case GO_DESKTOP:
      case GO_LIST   :
      case GO_MENU   :
      case GO_REGION :
      case GO_TAB    :
      case GO_WINDOW : return go;
   }
   return null;
}
Region* GuiObj::firstScrollableRegion()
{
   for(GuiObj *go=this; go; go=go->parent())if(go->isRegion())
   {
      Region &region=go->asRegion();
      if(region.slidebar[0]._usable || region.slidebar[1]._usable)return &region;
   }
   return null;
}
/******************************************************************************/
Int GuiObj::parents()C
{
   Int    parents=0; for(GuiObj *go=owner(); go; go=go->owner())parents++;
   return parents;
}
/******************************************************************************/
Bool GuiObj::kbCatch()C
{
   switch(type())
   {
      case GO_MENU    :
      case GO_MENU_BAR:
      case GO_REGION  :
      case GO_TABS    :
      case GO_TEXTBOX :
      case GO_TEXTLINE:
      case GO_VIEWPORT:
      case GO_WINDOW  : return visible() && enabled();

      case GO_BUTTON  : return asButton  ().focusable() && visible() && enabled();
      case GO_CHECKBOX: return asCheckBox().focusable() && visible() && enabled();
      case GO_COMBOBOX: return asComboBox().focusable() && visible() && enabled();
      case GO_CUSTOM  : return asCustom  ().focusable() && visible() && enabled();
      case GO_SLIDEBAR: return asSlideBar().focusable() && visible() && enabled();
      case GO_SLIDER  : return asSlider  ().focusable() && visible() && enabled();
      case GO_TAB     : return asTab     ().focusable() && visible() && enabled();
      case GO_LIST    : return                             visible() && enabled();
      default         : return false;
   }
}
/******************************************************************************/
static void AdjustGuiKb() // call when 'Gui.kb' got changed (or just 'kbSet' getting called to force showing soft keyboard when clicking on 'TextLine' or 'TextBox', etc.)
{
   Gui._window=&Gui.kb()->first(GO_WINDOW)->asWindow();
   Kb.setVisible();
}
GuiObj& GuiObj::kbSet() // this means setting keyboard focus to this element
{
   if(is()) // allow setting focus only if created, to keep consistency with clearing kb focus when deleting object
   {
      DEBUG_BYTE_LOCK(_used);

      if(MOBILE // do this for all types on Mobile platforms, because of the soft keyboard overlay, which could keep popping up annoyingly and occlude big portion of the screen
      || isDesktop() || isList()) // if this is a 'Desktop' or a 'List' then clear the sub kb focus so children won't have it, and only this object will
         if(GuiObjChildren *children=T.children())children->kb=null;

      // set kb from 'this' to parents
      for(GuiObj *go=this; go; )
      {
         GuiObj *parent=go->firstKbParent();
         if(parent)
         {
            GuiObj *kb, *cur=go; // find first object between 'parent' .. 'go' that can catch keyboard focus
            for(;;)
            {
               if(cur->kbCatch()){kb=cur; break;}
               if(cur->isTab() && cur->asTab()._children.kb){kb=cur; break;} // force setting tab when it owns a child with keyboard focus, do not set 'kb' to child directly, because kb pointers must be only 1 level !!
               cur=cur->parent();
               if(cur==parent || !cur){kb=null; break;} // if reached the parent, or for some reason null, then stop
            }
            switch(parent->type())
            {
               case GO_MENU   : parent->asMenu   ().         _kb=kb; break;
               case GO_DESKTOP: parent->asDesktop()._children.kb=kb; break;
               case GO_LIST   : parent->asList   ()._children.kb=kb; break;
               case GO_REGION : parent->asRegion ()._children.kb=kb; break;
               case GO_TAB    : parent->asTab    ()._children.kb=kb; break;
               case GO_WINDOW : parent->asWindow ()._children.kb=kb; break;
            }
         }
         go=parent;
      }

      // set kb from root to leaf
      for(GuiObj *go=Gui.desktop(); go; )
      {
         Gui._kb=go;
         switch(go->type())
         {
            case GO_MENU_BAR: {MenuBar &menubar=go->asMenuBar(); Menu *menu=(InRange(menubar._lit, menubar.elms()) ? &menubar.elm(menubar._lit).menu : null); go=menu;} break;
            case GO_TABS    : {Tabs    &tabs   =go->asTabs   (); Tab  *tab =(InRange(tabs()      , tabs          ) ? &tabs   .tab(tabs()      )      : null); go=((tab && (tab->kbCatch() || tab->_children.kb)) ? tab : null);} break; // set this tab only if it can catch keyboard focus or has a child with kb focus
            case GO_TAB     :                go=go->asTab    ()._children.kb; break;
            case GO_DESKTOP :                go=go->asDesktop()._children.kb; break;
            case GO_LIST    :                go=go->asList   ()._children.kb; break;
            case GO_REGION  :                go=go->asRegion ()._children.kb; break;
            case GO_WINDOW  :                go=go->asWindow ()._children.kb; break;
            case GO_MENU    :                go=go->asMenu   ().         _kb; break;
            default         :                go=                        null; break;
         }
      }

      AdjustGuiKb();
   }
   return T;
}
GuiObj& GuiObj::kbClear()
{
   DEBUG_BYTE_LOCK(_used);

   // remove from parents kb focus
   if(GuiObj *kb=firstKbParent())switch(kb->type())
   {
      case GO_DESKTOP: { Desktop &desktop=kb->asDesktop(); if(desktop._children.kb==this)desktop._children.kb=null      ;} break;
      case GO_LIST   : {_List    &list   =kb->asList   (); if(list   ._children.kb==this)list   ._children.kb=null      ;} break;
      case GO_MENU   : { Menu    &menu   =kb->asMenu   (); if(menu   .         _kb==this)menu   .         _kb=&menu.list;} break;
      case GO_REGION : { Region  &region =kb->asRegion (); if(region ._children.kb==this)region ._children.kb=null      ;} break;
      case GO_TAB    : { Tab     &tab    =kb->asTab    (); if(tab    ._children.kb==this)tab    ._children.kb=null      ;} break;
      case GO_WINDOW : { Window  &window =kb->asWindow (); if(window ._children.kb==this)window ._children.kb=null      ;} break;
   }

   // adjust global focus
   if(contains(Gui.kb()))
   {
      Gui._kb=(_parent ? _parent : null);
      for(; Gui.kb() && !Gui.kb()->kbCatch(); )Gui._kb=Gui.kb()->parent();
      AdjustGuiKb();
   }
   return T;
}
/******************************************************************************/
Int GuiObj::compareLevel(C GuiObj &obj)C
{
   if(parent() && obj.parent()==parent())
   {
      if(GuiObjChildren *children=parent()->children())return children->compareLevel(T, obj);

      // check for single 'Tab's in 'Tabs' (this is required for correct order when saving gui objects)
      if(parent()->isTabs() && isTab() && obj.isTab())
         return parent()->asTabs()._tabs.validIndex(&asTab()) - parent()->asTabs()._tabs.validIndex(&obj.asTab());
   }
   return 0;
}
GuiObj& GuiObj::               validateLevel(             )  {if(parent())if(GuiObjChildren *children=parent()->children())       children->               validateLevel(T); return T;}
Bool    GuiObj::partiallyOccludedInSameLevel(             )C {if(parent())if(GuiObjChildren *children=parent()->children())return children->partiallyOccludedInSameLevel(T); return false;}
Bool    GuiObj::moveUp                      (             )C {if(parent())if(GuiObjChildren *children=parent()->children())return children->moveUp                      (T); return false;}
Bool    GuiObj::moveDown                    (             )C {if(parent())if(GuiObjChildren *children=parent()->children())return children->moveDown                    (T); return false;}
GuiObj& GuiObj::moveToTop                   (             )  {if(parent())if(GuiObjChildren *children=parent()->children())       children->moveToTop                   (T); return T;}
GuiObj& GuiObj::moveToBottom                (             )  {if(parent())if(GuiObjChildren *children=parent()->children())       children->moveToBottom                (T); return T;}
GuiObj& GuiObj::moveAbove                   (C GuiObj &obj)  {if(parent() && obj.parent()==parent())if(GuiObjChildren *children=parent()->children())children->moveAbove(T, obj); return T;}
GuiObj& GuiObj::moveBelow                   (C GuiObj &obj)  {if(parent() && obj.parent()==parent())if(GuiObjChildren *children=parent()->children())children->moveBelow(T, obj); return T;}
GuiObj& GuiObj::windowsToTop                (             )
{
   for(GuiObj *c=this; c; )
   {
      GuiObj *parent=c->parent();
      if(parent)switch(c->type())
      {
         case GO_WINDOW: if(GuiObjChildren *children=parent->children())children->moveToTop(*c); break;
      }
      c=parent;
   }
   return T;
}
Bool GuiObj::visibleFull()C
{
   if(hidden())return false; // if we're starting from 'Tab' then check this, because 'Tab' visibility is ignored below
   for(C GuiObj *go=this; ; )
   {
      if(go->isTab())
      {
         if(go->parent() && go->parent()->isTabs()) // check if parent is 'Tabs' and it's set to that tab
         {
            Tabs &tabs    =go->parent()->asTabs();
            Tab  *tabs_tab=tabs._tabs.addr(tabs()); // get active 'Tab' in 'Tabs'
            if(   tabs_tab!=go)return false; // if active tab is not the processed one, then it means it's not selected and children should be hidden
         }
      }else // ignore visibility of a 'Tab'
      {
         if(go->hidden())return false;
      }
      go=go->parent(); if(!go)return true;
   }
}
Bool GuiObj::visibleOnActiveDesktop()C
{
   if(hidden())return false; // if we're starting from 'Tab' then check this, because 'Tab' visibility is ignored below
   for(C GuiObj *go=this; ; )
   {
      if(go->isTab())
      {
         if(go->parent() && go->parent()->isTabs()) // check if parent is 'Tabs' and it's set to that tab
         {
            Tabs &tabs    =go->parent()->asTabs();
            Tab  *tabs_tab=tabs._tabs.addr(tabs()); // get active 'Tab' in 'Tabs'
            if(   tabs_tab!=go)return false; // if active tab is not the processed one, then it means it's not selected and children should be hidden
         }
      }else // ignore visibility of a 'Tab'
      {
         if(go->hidden())return false;
      }
    C GuiObj *parent=go->parent(); if(!parent)return go==Gui.desktop(); // if no parent, then check if last object is active desktop
      go=parent;
   }
}
Bool GuiObj::enabledFull()C
{
   for(C GuiObj *go=this; go; go=go->parent())if(go->disabled())return false;
   return true;
}
Bool GuiObj::disabledFull()C
{
   for(C GuiObj *go=this; go; go=go->parent())if(go->disabled())return true;
   return false;
}
/******************************************************************************/
GuiObj& GuiObj::hide()
{
   if(visible())
   {
      DEBUG_BYTE_LOCK(_used);
     _visible=false;
      deactivate();
      notifyParentOfRectChange(rect(), true);
   }
   return T;
}
GuiObj& GuiObj::show()
{
   if(hidden() && is()) // can't show a deleted object
   {
      DEBUG_BYTE_LOCK(_used);
     _visible=true;
      notifyParentOfRectChange(rect(), false);
   }
   return T;
}
GuiObj& GuiObj::activate()
{
   DEBUG_BYTE_LOCK(_used);

   // hide all menus (from child to parent) until we reach any that belongs to 'this'
   for(GuiObj *menu=Gui.menu(); menu && menu->isMenu(); menu=menu->parent())if(!menu->contains(this))menu->hide();else break;

   if(isDesktop())Gui._desktop=&asDesktop();

   // show
   if(isWindow()) // treat window as a special case because it supports fading
   {
      Window &window=asWindow(); switch(window._fade_type)
      {
         case FADE_OUT : window.fadeIn(); break; // if window was fading out then fade in
         case FADE_NONE: window.show  (); break; // show immediately
       //case FADE_IN  :                  break; // do nothing and let it keep fading in
      }
   }else show();

   kbSet();
   windowsToTop();

   // check if the object is fully visible
   if(visibleFull())
   {
      Gui._menu=&first(GO_MENU)->asMenu();
   }
   return T;
}
GuiObj& GuiObj::deactivate()
{
   DEBUG_BYTE_LOCK(_used);

   // clear keyboard focus
   kbClear();

   // hide all menus that belong to 'this'
   for(GuiObj *menu=Gui.menu(); menu && menu->isMenu(); menu=menu->parent())if(contains(menu))menu->hide();else break;

   if(contains(Gui.menu           ()))Gui._menu            =&_parent->first(GO_MENU  )->asMenu  ();
   if(contains(Gui.windowLit      ()))Gui._window_lit      =&_parent->first(GO_WINDOW)->asWindow();
   if(contains(Gui.ms             ()))Gui._ms              =null;
   if(contains(Gui.msLit          ()))Gui._ms_lit          =_parent;
   if(contains(Gui.wheel          ()))Gui._wheel           =null;
   if(contains(Gui._overlay_textline))Gui._overlay_textline=null;
   if(contains(Gui.      _desc      ))Gui.      _desc      =null;
   if(contains(Gui._touch_desc      ))Gui._touch_desc      =null;
   REPA(Touches)if(contains(Touches[i].guiObj()))Touches[i]._gui_obj=null;
   return T;
}
GuiObj& GuiObj::setText()
{
   REP(childNum())child(i)->setText();
   return T;
}
/******************************************************************************/
// GET / SET
/******************************************************************************/
GuiObj& GuiObj::enabled(Bool enabled)
{
   T._disabled=!enabled;
   return T;
}
GuiObj& GuiObj::disabled(Bool disabled)
{
   T._disabled=disabled;
   return T;
}
GuiObj& GuiObj::rect(C Rect &rect)
{
   if(T.rect()!=rect)
   {
      DEBUG_BYTE_LOCK(_used);
      Rect old_client=localClientRect(), old_rect=T.rect(); T._rect=rect;
      Rect new_client=localClientRect();
      notifyChildrenOfClientRectChange(&old_client, &new_client);
      notifyParentOfRectChange        ( old_rect  ,  visible() );
   }
   return T;
}
GuiObj& GuiObj::hidden(  Bool  hidden) {return visible(!hidden);}
GuiObj& GuiObj::desc  (C Str  &desc  ) {T._desc=desc; return T;}
GuiObj& GuiObj::move  (C Vec2 &delta ) {if(delta.any()){Rect old_rect=rect(); T._rect+=delta; notifyParentOfRectChange(old_rect, visible());} return T;}
GuiObj& GuiObj::pos   (C Vec2 &pos   ) {return move(pos-T.pos  ());}
GuiObj& GuiObj::posRU (C Vec2 &pos   ) {return move(pos-T.posRU());}
GuiObj& GuiObj::posLD (C Vec2 &pos   ) {return move(pos-T.posLD());}
GuiObj& GuiObj::posL  (C Vec2 &pos   ) {return move(pos-T.posL ());}
GuiObj& GuiObj::posR  (C Vec2 &pos   ) {return move(pos-T.posR ());}
GuiObj& GuiObj::posD  (C Vec2 &pos   ) {return move(pos-T.posD ());}
GuiObj& GuiObj::posU  (C Vec2 &pos   ) {return move(pos-T.posU ());}
GuiObj& GuiObj::posC  (C Vec2 &pos   ) {return move(pos-T.posC ());}
GuiObj& GuiObj::resize(C Vec2 &delta ) {return rect(Rect(rect().min.x, Min(rect().max.y, rect().min.y-delta.y), Max(rect().min.x, rect().max.x+delta.x), rect().max.y));}
GuiObj& GuiObj::size  (C Vec2 &size  ) {return rect(Rect(rect().min.x,     rect().max.y        -Max(0, size.y),     rect().min.x        +Max(0, size.x), rect().max.y));}

Vec2 GuiObj::screenClientPos ()C {return screenPos()+clientOffset();}
Rect GuiObj::screenClientRect()C {return Rect_LU(screenClientPos(), clientSize());}
Rect GuiObj::screenRect      ()C {return Rect_LU(screenPos      (),       size());}
Vec2 GuiObj::screenPos       ()C
{
   Vec2 pos=T.pos();
      C GuiObj *go=this;
again:
   if(C GuiObj *parent=go->parent())
   {
      switch(parent->type())
      {
         case GO_WINDOW: pos+=parent->asWindow().clientRect().lu(); break;
         case GO_MENU  : pos+=parent->asMenu  ().clientRect().lu(); goto finished; // menus are always on top

         case GO_REGION:
         {
          C Region &region=parent->asRegion();
            if(go!=&region.slidebar[0]
            && go!=&region.slidebar[1]
            && go!=&region.view       )
            {
               pos  +=region.clientRect().lu();
               pos.x-=region.slidebar[0].offset();
               pos.y+=region.slidebar[1].offset();
            }
         }break;

         case GO_LIST:
         {
            pos+=parent->asList().childOffset(T);
            pos+=parent->pos();
         }break;
      }
      go=parent; goto again;
   }

finished:;
   return pos;
}
Vec2 GuiObj::clientOffset()C
{
   switch(type())
   {
      case GO_MENU  : return asMenu  ().clientRect().lu()-pos();
      case GO_WINDOW: return asWindow().clientRect().lu()-pos();
      case GO_REGION: return asRegion().clientRect().lu()-pos();
   }
   return 0;
}
Vec2 GuiObj::clientSize()C
{
   switch(type())
   {
      case GO_MENU  : return asMenu  ().clientSize();
      case GO_WINDOW: return asWindow().clientSize();
      case GO_REGION: return asRegion().clientSize();
      default       : return size();
   }
}
Rect GuiObj::localClientRect()C
{
 C GuiObj *go=this;
   for(; go; )
   {
      if(go->isTab ())go=go->parent();else
      if(go->isTabs())go=go->parent();else break;
   }
   if(go)switch(go->type())
   {
      case GO_DESKTOP: return go->rect();
      default        : return Rect_LU(Vec2Zero, go->clientSize());
   }
   return RectZero;
}
CChar*  GuiObj::typeName()C {return GuiObjTypeName(type());}
GuiObj& GuiObj::baseLevel(Int level) {if(_base_level!=level){_base_level=level; validateLevel();} return T;}
/******************************************************************************/
// MAIN
/******************************************************************************/
GuiObj* GuiObj::test(C GuiPC &gpc, C Vec2 &pos, GuiObj* &mouse_wheel)
{
   return (/*gpc.visible &&*/ visible() && Cuts(pos, (rect()+gpc.offset)&gpc.clip) /*&& is()*/) ? this : null; // no need to check for 'is' because we already check for 'visible' and deleted objects can't be visible
}
/******************************************************************************/
#define NEAREST_AREA_MIN 0.333f // fraction of original area that must be visible to detect
static Bool Exclude(Rect &rect, C Rect &cover) // adjust 'rect' to don't have any parts belonging to 'cover' !! assumes that rectangles intersect !!
{
   Rect part[4]; // we can generate up to 4 rectangles
   Int  parts=0; // how many generated
   if(cover.min.x>rect.min.x)part[parts++].set( rect.min.x,  rect.min.y, cover.min.x,  rect.max.y); // set left  part
   if(cover.max.x<rect.max.x)part[parts++].set(cover.max.x,  rect.min.y,  rect.max.x,  rect.max.y); // set right part
   if(cover.min.y>rect.min.y)part[parts++].set( rect.min.x,  rect.min.y,  rect.max.x, cover.min.y); // set lower part
   if(cover.max.y<rect.max.y)part[parts++].set( rect.min.x, cover.max.y,  rect.max.x,  rect.max.y); // set upper part
   if(parts) // if have any parts
   { // find biggest part (with biggest area)
      Int biggest     =parts-1; // assume it's the last one
      Flt biggest_area=part[biggest].area(); // calc its area
      REP(parts-1) // iterate all others
      {
         Flt area=part[i].area();
         if( area>biggest_area){biggest_area=area; biggest=i;}
      }
      rect=part[biggest]; // adjust rectangle to the biggest part
      return true; // success
   }
   return false; // empty
}
void GuiObjNearest::cover(C Rect &rect)
{
   REPA(nearest)
   {
      auto &obj=nearest[i];
      Rect &obj_rect=obj.rect;
      if(Cover(obj_rect, rect))
      {
         if(Exclude(obj_rect, rect) && obj_rect.area()>=obj.area_min)obj.recalc=true; // if still have some rectangle left and its area is big enough, then mark that it needs to be recalculated
         else                                                      nearest.remove(i); // just remove it
      }
   }
   if(state) // if already encountered starting object, which means that at this stage we can occlude it
      if(Cover(T.rect, rect))
   {
      state=2; // set that we've modified it
      if(!Exclude(T.rect, rect))T.rect=T.plane.pos; // if completely covered then set as a single point
   }
}
Bool GuiObjNearest::test(C Rect &rect)C
{
   return rect.valid() && MaxDist(rect, plane)>min_dist;
}
static Vec2 Delta(C Vec2 &delta, C GuiObjNearest &gon, C Rect &test_rect)
{
 C Rect &start_rect=gon.rect;
 C Vec2 &move      =gon.plane.normal;
   // handle special cases for rectangles in line (this is for things like elements placed on a grid, and Property where fields are stored as vertical list, but some are narrow (checkbox) and some wide (combobox))
   if(move.x && CoverY(start_rect, test_rect))return Vec2(DeltaX(start_rect, test_rect), 0); // if moving horizontally and rectangles intersect in Y coordinates then return shortest delta between the rectangles
   if(move.y && CoverX(start_rect, test_rect))return Vec2(0, DeltaY(start_rect, test_rect)); // if moving   vertically and rectangles intersect in X coordinates then return shortest delta between the rectangles
   return delta;
}
Bool GuiObjNearest::Obj::recalcDo(GuiObjNearest &gon)
{
   Vec2 rect_center=rect.center(), delta=rect_center-gon.plane.pos;
   Flt  dist_plane=Dot(delta, gon.plane.normal);
   if(  dist_plane>gon.min_dist)
   {
      Vec2 rect_delta     =Delta(delta, gon, rect);
      Flt  rect_dist_plane=Dot(rect_delta, gon.plane.normal),
           rect_dist2     =    rect_delta.length2();
      if(  rect_dist_plane>0 || rect_dist2==0)
      {
         recalc   =false;
         dist     =DistDot(delta.length2(), dist_plane     );
         dist_rect=DistDot(rect_dist2     , rect_dist_plane);
         return true;
      }
   }
   return false;
}
void GuiObjNearest::add(C Rect &rect, Flt area, GuiObj &obj)
{
   Flt area_min=area*NEAREST_AREA_MIN;
   if(rect.area()>=area_min)
   {
      Vec2 rect_center=rect.center(), delta=rect_center-T.plane.pos;
      Flt  dist_plane=Dot(delta, T.plane.normal);
      if(  dist_plane>T.min_dist)
      {
         Vec2 rect_delta     =Delta(delta, T, rect);
         Flt  rect_dist_plane=Dot(rect_delta, T.plane.normal),
              rect_dist2     =    rect_delta.length2();
         if(  rect_dist_plane>0 || rect_dist2==0)
         {
            auto &nearest=T.nearest.New();
            nearest.recalc   =false;
            nearest.area_min =area_min;
            nearest.dist     =DistDot(delta.length2(), dist_plane     );
            nearest.dist_rect=DistDot(rect_dist2     , rect_dist_plane);
            nearest.rect     =rect;
            nearest.obj      =&obj;
         }
      }
   }
}
GuiObjNearest::Obj* GuiObjNearest::findNearest()
{
   if(nearest.elms())
   {
      auto *nearest=&T.nearest.last();
      Flt   nearest_dist_rect_min=nearest->dist_rect   -EPS*0.5f,
            nearest_dist_rect_max=nearest_dist_rect_min+EPS;
      REP(T.nearest.elms()-1)
      {
         auto &obj=T.nearest[i];
         Flt   obj_dist_rect=obj.dist_rect;
         if(obj_dist_rect< nearest_dist_rect_min
         || obj_dist_rect<=nearest_dist_rect_max && obj.dist<nearest->dist)
         {
            nearest=&obj;
            nearest_dist_rect_min=    obj_dist_rect    -EPS*0.5f;
            nearest_dist_rect_max=nearest_dist_rect_min+EPS;
         }
      }
      return nearest;
   }
   return null;
}
void GuiObj::nearest(C GuiPC &gpc, GuiObjNearest &gon)
{
   if(/*gpc.visible &&*/ visible())
   {
      if(gon.obj==this)gon.state=1;else // if encountered the starting object, then mark as encountered, and don't process it
      {
         Rect rect=T.rect()+gpc.offset; rect&=gpc.clip;
         gon.add(rect, T.rect().area(), T);
      }
   }
}
/******************************************************************************/
// IO
/******************************************************************************/
Bool GuiObj::save(File &f, CChar *path)C
{
   f.putMulti(Byte(4), _visible, _disabled, _rect)<<_desc; // version
   return f.ok();
}
Bool GuiObj::load(File &f, CChar *path)
{
   switch(f.decUIntV()) // version
   {
      case 4:
      {
         f.getMulti(_visible, _disabled, _rect)>>_desc;
         if(f.ok())return true;
      }break;

      case 3:
      {
         f.getMulti(_visible, _disabled, _rect)._getStr1(_desc);
         if(f.ok())return true;
      }break;

      case 2:
      {
        _visible=!f.getBool(); f>>_disabled>>_rect; f._getStr(_desc);
         if(f.ok())return true;
      }break;

      case 1:
      {
        _visible=!f.getBool(); f>>_disabled>>_rect; _desc=f._getStr16();
         if(f.ok())return true;
      }break;

      case 0:
      {
        _visible=!f.getBool(); f>>_disabled>>_rect; _desc.clear();
         if(f.ok())return true;
      }break;
   }
   return false;
}
/******************************************************************************/
// GUI OBJ CHILDREN
/******************************************************************************/
Bool GuiObjChildren::find(C GuiObj &child, Int &index)C
{
   REPA(children)if(T[i]==&child){index=i; return true;}
   return false;
}
Bool GuiObjChildren::find(C GuiObj &child_a, C GuiObj &child_b, Int &index_a, Int &index_b)C
{
   index_a=index_b=-1;
   REPA(children)
   {
    C GuiObj *go=T[i];
      if(go==&child_a){index_a=i; if(index_b>=0)return true;}
      if(go==&child_b){index_b=i; if(index_a>=0)return true;}
   }
   return false;
}

void GuiObjChildren::del()
{
#if 1 // disconnect children (this should be used instead of deleting children, as we may still use them)
   REPAO(children).go->_parent=null;
#else // delete children
   for(GuiObj *last=null; children.elms(); )
   {
      GuiObj *go=children.last().go; // can't do 'pop' because calling 'del' assumes that object is still in this container, and 'GuiObjChildren.remove' can find it
      if(go!=last) // deleting object for the first time
      {
         go->del();
         last=go;
      }else // if we already tried to delete this object then it means that 'del' method didn't succeed (for example it was overrided and super wasn't called)
      {
      #if DEBUG // crash in debug
         Str s=S+"Removing Gui Object ("+go->typeName()+") from Parent ";
         if(go->parent())s+=S+"("+go->parent()->typeName()+") ";
         s+="failed.\nDid you override 'GuiObj.del' method and didn't call 'super.del' ?";
         Exit(s);
      #else // silently disconnect in release
         go->_parent=null;
         children.removeLast();
      #endif
      }
   }
#endif
   children.del(); kb=null; changed=true;
}
Bool GuiObjChildren::remove(GuiObj &child)
{
   Int i; if(find(child, i))
   {
      children.remove(i, true); if(child.contains(kb))kb=null; child._parent=null; changed=true; // don't deactivate 'child' because we may be just changing parents for it
      return true;
   }
   return false;
}
GuiObjChildren::Child* GuiObjChildren::add(GuiObj &child, GuiObj &parent)
{
   GuiObj *old_parent=child.parent();
   if(old_parent!=&parent)
   {
      if(old_parent)*old_parent-=child;

      Int l=0, r=children.elms(); for(; l<r; )
      {
         Int m=UInt(l+r)/2;
         if(CompareLevel(child, *children[m].go)<0)r=m;else l=m+1;
      }

      Child &c=children.NewAt(l); c.go=&child; child._parent=&parent; child.parentClientRectChanged(old_parent ? &NoTemp(old_parent->localClientRect()) : null, &NoTemp(parent.localClientRect()));
      if(!kb && child.kbCatch())kb=&child;
      changed=true;
      return &c;
   }
   Int index; if(find(child, index))return &children[index]; // if that's the same parent then return its existing reference
   return null;
}
void GuiObjChildren::validateLevel(C GuiObj &child)
{
   Int i; if(find(child, i))
   {
      for(; InRange(i-1, children) && CompareLevel(*children[i].go, *children[i-1].go)<0; i--){children.swapOrder(i, i-1); changed=true;}
      for(; InRange(i+1, children) && CompareLevel(*children[i].go, *children[i+1].go)>0; i++){children.swapOrder(i, i+1); changed=true;}
   }
}
Int GuiObjChildren::compareLevel(C GuiObj &child_a, C GuiObj &child_b)C
{
   Int ia, ib; if(find(child_a, child_b, ia, ib))return ia-ib; // if both were found
   return 0;
}
Bool GuiObjChildren::partiallyOccludedInSameLevel(C GuiObj &child)
{
 C Rect &rect=child.rect();
   REPA(children) // go from the end to process top objects first
   {
    C GuiObj &test=*children[i].go; if(&test==&child)return false; // found self
      if(test.visible() && Cuts(test.rect(), rect) && CompareLevel(child, test)>=0)return true; // if visible and partially occludes and level of 'test' is not higher (but same or lower), we test the level because we use this function for focusing windows (to know if activating it, and thus moving it to front, would make any difference)
   }
   return false;
}

Bool GuiObjChildren::moveUp(C GuiObj &child)
{
   Int i; if(find(child, i))if(InRange(i+1, children) && CompareLevel(child, *children[i+1].go)>=0){children.swapOrder(i, i+1); changed=true; return true;}
   return false;
}
Bool GuiObjChildren::moveDown(C GuiObj &child)
{
   Int i; if(find(child, i))if(InRange(i-1, children) && CompareLevel(child, *children[i-1].go)<=0){children.swapOrder(i, i-1); changed=true; return true;}
   return false;
}
void GuiObjChildren::moveToTop(C GuiObj &child)
{
   Int i; if(find(child, i))for(; InRange(i+1, children) && CompareLevel(child, *children[i+1].go)>=0; i++){children.swapOrder(i, i+1); changed=true;}
}
void GuiObjChildren::moveToBottom(C GuiObj &child)
{
   Int i; if(find(child, i))for(; InRange(i-1, children) && CompareLevel(child, *children[i-1].go)<=0; i--){children.swapOrder(i, i-1); changed=true;}
}
void GuiObjChildren::moveAbove(C GuiObj &child_a, C GuiObj &child_b)
{
   Int ia, ib; if(find(child_a, child_b, ia, ib))for(; ia<ib && InRange(ia+1, children) && CompareLevel(child_a, *children[ia+1].go)>=0; ia++){children.swapOrder(ia, ia+1); changed=true;}
}
void GuiObjChildren::moveBelow(C GuiObj &child_a, C GuiObj &child_b)
{
   Int ia, ib; if(find(child_a, child_b, ia, ib))for(; ia>ib && InRange(ia-1, children) && CompareLevel(child_a, *children[ia-1].go)<=0; ia--){children.swapOrder(ia, ia-1); changed=true;}
}

static Bool ValidForSwitch(GuiObj *go)
{
   if(go && go->kbCatch())switch(go->type())
   {
      case GO_WINDOW  :
      case GO_VIEWPORT:
      case GO_TABS    : break;

      case GO_BUTTON: if(GuiSkin *skin=go->asButton().getSkin())return skin->keyboard_highlight_color.a!=0; break; // only if there's visual indication of keyboard highlight

      default: return true;
   }
   return false;
}
Bool GuiObjChildren::Switch(C GuiObj &go, Bool next)
{
   Int i; if(find(go, i))
   {
      Int dir=SignBool(next);
      for(Int j=i+dir+children.elms(); (j%children.elms())!=i; j+=dir) // add 'children.elms()' at start to solve negative modulo issue
      {
         GuiObj *go=T[j%children.elms()]; if(ValidForSwitch(go))
         {
            go->activate();
            if(go->isTextLine() && go->enabled())go->asTextLine().selectAll();
            return true;
         }
      }
   }
   return false;
}
GuiObj* GuiObjChildren::test   (C GuiPC &gpc, C Vec2 &pos, GuiObj* &mouse_wheel) { REPA(children)if(GuiObj *go=T[i])if(go=go->test   (gpc, pos, mouse_wheel))return go; return null;} // order is important, go from the end (objects on top) and stop on first found
void    GuiObjChildren::nearest(C GuiPC &gpc, GuiObjNearest &gon               ) {FREPA(children)if(GuiObj *go=T[i])      go->nearest(gpc, gon             )          ;             } // order is important
void    GuiObjChildren::draw   (C GuiPC &gpc                                   ) {FREPA(children)if(GuiObj *go=T[i])      go->draw   (gpc                  )          ;             } // order is important
void    GuiObjChildren::update (C GuiPC &gpc                                   )
{
   // clear all children at start
   FREPA(children)if(GuiObj *go=T[i])go->_updated=false;

again:
   changed=false; // set no change at start
   FREPA(children)if(GuiObj *go=T[i])if(!go->_updated) // if this object wasn't updated yet
   {
      go->_updated=true; // set as updated
      go-> update(gpc);  // update
      if(changed)goto again; // if detected any change in children container, then start again
   }
}
/******************************************************************************/
CChar* GuiObjTypeName(GUI_OBJ_TYPE type)
{
   switch(type)
   {
      default         : return null;
      case GO_BUTTON  : return u"Button";
      case GO_CHECKBOX: return u"CheckBox";
      case GO_COMBOBOX: return u"ComboBox";
      case GO_CUSTOM  : return u"Custom";
      case GO_DESKTOP : return u"Desktop";
      case GO_IMAGE   : return u"Image";
      case GO_LIST    : return u"List";
      case GO_MENU    : return u"Menu";
      case GO_MENU_BAR: return u"MenuBar";
      case GO_PROGRESS: return u"ProgressBar";
      case GO_REGION  : return u"Region";
      case GO_SLIDEBAR: return u"SlideBar";
      case GO_SLIDER  : return u"Slider";
      case GO_TAB     : return u"Tab";
      case GO_TABS    : return u"Tabs";
      case GO_TEXT    : return u"Text";
      case GO_TEXTBOX : return u"TextBox";
      case GO_TEXTLINE: return u"TextLine";
      case GO_VIEWPORT: return u"Viewport";
      case GO_WINDOW  : return u"Window";
   }
}
/******************************************************************************/
}
/******************************************************************************/
