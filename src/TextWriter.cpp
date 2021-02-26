
// Copyright (c)2007 Nicholas Piegdon
// See license.txt for license information

#include <map>

#include "TextWriter.h"
#include "Renderer.h"
#include "PianoGameError.h"
#include "os_graphics.h"

#ifdef WIN32
// TODO: This should be deleted at shutdown
static std::map<int, HFONT> font_handle_lookup;
static int next_call_list_start = 1;
#else
// TODO: This should be deleted at shutdown

GLuint Texture2DCreateFromString(const GLchar * const pString,
                                const GLchar * const pFontName,
                                const CGFloat& rFontSize,
                                const CTTextAlignment& rAlignment,
                                const CGFloat * const pColor,
                                 CGSize& rSize);

#endif

// TODO: This should be deleted at shutdown
static std::map<int, int> font_size_lookup;

TextWriter::TextWriter(int in_x, int in_y, Renderer &in_renderer, bool in_centered, int in_size, std::wstring fontname) :
x(in_x), y(in_y), size(in_size), original_x(0), last_line_height(0), centered(in_centered), renderer(in_renderer)
{
   x += renderer.m_xoffset;
   original_x = x;

   y += renderer.m_yoffset;

#ifdef WIN32

   Context c = renderer.m_context;
   point_size = MulDiv(size, GetDeviceCaps(c, LOGPIXELSY), 72);

   HFONT font = 0;
   if (font_size_lookup[in_size] == 0)
   {
      // Set up the LOGFONT structure
      LOGFONT logical_font;
      logical_font.lfHeight = get_point_size();
      logical_font.lfWidth = 0;
      logical_font.lfEscapement = 0;
      logical_font.lfOrientation = 0;
      logical_font.lfWeight = FW_NORMAL;
      logical_font.lfItalic = false;
      logical_font.lfUnderline = false;
      logical_font.lfStrikeOut = false;
      logical_font.lfCharSet = ANSI_CHARSET;
      logical_font.lfOutPrecision = OUT_DEFAULT_PRECIS;
      logical_font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
      logical_font.lfQuality = PROOF_QUALITY;
      logical_font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
      lstrcpy(logical_font.lfFaceName, fontname.c_str()); 

      font = CreateFontIndirect(&logical_font);

      HFONT previous_font = (HFONT)SelectObject(c, font);

      wglUseFontBitmaps(c, 0, 128, next_call_list_start);
      font_size_lookup[in_size] = next_call_list_start;
      font_handle_lookup[in_size] = font;
      next_call_list_start += 130;

      SelectObject(c, previous_font);
   }

#else

   // TODO: is this sufficient?
   point_size = size;

#endif

}

TextWriter::~TextWriter()
{
}

int TextWriter::get_point_size() 
{
   return point_size;
}

TextWriter& TextWriter::next_line()
{
   y += std::max(last_line_height, get_point_size());
   x = original_x;

   last_line_height = 0;
   return *this;
}

TextWriter& Text::operator<<(TextWriter& tw) const
{
   int draw_x;
   int draw_y;
    
    // TODO: This isn't Unicode!
    std::string narrow(m_text.begin(), m_text.end());

#ifndef WIN32

   CGFloat color[4] = {m_color.r / 255.0f, m_color.g / 255.0f, m_color.b / 255.0f, m_color.a / 255.0f};
   CGSize size;

   GLuint texture = Texture2DCreateFromString(narrow.c_str(), nullptr, tw.size, tw.centered ? kCTTextAlignmentCenter : kCTTextAlignmentLeft, color, size);
    
    draw_x = size.width;
    draw_y = size.height;

#endif
   calculate_position_and_advance_cursor(tw, &draw_x, &draw_y);

   
   glPushMatrix();
#ifdef WIN32
    glBindTexture(GL_TEXTURE_2D, 0);
    tw.renderer.SetColor(m_color);
    glListBase(font_size_lookup[tw.size]);
    glRasterPos2i(draw_x, draw_y + tw.size);
    glCallLists(static_cast<int>(narrow.length()), GL_UNSIGNED_BYTE, narrow.c_str());
#else
    tw.renderer.DrawTextTextureQuad(texture, draw_x, draw_y, size.width, size.height);

    //glDeleteTextures(1, &texture);
#endif
   glPopMatrix();

   // TODO: Should probably delete these on shutdown.
   //glDeleteLists(1000, 128);

   return tw;
}

void Text::calculate_position_and_advance_cursor(TextWriter &tw, int *out_x, int *out_y) const
{
#ifdef WIN32

   const long options = DT_LEFT | DT_NOPREFIX;

   Context c = tw.renderer.m_context;
   int previous_map_mode = SetMapMode(c, MM_TEXT);

   HFONT font = font_handle_lookup[tw.size];

   // Create the font we want to use, and swap it out with
   // whatever is currently in there, along with our color
   HFONT previous_font = (HFONT)SelectObject(c, font);

   // Call DrawText to find out how large our text is
   RECT drawing_rect = { tw.x, tw.y, 0, 0 };
   tw.last_line_height = DrawText(c, m_text.c_str(), int(m_text.length()), &drawing_rect, options | DT_CALCRECT);

   // Return the hdc settings to their previous setting
   SelectObject(c, previous_font);
   SetMapMode(c, previous_map_mode);
#else

    Rect drawing_rect = { tw.y, tw.x, tw.y + *out_y, tw.x + *out_x};

#endif

    // Update the text-writer with post-draw coordinates
    if (tw.centered) drawing_rect.left -= (drawing_rect.right - drawing_rect.left) / 2;
    if (!tw.centered) tw.x += drawing_rect.right - drawing_rect.left;

    // Tell the draw function where to put the text
    *out_x = drawing_rect.left;
    *out_y = drawing_rect.top;


}

TextWriter& operator<<(TextWriter& tw, const Text& t)
{
   return t.operator <<(tw);
}

TextWriter& newline(TextWriter& tw)
{
   return tw.next_line();
}

TextWriter& operator<<(TextWriter& tw, const std::wstring& s)  { return tw << Text(s, White); }
TextWriter& operator<<(TextWriter& tw, const int& i)           { return tw << Text(i, White); }
TextWriter& operator<<(TextWriter& tw, const unsigned int& i)  { return tw << Text(i, White); }
TextWriter& operator<<(TextWriter& tw, const long& l)          { return tw << Text(l, White); }
TextWriter& operator<<(TextWriter& tw, const unsigned long& l) { return tw << Text(l, White); }

// Create a bitmap context from a string, font, justification, and font size
static CGContextRef CGContextCreateFromAttributedString(CFAttributedStringRef pAttrString,
                                                        const CFRange& rRange,
                                                        CGColorSpaceRef pColorspace,
                                                        CGSize& rSize)
{
    CGContextRef pContext = nullptr;
    
    if(pAttrString != nullptr)
    {
        // Acquire a frame setter
        CTFramesetterRef pFrameSetter = CTFramesetterCreateWithAttributedString(pAttrString);
        
        if(pFrameSetter != nullptr)
        {
            // Create a path for layout
            CGMutablePathRef pPath = CGPathCreateMutable();
            
            if(pPath != nullptr)
            {
                CFRange range;
                CGSize  constraint = CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX);
                
                // Get the CoreText suggested size from our framesetter
                rSize = CTFramesetterSuggestFrameSizeWithConstraints(pFrameSetter,
                                                                     rRange,
                                                                     nullptr,
                                                                     constraint,
                                                                     &range);
                
                // Set path bounds
                CGRect bounds = CGRectMake(0.0f,
                                           0.0f,
                                           rSize.width,
                                           rSize.height);
                
                // Bound the path
                CGPathAddRect(pPath, nullptr, bounds);
                
                // Layout the attributed string in a frame
                CTFrameRef pFrame = CTFramesetterCreateFrame(pFrameSetter, range, pPath, nullptr);
                
                if(pFrame != nullptr)
                {
                    // Compute bounds for the bitmap context
                    size_t width  = size_t(rSize.width);
                    size_t height = size_t(rSize.height);
                    size_t stride = sizeof(GLuint) * width;
                    
                    // No explicit backing-store allocation here.  We'll let the
                    // context allocate the storage for us.
                    pContext = CGBitmapContextCreate(nullptr,
                                                     width,
                                                     height,
                                                     8,
                                                     stride,
                                                     pColorspace,
                                                     kCGImageAlphaPremultipliedLast);
                    
                    if(pContext != nullptr)
                    {
                        // Use this for vertical reflection
                        CGContextTranslateCTM(pContext, 0.0, height);
                        CGContextScaleCTM(pContext, 1.0, -1.0);
                        
                        // Draw the frame into a bitmap context
                        CTFrameDraw(pFrame, pContext);
                        
                        // Flush the context
                        CGContextFlush(pContext);
                    } // if
                    
                    // Release the frame
                    CFRelease(pFrame);
                } // if
                
                CFRelease(pPath);
            } // if
            
            CFRelease(pFrameSetter);
        } // if
    } // if
    
    return pContext;
} // CGContextCreateFromString
    

// Create an attributed string from a CF string, font, justification, and font size
static CFMutableAttributedStringRef CFMutableAttributedStringCreate(CFStringRef pString,
                                                                    CFStringRef pFontNameSrc,
                                                                    CGColorRef pComponents,
                                                                    const CGFloat& rFontSize,
                                                                    const CTTextAlignment nAlignment,
                                                                    CFRange *pRange)
{
    CFMutableAttributedStringRef pAttrString = nullptr;
    
    if(pString != nullptr)
    {
        // Paragraph style setting structure
        const GLuint nCntStyle = 2;
        
        // For single spacing between the lines
        const CGFloat nLineHeightMultiple = 1.0f;
        
        // Paragraph settings with alignment and style
        CTParagraphStyleSetting settings[nCntStyle] =
        {
            {
                kCTParagraphStyleSpecifierAlignment,
                sizeof(CTTextAlignment),
                &nAlignment
            },
            {
                kCTParagraphStyleSpecifierLineHeightMultiple,
                sizeof(CGFloat),
                &nLineHeightMultiple
            }
        };
        
        // Create a paragraph style
        CTParagraphStyleRef pStyle = CTParagraphStyleCreate(settings, nCntStyle);
        
        if(pStyle != nullptr)
        {
            // If the font name is nullptr default to Helvetica
            CFStringRef pFontNameDst = (pFontNameSrc) ? pFontNameSrc : CFSTR("Helvetica");
            
            // Prepare font
            CTFontRef pFont = CTFontCreateWithName(pFontNameDst, rFontSize, nullptr);
            
            if(pFont != nullptr)
            {
                // Set attributed string properties
                const GLuint nCntDict = 3;
                
                CFStringRef keys[nCntDict] =
                {
                    kCTParagraphStyleAttributeName,
                    kCTFontAttributeName,
                    kCTForegroundColorAttributeName
                };
                
                CFTypeRef values[nCntDict] =
                {
                    pStyle,
                    pFont,
                    pComponents
                };
                
                // Create a dictionary of attributes for our string
                CFDictionaryRef pAttributes = CFDictionaryCreate(nullptr,
                                                                 (const void **)&keys,
                                                                 (const void **)&values,
                                                                 nCntDict,
                                                                 &kCFTypeDictionaryKeyCallBacks,
                                                                 &kCFTypeDictionaryValueCallBacks);
                
                if(pAttributes != nullptr)
                {
                    // Creating a mutable attributed string
                    pAttrString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
                    
                    if(pAttrString != nullptr)
                    {
                        // Set a mutable attributed string with the input string
                        CFAttributedStringReplaceString(pAttrString, CFRangeMake(0, 0), pString);
                        
                        // Compute the mutable attributed string range
                        *pRange = CFRangeMake(0, CFAttributedStringGetLength(pAttrString));
                        
                        // Set the attributes
                        CFAttributedStringSetAttributes(pAttrString, *pRange, pAttributes, 0);
                    } // if
                    
                    CFRelease(pAttributes);
                } // if
                
                CFRelease(pFont);
            } // if
            
            CFRelease(pStyle);
        } // if
    } // if
    
    return pAttrString;
} // CFMutableAttributedStringCreate

// Create a 2D texture
static GLuint GLUTexture2DCreate(const GLuint& rWidth,
                                 const GLuint& rHeight,
                                 const GLvoid * const pPixels)
{
    GLuint nTID = 0;
    
    // Greate a texture
    glGenTextures(1, &nTID);
    
    if(nTID)
    {
        // Bind a texture with ID
        glBindTexture(GL_TEXTURE_2D, nTID);
        
        // Set texture properties (including linear mipmap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Initialize the texture
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     rWidth,
                     rHeight,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_INT_8_8_8_8_REV,
                     pPixels);
        
        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Discard
        glBindTexture(GL_TEXTURE_2D, 0);
    } // if
    
    return nTID;
} // GLUTexture2DCreate

// Create a texture from a bitmap context
static GLuint GLUTexture2DCreateFromContext(CGContextRef pContext)
{
    GLuint nTID = 0;
    
    if(pContext != nullptr)
    {
        GLuint nWidth  = GLuint(CGBitmapContextGetWidth(pContext));
        GLuint nHeight = GLuint(CGBitmapContextGetHeight(pContext));
        
        const GLvoid *pPixels = CGBitmapContextGetData(pContext);
        
        nTID = GLUTexture2DCreate(nWidth, nHeight, pPixels);
        
        // Was there a GL error?
        GLenum nErr = glGetError();
        
        if(nErr != GL_NO_ERROR)
        {
            glDeleteTextures(1, &nTID);
            
            throw PianoGameError(L"OpenGL error trying to create texture");
        } // if
    } // if
    
    return nTID;
} // GLUTexture2DCreateFromContext

// Create a bitmap context from a core foundation string, font,
// justification, and font size
static CGContextRef CGContextCreateFromString(CFStringRef pString,
                                              CFStringRef pFontName,
                                              const CGFloat& rFontSize,
                                              const CTTextAlignment& rAlignment,
                                              const CGFloat * const pComponents,
                                              CGSize &rSize)
{
    CGContextRef pContext = nullptr;
    
    if(pString != nullptr)
    {
        // Get a generic linear RGB color space
        CGColorSpaceRef pColorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear);
        
        if(pColorspace != nullptr)
        {
            // Create a white color reference
            CGColorRef pColor = CGColorCreate(pColorspace, pComponents);
            
            if(pColor != nullptr)
            {
                // Creating a mutable attributed string
                CFRange range;
                
                CFMutableAttributedStringRef pAttrString = CFMutableAttributedStringCreate(pString,
                                                                                           pFontName,
                                                                                           pColor,
                                                                                           rFontSize,
                                                                                           rAlignment,
                                                                                           &range);
                
                if(pAttrString != nullptr)
                {
                    // Create a context from our attributed string
                    pContext = CGContextCreateFromAttributedString(pAttrString,
                                                                   range,
                                                                   pColorspace,
                                                                   rSize);
                    
                    CFRelease(pAttrString);
                } // if
                
                CFRelease(pColor);
            } // if
            
            CFRelease(pColorspace);
        } // if
    } // if
    
    return pContext;
} // CGContextCreateFromString

// Create a bitmap context from a c-string, font, justification, and font size
static CGContextRef CGContextCreateFromString(const GLchar * const pString,
                                              const GLchar * const pFontName,
                                              const CGFloat& rFontSize,
                                              const CTTextAlignment& rAlignment,
                                              const CGFloat * const pComponents,
                                              CGSize& rSize)
{
    CGContextRef pContext = nullptr;
    
    if(pString != nullptr)
    {
        CFStringRef pCFString = CFStringCreateWithCString(kCFAllocatorDefault,
                                                          pString,
                                                          kCFStringEncodingASCII);
        
        if(pCFString != nullptr)
        {
            const GLchar *pFontString = (pFontName) ? pFontName : "Helvetica";
            
            CFStringRef pFontCFString = CFStringCreateWithCString(kCFAllocatorDefault,
                                                                  pFontString,
                                                                  kCFStringEncodingASCII);
            
            if(pFontCFString != nullptr)
            {
                pContext = CGContextCreateFromString(pCFString,
                                                     pFontCFString,
                                                     rFontSize,
                                                     rAlignment,
                                                     pComponents,
                                                     rSize);
                
                CFRelease(pFontCFString);
            } // if
            
            CFRelease(pCFString);
        } // if
    } // if
    
    return pContext;
} // CGContextCreateFromString
    
// Generate a texture from a cstring, using a font, at a size,
// with alignment and color
GLuint Texture2DCreateFromString(const GLchar * const pString,
                                      const GLchar * const pFontName,
                                      const CGFloat& rFontSize,
                                      const CTTextAlignment& rAlignment,
                                      const CGFloat * const pColor,
                                      CGSize& rSize)
{
    GLuint nTID = 0;
    
    CGContextRef pCtx = CGContextCreateFromString(pString,
                                                  pFontName,
                                                  rFontSize,
                                                  rAlignment,
                                                  pColor,
                                                  rSize);
    
    if(pCtx != nullptr)
    {
        nTID = GLUTexture2DCreateFromContext(pCtx);
        
        CGContextRelease(pCtx);
    } // if
    
    return nTID;
} // GLUTexture2DCreateFromString





