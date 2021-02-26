#import <AppKit/AppKit.h>
#include <set>

using namespace std;


#include "UserSettings.h"
#include "string_util.h"

namespace FileSelector
{

void RequestMidiFilename(std::wstring *returned_filename, std::wstring *returned_file_title)
{
   // Grab the filename of the last song we played
   // and pre-load it into the open dialog
   wstring last_filename = UserSetting::Get(L"Last File", L"");

   const static int BufferSize = 512;
   wchar_t filename[BufferSize] = L"";
   wchar_t filetitle[BufferSize] = L"";
   

    NSOpenPanel* panel = [NSOpenPanel openPanel];
    
      [panel setMessage:@"Choose a MIDI song to play"];
      [panel setAllowedFileTypes: @[@"mid", @"midi"]];

    int i = [panel runModal];
    if(i == NSOKButton){
        NSURL* theDoc = [[panel URLs] objectAtIndex:0];
        if (returned_filename) *returned_filename = WideFromMacString((CFStringRef)[theDoc path]);
        if (returned_file_title) *returned_file_title = WideFromMacString((CFStringRef)[[theDoc path] lastPathComponent]);
    } else {

         if (returned_file_title) *returned_file_title = L"";
         if (returned_filename) *returned_filename = L"";

     }
    
}

}; // End namespace
