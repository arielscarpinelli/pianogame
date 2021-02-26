
// Copyright (c)2007 Nicholas Piegdon
// See license.txt for license information

#include "CompatibleSystem.h"
#include "string_util.h"
#include "version.h"
#include "os.h"

#ifndef WIN32
    #include <sys/time.h>
#endif

namespace Compatible
{
   unsigned long GetMilliseconds()
   {
#ifdef WIN32
      return timeGetTime();
#else
      timeval tv;
      gettimeofday(&tv, 0);
      return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif
   }


#ifdef WIN32
   void ShowError(const std::wstring &err)
   {
      const static std::wstring friendly_app_name = WSTRING(L"Piano Game " << PianoGameVersionString);
      const static std::wstring message_box_title = WSTRING(friendly_app_name << L" Error");
      
      MessageBox(0, err.c_str(), message_box_title.c_str(), MB_ICONERROR);
   }
#endif

   void HideMouseCursor()
   {
#ifdef WIN32
      ShowCursor(false);
#else
      CGDisplayHideCursor(kCGDirectMainDisplay);
#endif
   }
   
   void ShowMouseCursor()
   {
#ifdef WIN32
      ShowCursor(true);
#else
      CGDisplayShowCursor(kCGDirectMainDisplay);
#endif
   }


   int GetDisplayWidth()
   {
#ifdef WIN32
      return GetSystemMetrics(SM_CXSCREEN);
#else
      return int(CGDisplayBounds(kCGDirectMainDisplay).size.width);
#endif
   }

   int GetDisplayHeight()
   {
#ifdef WIN32
      return GetSystemMetrics(SM_CYSCREEN);
#else
      return int(CGDisplayBounds(kCGDirectMainDisplay).size.height);
#endif
   }


#ifdef WIN32
   void GracefulShutdown()
   {
      PostQuitMessage(0);
   }
#endif

}; // End namespace
