#include "sdl.h"
#include "gl.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (__interrupt __far* KeyboardHandler)();
static KeyboardHandler oldKeyboardHandler = NULL;

bool keyPressed[0x80] = {0};

bool isKeyPressed(int scanCode) {
  return keyPressed[scanCode & 0x7f];
}

static void (__interrupt __far keyboardHandler) (void) {
  unsigned char key = inp(0x60);
  keyPressed[key & 0x7f] = key & 0x80 ? false : true;
  oldKeyboardHandler();
}

void uninstallKeyboardHandler() {
  if (oldKeyboardHandler != NULL) {
    union REGS r;
    r.w.ax = 0x205;
    r.h.bl = 0x09;
    r.x.ecx = FP_SEG(oldKeyboardHandler);
    r.x.edx = FP_OFF(oldKeyboardHandler);
    int386 (0x31, &r, &r);
    oldKeyboardHandler = NULL;
  }
}

void installKeyboardHandler() {
  if (oldKeyboardHandler != NULL)
    uninstallKeyboardHandler();
  union REGS r;
  r.w.ax = 0x204;
  r.h.bl = 0x09;
  int386 (0x31, &r, &r);
  oldKeyboardHandler = (KeyboardHandler)MK_FP(r.x.ecx,r.x.edx);
  r.w.ax = 0x205;
  r.h.bl = 0x09;
  r.x.ecx = FP_SEG(keyboardHandler);
  r.x.edx = FP_OFF(keyboardHandler);
  int386 (0x31, &r, &r);
  atexit(uninstallKeyboardHandler);
}

#define ERROR(__text__) {glDone();printf(__text__);exit(0);}
#define RDELETE(__a__) {if ((__a__)!=NULL) {free(__a__);__a__=NULL;}}
#define RALLOC(__type__,__var__) __type__ *__var__ = (__type__*)malloc(sizeof(__type__));memset(__var__,0,sizeof(__type__));
#define _RALLOC(__type__,__size__) (__type__*)malloc(sizeof(__type__)*(__size__));

unsigned int sdl_initialized = 0;

unsigned short SDL_SwapLE16(unsigned short a) {
  return a;
}

unsigned int SDL_SwapLE32(unsigned int a) {
  return a;
}

int snprintf(char *buffer, const int size, const char *format, ...) {
  return sprintf(buffer,format); // :mad: todo
}

SDL_Result SDL_Init(int flags) {
  sdl_initialized |= flags;
  glSetTime(0);
  installKeyboardHandler();
  glWaitVSync = GL_TRUE;
  return SDL_OK;
}

bool SDL_WasInit(int flags) {
  return (sdl_initialized & flags)==flags ? true : false;
}

SDL_Result SDL_InitSubSystem(int flags) {
  sdl_initialized |= flags;
  return flags == SDL_INIT_AUDIO ? SDL_ERROR : SDL_OK;
}

void SDL_QuitSubSystem(int flags) {
}

int SDL_GetNumVideoDisplays() {
  return 1;
}

int sdlDrawColorR = 0;
int sdlDrawColorG = 0;
int sdlDrawColorB = 0;
int sdlDrawColorA = 0;

void SDL_SetRenderDrawColor(SDL_Renderer *z, int r, int g, int b, int a) {
  sdlDrawColorR = r;
  sdlDrawColorG = g;
  sdlDrawColorB = b;
  sdlDrawColorA = a;
}

void SDL_FillRect(SDL_Surface *surface, SDL_Rect *a, int b) {
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = dx0 + surface->w;
  int dy1 = dy0 + surface->h;
  if (a != NULL) {
    dx0 = a->x;
    dy0 = a->y;
    dx1 = dx0+a->w;
    dy1 = dy0+a->h;
  }
  if (dx0<0) dx0 = 0;
  if (dy0<0) dy0 = 0;
  if (dx1>surface->w) dx1 = surface->w;
  if (dy1>surface->h) dy1 = surface->h;
  if (surface->format->BitsPerPixel==8) {
    for (int y = dy0; y < dy1; y++) {
      for (int x = dx0; x < dx1; x++) {
        surface->pixels[x+y*surface->pitch]=0;
      }
    }
  }
}

SDL_Scancode SDL_GetScancodeFromName(const char *a) {
  return SDL_SCANCODE_NONE;
}

const char *SDL_GetScancodeName(SDL_Scancode a) {
  return "none";
}

void SDL_Delay(double a) {
}

void SDL_Quit() {
  glDone();
  uninstallKeyboardHandler();
  exit(0);
}

void SDL_JoystickEventState(int state) {
}

int SDL_JoystickGetAxis(SDL_Joystick *a, int b) {
  return 0;
}

int SDL_JoystickGetButton(SDL_Joystick *a, int b) {
  return 0;
}

int SDL_JoystickGetHat(SDL_Joystick *a, int b) {
  return 0;
}

void SDL_JoystickUpdate() {
}

int SDL_NumJoysticks() {
  return 0;
}

SDL_Joystick *SDL_JoystickOpen(int j) {
  return NULL;
}

void SDL_JoystickClose(SDL_Joystick *a) {
}

const char *SDL_JoystickName(SDL_Joystick *a) {
  return NULL;
}

int SDL_JoystickNumAxes(SDL_Joystick *a) {
  return 0;
}

int SDL_JoystickNumButtons(SDL_Joystick *a) {
  return 0;
}

int SDL_JoystickNumHats(SDL_Joystick *a) {
  return 0;
}

int SDL_GetTicks() {
  return (int)(glSeconds()*1000.0);
}

SDL_Event events[256];
int currentEvent=0;

bool keyPressed_last[0x80] = {0};

void addKeyboardEvents() {
  int key = glNextKey();
  if (key == GL_VK_END) {
    SDL_Quit();
  }
  if (key != 0) {
    if (key < 128 && key >= 32) {
      SDL_Event *v = &events[currentEvent];
      v->type = SDL_TEXTINPUT;
      v->text.text[0] = key;
      v->text.text[1] = 0;
      currentEvent++;
    }
  }
  for (int i = 0; i < 0x80; i++) {
    if (keyPressed[i] != keyPressed_last[i]) {
      keyPressed_last[i] = keyPressed[i];
      SDL_Event *v = &events[currentEvent];
      v->type = keyPressed[i] ? SDL_KEYDOWN : SDL_KEYUP;
      v->key.state = keyPressed[i] ? 1 : 0;
      v->key.keysym.scancode = i;
      currentEvent++;
    }
  }
}

bool buttonPressed_last[3] = {0};
double sdlMouseX = 0;
double sdlMouseY = 0;

void addMouseEvents() {
  double dx,dy;
  glNextMouseDelta(&dx,&dy);
  sdlMouseX += dx;
  sdlMouseY += dy;
  if (true) {
    if (sdlMouseX<0)sdlMouseX=0;
    if (sdlMouseY<0)sdlMouseY=0;
    if (sdlMouseX>glFrameBufferWidth-1) sdlMouseX=glFrameBufferWidth-1;
    if (sdlMouseY>glFrameBufferHeight-1) sdlMouseY=glFrameBufferHeight-1;
  }
  for (int i = 0; i < 0x03; i++) {
    bool buttonPressed = glMouseButtons() & (1<<i);
    if (buttonPressed != buttonPressed_last[i]) {
      buttonPressed_last[i] = buttonPressed;
      SDL_Event *v = &events[currentEvent];
      v->type = buttonPressed ? SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
      int j = SDL_BUTTON_LEFT;
      switch(i) {
      case 0: j = SDL_BUTTON_LEFT; break;
      case 1: j = SDL_BUTTON_RIGHT; break;
      case 2: j = SDL_BUTTON_MIDDLE; break;
      }
      v->button.button = j;
      v->button.x = sdlMouseX;
      v->button.y = sdlMouseY;
      currentEvent++;
    }
  }
  if (dx!=0||dy!=0) {
    SDL_Event *v = &events[currentEvent];
    v->type = SDL_MOUSEMOTION;
    v->motion.x = sdlMouseX;
    v->motion.y = sdlMouseY;
    v->motion.xrel = dx;
    v->motion.yrel = dy;
    currentEvent++;
  }
}

void addFocusEvent(bool hasNowFocus) {
  SDL_Event *v = &events[currentEvent];
  v->type = SDL_WINDOWEVENT;
  v->window.event = hasNowFocus?SDL_WINDOWEVENT_FOCUS_GAINED:SDL_WINDOWEVENT_FOCUS_LOST;
  currentEvent++;
}

void addResizeEvent() {
  SDL_Event *v = &events[currentEvent];
  v->type = SDL_WINDOWEVENT;
  v->window.event = SDL_WINDOWEVENT_RESIZED;
  currentEvent++;
}

void SDL_PushEvent(const SDL_Event *e) {
}

bool SDL_PollEvent(SDL_Event *e) {
  if (currentEvent <= 0) {
    currentEvent = 0;
    addKeyboardEvents();
    addMouseEvents();
    return false;  
  }
  currentEvent--;
  *e = events[currentEvent];
  return true;
}

int SDL_GetError() {
  return 0;
}

void SDL_ShowCursor(const bool show) {
}

void SDL_strlcpy(char *a, const char *b, size_t c) {
  int i;
  for (i = 0; i < c-1; i++) {
    if (b[i]==0x00) break;
    a[i]=b[i];
  }
  a[i]=0x00;
}

void SDL_SetRelativeMouseMode(bool on) {
}

unsigned int SDL_OpenAudioDevice(void *a, int, const SDL_AudioSpec *b, SDL_AudioSpec *c, bool d) {
  return 0;
}

void SDL_PauseAudioDevice(unsigned int id, int b) {
}

void SDL_CloseAudioDevice(unsigned int id) {
}

void SDL_LockAudioDevice(unsigned int id) {
}

void SDL_UnlockAudioDevice(unsigned int id) {
}

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, unsigned int b, int c, int d, unsigned int e, int f, int g) {
  return 0;
}

SDL_Result SDL_ConvertAudio(SDL_AudioCVT *cvt) {
  return SDL_OK;
}

unsigned int SDL_MapRGB(SDL_PixelFormat *format, int r, int g, int b) {
  switch(format->BitsPerPixel) {
  case 32:return r|(g<<8)|(b<<16)|0xff000000;
  }
  return rand()*0x01020304;
}

unsigned int SDL_GetModState() {
  return 0;
}

unsigned short SDL_Swap16(unsigned short a) {
  return (a>>8)|(a<<8);
}

SDL_Surface *SDL_CreateRGBSurface(int a, int w, int h, int cl, int d, int e, int f, int g) {
  if (cl != 8) ERROR("Error1");
  RALLOC(SDL_Surface,r);
  r->format = _RALLOC(SDL_PixelFormat,1);
  r->format->BitsPerPixel = cl;
  r->pixels = (Uint8*)malloc(w*h); // 8bit
  r->pitch = w;
  r->w = w;
  r->h = h;
  return r;
}

void SDL_FreeSurface(SDL_Surface *a) {
  if (a!=NULL) {
    RDELETE(a->pixels);
    RDELETE(a->format);
  }
  RDELETE(a);
}

SDL_Window *SDL_CreateWindow(const char *windowName,int posX, int posY, int w, int h, int flags) {
  RALLOC(SDL_Window,r);
  r->x = posX;
  r->y = posY;
  r->w = w;
  r->h = h;
  r->flags = flags;
  glDone();
  if (!glVesa(w,h,32)) {
    printf("Vesa: SDL window:%dx%d failed!\n",w,h);
    SDL_Quit();
  }
  addFocusEvent(true);
  return r;
}

void SDL_DestroyWindow(SDL_Window *a) {
  RDELETE(a);
}

void SDL_GetWindowSize(SDL_Window *a, int *w, int *h) {
  if (w != NULL) *w = a->w;
  if (h != NULL) *h = a->h;
}

void SDL_SetWindowPosition(SDL_Window *a, int x, int y) {
  a->x = x;
  a->y = y;
}

void SDL_SetWindowSize(SDL_Window *a, int w, int h) {
  glDone();
  if (!glVesa(w,h,32)) {
    printf("Vesa: SDL window:%dx%d failed!\n",w,h);
    SDL_Quit();
  }
  a->w = w;
  a->h = h;
}

SDL_Result SDL_SetWindowFullscreen(SDL_Window *a, bool b) {
  return SDL_OK;
}

void SDL_GetDisplayBounds(int displayIndex, SDL_Rect *b) {
}

int SDL_GetWindowDisplayIndex(SDL_Window *a) {
  return 0;
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *a, int b, int c) {
  RALLOC(SDL_Renderer,r);
  r->window = a;
  return r;
}

void SDL_DestroyRenderer(SDL_Renderer *a) {
  RDELETE(a);
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *a, unsigned int format, int flags, int w, int h) {
  RALLOC(SDL_Texture,r);
  r->pitch = w * sizeof(unsigned int);
  r->w = w;
  r->h = h;
  r->pixels = _RALLOC(unsigned int,w*h); 
  r->format = format;
  return r;
}

void SDL_DestroyTexture(SDL_Texture *a) {
  if (a != NULL) RDELETE(a->pixels);
  RDELETE(a);
}

void SDL_QueryTexture(SDL_Texture *t, int *a, int *b, int *w, int *h) {
  if (w!=NULL) *w = t->w;
  if (h!=NULL) *h = t->h;
}

void SDL_LockTexture(SDL_Texture *t, void *a, void **data, int *pitch) {
  if (data != NULL) *data = t->pixels;
  if (pitch != NULL) *pitch = t->pitch;
}

void SDL_UnlockTexture(SDL_Texture *t) {
}

SDL_PixelFormat *SDL_AllocFormat(unsigned int a) {
  RALLOC(SDL_PixelFormat,r);
  switch(a) {
  case SDL_PIXELFORMAT_RGB888: {
    r->BitsPerPixel = 32;
  } break;
  }
  return r;
}

void SDL_FreeFormat(SDL_PixelFormat *a) {
  RDELETE(a);
}

void SDL_ShowWindow(SDL_Window *a) {
}

void SDL_RenderClear(SDL_Renderer *a) {
  memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
}

void SDL_RenderPresent(SDL_Renderer *a) {
  glRefresh();
}

void SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, SDL_Rect *src_rect, SDL_Rect *dst_rect) {
  if (glFrameBuffer == NULL) return;
  if (glFrameBufferWidth != r->window->w) return;
  if (glFrameBufferHeight != r->window->h) return;
  int sx0 = 0;
  int sy0 = 0;
  int sx1 = sx0+t->w;
  int sy1 = sy0+t->h;
  int dx0 = dst_rect->x;
  int dy0 = dst_rect->y;
  int dx1 = dx0+dst_rect->w;
  int dy1 = dy0+dst_rect->h;
  int minx = 0;
  int miny = 0;
  int maxx = r->window->w;
  int maxy = r->window->h;
  if (dx0>=maxx) return;
  if (dx1<=minx) return;
  if (dy0>=maxy) return;
  if (dy1<=miny) return;
  if (dx0<minx) {
    sx0 += minx-dx0;
    dx0 = minx;
  }
  if (dy0<miny) {
    sy0 += miny-dy0;
    dy0 = miny;
  }
  if (dx1>maxx) {
    sx1 -= dx1-maxx;
    dx1 = maxx;
  }
  if (dy1>maxy) {
    sy1 -= dy1-maxy;
    dy1 = maxy;
  }
  for (int y = dy0; y < dy1; y++) {
    const int sy = y - dy0 + sy0;
    if (sy >= 0 && sy < t->h) {
      for (int x = dx0; x < dx1; x++) {
        const int sx = x - dx0 + sx0;
        if (sx >= 0 && sx < t->w) {
          glFrameBuffer[x+y*glFrameBufferWidth] = t->pixels[sx+sy*t->w];
        }
      }
    }
  }
}

const char *SDL_GetPixelFormatName(unsigned int a) {
  return "";
}

#ifdef __cplusplus
}
#endif // __cplusplus
