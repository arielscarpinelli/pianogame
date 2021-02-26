#include "CompatibleSystem.h"
#include "string_util.h"
#include "version.h"
#include "os.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

namespace Compatible
{

   void ShowError(const std::wstring &err)
   {
      // The cursor might have been hidden.
      ShowMouseCursor();

       NSAlert *alert = [[NSAlert alloc] init];
       [alert addButtonWithTitle:@"OK"];
       [alert setMessageText: (NSString*)MacStringFromWide(err).get()];
       [alert setAlertStyle:NSWarningAlertStyle];

       NSLog(@"%@", alert.messageText);
       //[alert runModal];
   }

    void GracefulShutdown()
    {
       [NSApp terminate: nil];
    }

}


