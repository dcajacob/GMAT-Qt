#include <cstdio>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#else
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

std::string quoted(const GLubyte *value)
{
   std::string result="\"";
   if (value) for (const unsigned char character:std::string(reinterpret_cast<const char *>(value))) {
      if (character=='"' || character=='\\') result+='\\';
      if (character>=32) result+=character;
   }
   return result+'"';
}

int main()
{
   bool current=false;
#if defined(_WIN32)
   WNDCLASSA windowClass={};
   windowClass.style=CS_OWNDC;
   windowClass.lpfnWndProc=DefWindowProcA;
   windowClass.hInstance=GetModuleHandleA(nullptr);
   windowClass.lpszClassName="GmatNativeGraphicsProbe";
   RegisterClassA(&windowClass);
   const HWND window=CreateWindowA(windowClass.lpszClassName,"GMAT graphics probe",WS_OVERLAPPEDWINDOW,
      0,0,64,64,nullptr,nullptr,windowClass.hInstance,nullptr);
   const HDC device=window ? GetDC(window) : nullptr;
   PIXELFORMATDESCRIPTOR format={};
   format.nSize=sizeof(format);
   format.nVersion=1;
   format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
   format.iPixelType=PFD_TYPE_RGBA;
   format.cColorBits=24;
   format.cDepthBits=24;
   const int pixelFormat=device ? ChoosePixelFormat(device,&format) : 0;
   HGLRC context=nullptr;
   if (pixelFormat && SetPixelFormat(device,pixelFormat,&format)) {
      context=wglCreateContext(device);
      current=context && wglMakeCurrent(device,context);
   }
#elif defined(__APPLE__)
   const CGLPixelFormatAttribute attributes[]={kCGLPFAColorSize,static_cast<CGLPixelFormatAttribute>(24),
      kCGLPFADepthSize,static_cast<CGLPixelFormatAttribute>(24),static_cast<CGLPixelFormatAttribute>(0)};
   CGLPixelFormatObj format=nullptr;
   GLint count=0;
   CGLContextObj context=nullptr;
   if (CGLChoosePixelFormat(attributes,&format,&count)==kCGLNoError && format) {
      if (CGLCreateContext(format,nullptr,&context)==kCGLNoError && context)
         current=CGLSetCurrentContext(context)==kCGLNoError;
      CGLDestroyPixelFormat(format);
   }
#else
   Display *display=XOpenDisplay(nullptr);
   Window window=0;
   GLXContext context=nullptr;
   if (display) {
      int attributes[]={GLX_RGBA,GLX_DEPTH_SIZE,24,GLX_DOUBLEBUFFER,None};
      XVisualInfo *visual=glXChooseVisual(display,DefaultScreen(display),attributes);
      if (visual) {
         const Colormap colormap=XCreateColormap(display,RootWindow(display,visual->screen),visual->visual,AllocNone);
         XSetWindowAttributes options={};
         options.colormap=colormap;
         window=XCreateWindow(display,RootWindow(display,visual->screen),0,0,64,64,0,visual->depth,
            InputOutput,visual->visual,CWColormap,&options);
         context=glXCreateContext(display,visual,nullptr,True);
         current=context && glXMakeCurrent(display,window,context);
         XFree(visual);
      }
   }
#endif
   if (current && glGetString(GL_VERSION)) {
      std::printf("{\"available\":true,\"vendor\":%s,\"renderer\":%s,\"version\":%s}\n",
         quoted(glGetString(GL_VENDOR)).c_str(),quoted(glGetString(GL_RENDERER)).c_str(),
         quoted(glGetString(GL_VERSION)).c_str());
   } else {
      std::puts("{\"available\":false}");
   }
#if defined(_WIN32)
   wglMakeCurrent(nullptr,nullptr);
   if (context) wglDeleteContext(context);
   if (device) ReleaseDC(window,device);
   if (window) DestroyWindow(window);
#elif defined(__APPLE__)
   CGLSetCurrentContext(nullptr);
   if (context) CGLDestroyContext(context);
#else
   if (display) {
      glXMakeCurrent(display,None,nullptr);
      if (context) glXDestroyContext(display,context);
      if (window) XDestroyWindow(display,window);
      XCloseDisplay(display);
   }
#endif
   return current ? 0 : 2;
}
