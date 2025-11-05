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

// PMODEW is needed for the Irqs to work (not Dos4gw)

SDL_Rect DRectToRect(SDL_DRect *d) {SDL_Rect r; r.x = (int)d->x; r.y = (int)d->y; r.w = (int)d->w; r.h = (int)d->h; return r;}

SDL_Result SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {return SDL_ERROR;}
void SDL_CloseAudio() {}
void SDL_PauseAudio(int pause_on) {}
void SDL_PumpEvents() {}
Uint32 SDL_GetWindowID(SDL_Window *window) {return window->windowID;}
SDL_Result SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors) {for(int i = 0; i < ncolors; i++) palette->colors[i+firstcolor]=colors[i];return SDL_OK;}
SDL_Result SDL_UpperBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {return SDL_OK;}
SDL_Result SDL_LowerBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {return SDL_UpperBlit(src, srcrect, dst, dstrect);}
void SDL_SetWindowTitle(SDL_Window *window, const char *title) {window->title = title;}
Uint32 SDL_GetWindowPixelFormat(SDL_Window *window) {return window->pixelFormat;}
void SDL_SetWindowMinimumSize(SDL_Window *window, int min_w, int min_h) {window->min_w = min_w; window->min_h = min_h;}
SDL_Result SDL_RenderSetIntegerScale(SDL_Renderer *renderer, bool enable) {return SDL_OK;}
bool SDL_PixelFormatEnumToMasks(Uint32 format, int *bpp, Uint32 *Rmask, Uint32 *Gmask, Uint32 *Bmask, Uint32 *Amask) {return false;}
SDL_Result SDL_RenderGetViewport(SDL_Renderer *renderer, SDL_Rect *rect) {*rect = DRectToRect(&renderer->viewport); return SDL_OK;}
void SDL_RenderGetScale(SDL_Renderer *renderer, float *scaleX, float *scaleY) {*scaleX = renderer->scale.x; *scaleY = renderer->scale.y;}
SDL_Result SDL_GetRendererInfo(SDL_Renderer *renderer, SDL_RendererInfo *info) {*info = renderer->info; return SDL_OK;}
SDL_Result SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode *mode) {return SDL_OK;}
void SDL_WarpMouseInWindow(SDL_Window *window, int x, int y) {}
bool SDL_IsGameController(int joystick_index) {return false;}
SDL_Haptic *SDL_HapticOpen(int device_index) {return NULL;}
void SDL_HapticClose(SDL_Haptic *haptic) {;}
SDL_Result SDL_HapticRumbleInit(SDL_Haptic *haptic) {return SDL_ERROR;}
bool SDL_GameControllerGetAttached(SDL_GameController *gamecontroller) {return false;}
SDL_GameControllerType SDL_GameControllerTypeForIndex(int joystick_index) {return SDL_CONTROLLER_TYPE_UNKNOWN;}
SDL_Result SDL_GameControllerRumble(SDL_GameController *gamecontroller, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms) {return SDL_ERROR;}
int SDL_GetNumTouchFingers(SDL_TouchID touchID) {return 0;}
SDL_Result SDL_LockSurface(SDL_Surface *surface) {return SDL_OK;}
void SDL_UnlockSurface(SDL_Surface *surface) {}
void SDL_GL_GetDrawableSize(SDL_Window *window, int *w, int *h) {*w = window->w; *h = window->h;}
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode) {return SDLK_UNKNOWN;}
bool SDL_IsTextInputActive() {return false;}
void SDL_StartTextInput() {}
void SDL_StopTextInput() {}

void SDL_SetBiosCursor(int x, int y) {
    union REGS regs;
    regs.x.eax = 0x200;
    regs.h.bh = 0;
    regs.h.dl = (unsigned char)(x&255);
    regs.h.dh = (unsigned char)(y&255);
    int386(0x10, &regs, &regs);
}

#if defined(SDL_USETIMERIRQ) && (!defined(__DJGPP__))
static volatile double timerSeconds = 0;
static volatile int timerIrqPos = 0;
typedef void (__interrupt __far* TimerIrqHandler)();
static TimerIrqHandler oldTimerIrqHandler = NULL;
#define HARDWARE_TIMER_FREQUENCY 1193180 // 1193180 / 65536  = 18.2 (found in the internet that it's more like 1193181.66666666 instead of 1193180)
#define TIMER_MULTIPLICATOR 1024 // 1193180 * 1024 / 65536
static void (__interrupt __far timerIrqHandler) (void)
{
  timerSeconds += (double)1.0/((double)HARDWARE_TIMER_FREQUENCY * TIMER_MULTIPLICATOR / 65536.0);
  timerIrqPos++;
  if (timerIrqPos>=TIMER_MULTIPLICATOR) {
    timerIrqPos = 0;
    oldTimerIrqHandler();
  } else {
    outp(0x20,0x20);
  }
}

static void uninstallTimerIrq() {
  if (oldTimerIrqHandler != NULL) {
    union REGS r;
    outp(0x43,0x36);
    outp(0x40,0); // both 0 = 65536 = 1193180.0/65536.0 = ~18.2 hz
    outp(0x40,0);
    r.x.eax = 0x205;
    r.h.bl = 0x08;
    r.x.ecx = FP_SEG(oldTimerIrqHandler);
    r.x.edx = FP_OFF(oldTimerIrqHandler);
    int386 (0x31, &r, &r);
    oldTimerIrqHandler = NULL;
  }
}

static void installTimerIrq() {
  if (oldTimerIrqHandler == NULL) {
    timerSeconds=0;
    timerIrqPos=0;
    union REGS r;
    r.x.eax = 0x204;
    r.h.bl = 0x08;
    int386(0x31, &r, &r);
    oldTimerIrqHandler = (TimerIrqHandler)MK_FP(r.x.ecx,r.x.edx);
    int here = 65536/TIMER_MULTIPLICATOR;
    r.x.eax = 0x205;
    r.h.bl = 0x08;
    r.x.ecx = FP_SEG(timerIrqHandler);
    r.x.edx = FP_OFF(timerIrqHandler);
    int386 (0x31, &r, &r);
    outp(0x43,0x36);
    outp(0x40,here & 255);
    outp(0x40,(here>>8) & 255);
    atexit(uninstallTimerIrq);
  }
}

void SDL_SetSeconds(double s) {timerSeconds = s;}
double SDL_GetSeconds() {return timerSeconds;}

#else // SDL_USETIMERIRQ
static void installTimerIrq() {;}
static void uninstallTimerIrq() {;}
void SDL_SetSeconds(double s) {glSetTime(s);}
double SDL_GetSeconds() {return glSeconds();}
#endif // SDL_USETIMERIRQ

typedef void (__interrupt __far* KeyboardHandler)();
static KeyboardHandler oldKeyboardHandler = NULL;

Uint8 volatile keyPressed[0x100] = {0};

Uint8 isKeyPressed(int scanCode) {
  return keyPressed[scanCode & 0x7f];
}

static void (__interrupt __far keyboardHandler) (void) {
  unsigned char key = (unsigned char)inp(0x60);
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

SDL_MessageBoxButtonData _SDL_MessageBoxButtonData(int flags, int buttonid, const char *text) {
  SDL_MessageBoxButtonData r; r.flags = flags; r.buttonid = buttonid; r.text = text; return r;
}

SDL_Result SDL_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid) {
  return SDL_ERROR;
}

SDL_Result SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window) {
  return SDL_ERROR;
}

void SDL_Log(const char *fmt, ...) {;}
void SDL_GetVersion(SDL_version *a) {a->minor = SDL_MINOR_VERSION;a->major = SDL_MAJOR_VERSION;a->patch = SDL_PATCHLEVEL;}

Uint16 SDL_SwapLE16(Uint16 a) {
  return a;
}

Uint32 SDL_SwapLE32(Uint32 a) {
  return a;
}

Uint16 SDL_Swap16(Uint16 a) {
  return (Uint16)((a>>8)|(a<<8));
}

int snprintf(char *buffer, const int size, const char *format, ...) {
  return sprintf(buffer,format); // :mad: todo
}

char *SDL_GetPrefPath(const char *a, const char *b) {
  return ".";
}

char *SDL_GetBasePath() {
  return ".";
}

SDL_Result SDL_Init(Uint32 flags) {
  sdl_initialized |= flags;
  installKeyboardHandler();
  installTimerIrq();
  SDL_SetSeconds(0);
  //glWaitVSync = GL_TRUE;
  return SDL_OK;
}

bool SDL_WasInit(Uint32 flags) {
  return (sdl_initialized & flags)==flags ? true : false;
}

SDL_Result SDL_InitSubSystem(Uint32 flags) {
  sdl_initialized |= flags;
  return flags == SDL_INIT_AUDIO ? SDL_ERROR : SDL_OK;
}

void SDL_QuitSubSystem(Uint32 flags) {
}

void SDL_free(void *a) {
  free(a);
}

int SDL_GetNumVideoDisplays() {
  return 1;
}

int sdlDrawColorR = 0;
int sdlDrawColorG = 0;
int sdlDrawColorB = 0;
int sdlDrawColorA = 0;

SDL_Result SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  sdlDrawColorR = r;
  sdlDrawColorG = g;
  sdlDrawColorB = b;
  sdlDrawColorA = a;
  return SDL_OK;
}

void SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color) {
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = dx0 + dst->w;
  int dy1 = dy0 + dst->h;
  if (rect != NULL) {
    dx0 = rect->x;
    dy0 = rect->y;
    dx1 = dx0+rect->w;
    dy1 = dy0+rect->h;
  }
  if (dx0<0) dx0 = 0;
  if (dy0<0) dy0 = 0;
  if (dx1>dst->w) dx1 = dst->w;
  if (dy1>dst->h) dy1 = dst->h;
  if (dst->format->BitsPerPixel==8) {
    for (int y = dy0; y < dy1; y++) {
      for (int x = dx0; x < dx1; x++) {
        dst->pixels[x+y*dst->pitch]=0;
      }
    }
  }
}

SDL_Scancode SDL_GetScancodeFromName(const char *name) {
  return SDL_SCANCODE_NONE;
}

const char *SDL_GetScancodeName(SDL_Scancode scancode) {
  return "none";
}

void SDL_Delay(double ms) {
  double b = SDL_GetTicks();
  while(fabs((double)SDL_GetTicks()-b)<ms) {;}
}

void SDL_Quit() {
  glDone();
  uninstallKeyboardHandler();
  uninstallTimerIrq();
  exit(0);
}

int SDL_JoystickEventState(int state) {
  return 0;
}

int SDL_JoystickGetAxis(SDL_Joystick *joystick, int axis) {
  return 0;
}

int SDL_JoystickGetButton(SDL_Joystick *joystick, int button) {
  return 0;
}

int SDL_JoystickGetHat(SDL_Joystick *joystick, int hat) {
  return 0;
}

void SDL_JoystickUpdate() {
}

int SDL_NumJoysticks() {
  return 0;
}

SDL_Joystick *SDL_JoystickOpen(int device_index) {
  return NULL;
}

void SDL_JoystickClose(SDL_Joystick *joystick) {
}

const char *SDL_JoystickName(SDL_Joystick *joystick) {
  return NULL;
}

const char *SDL_JoystickNameForIndex(int device_index) {
  return NULL;
}

int SDL_JoystickNumAxes(SDL_Joystick *joystick) {
  return 0;
}

int SDL_JoystickNumButtons(SDL_Joystick *joystick) {
  return 0;
}

int SDL_JoystickNumHats(SDL_Joystick *joystick) {
  return 0;
}

SDL_GameController *SDL_GameControllerOpen(int joystick_index) {
  return 0;
}

int SDL_GameControllerGetButton(SDL_GameController *gamecontroller,SDL_GameControllerButton button) {
  return 0;
}

int SDL_GameControllerGetAxis(SDL_GameController *gamecontroller,SDL_GameControllerAxis axis) {
  return 0;
}

void SDL_GameControllerClose(SDL_GameController *gamecontroller) {
}

Uint32 SDL_GetTicks() {
  return (Uint32)(SDL_GetSeconds()*1000.0);
}

SDL_Event events[256];
int currentEvent=0;

Uint8 keyPressed_last[0x100] = {0};

Uint8 *SDL_GetKeyboardState(int *numkeys) {
  *numkeys = sizeof(keyPressed_last);
  return keyPressed_last;
}

void SDL_ResetKeyboard() {
  memset(keyPressed_last,0,sizeof(keyPressed_last));
  memset((void*)keyPressed,0,sizeof(keyPressed));
}

const char *SDL_GetKeyName(SDL_Keycode key) {
  static char buffer[2];
  buffer[1] = 0;
  buffer[0] = (Uint8)((int)key & 255);
  return buffer;
}

/*
  CONVSC2(SDLK_EXCLAIM,SDL_SCANCODE_EXCLAIM)\
  CONVSC2(SDLK_QUOTEDBL,SDL_SCANCODE_QUOTEDBL)\
  CONVSC2(SDLK_HASH,SDL_SCANCODE_HASH)\
  CONVSC2(SDLK_PERCENT,SDL_SCANCODE_PERCENT)\
  CONVSC2(SDLK_DOLLAR,SDL_SCANCODE_DOLLAR)\
  CONVSC2(SDLK_AMPERSAND,SDL_SCANCODE_AMPERSAND)\
  CONVSC2(SDLK_QUOTE,SDL_SCANCODE_QUOTE)\
  CONVSC2(SDLK_LEFTPAREN,SDL_SCANCODE_LEFTPAREN)\
  CONVSC2(SDLK_RIGHTPAREN,SDL_SCANCODE_RIGHTPAREN)\
  CONVSC2(SDLK_COLON,SDL_SCANCODE_COLON)\
  CONVSC2(SDLK_LESS,SDL_SCANCODE_LESS)\
  CONVSC2(SDLK_GREATER,SDL_SCANCODE_GREATER)\
  CONVSC2(SDLK_QUESTION,SDL_SCANCODE_QUESTION)\
  CONVSC2(SDLK_AT,SDL_SCANCODE_AT)\
  CONVSC2(SDLK_CARET,SDL_SCANCODE_CARET)\
  CONVSC2(SDLK_UNDERSCORE,SDL_SCANCODE_UNDERSCORE)\
  CONVSC2(SDLK_SCROLLLOCK,SDL_SCANCODE_SCROLLOCK)\
  CONVSC2(SDLK_ASTERISK,SDL_SCANCODE_ASTERISK)\
  CONVSC2(SDLK_BACKQUOTE,SDL_SCANCODE_BACKQUOTE)\
  CONVSC2(SDLK_KP_PERIOD,SDL_SCANCODE_KP_PERIOD)\
  CONVSC2(SDLK_KP_DIVIDE,SDL_SCANCODE_KP_DIVIDE)\
  CONVSC2(SDLK_KP_ENTER,SDL_SCANCODE_KP_ENTER)\
  CONVSC2(SDLK_PAUSE,SDL_SCANCODE_PAUSE)\
  CONVSC2(SDLK_KP_1,SDL_SCANCODE_KP_1)\
  CONVSC2(SDLK_KP_2,SDL_SCANCODE_KP_2)\
  CONVSC2(SDLK_KP_3,SDL_SCANCODE_KP_3)\
  CONVSC2(SDLK_KP_4,SDL_SCANCODE_KP_4)\
  CONVSC2(SDLK_KP_6,SDL_SCANCODE_KP_6)\
  CONVSC2(SDLK_KP_7,SDL_SCANCODE_KP_7)\
  CONVSC2(SDLK_KP_8,SDL_SCANCODE_KP_8)\
  CONVSC2(SDLK_KP_9,SDL_SCANCODE_KP_9)\
  CONVSC2(SDLK_KP_0,SDL_SCANCODE_KP_0)\
  CONVSC2(SDLK_KP_MULTIPLY,SDL_SCANCODE_KP_MULTIPLY)\
  CONVSC2(SDLK_RCTRL,SDL_SCANCODE_RCTRL)\
  CONVSC2(SDLK_RALT,SDL_SCANCODE_RALT)\
  CONVSC2(SDLK_PERIOD,SDL_SCANCODE_PERIOD)\
  CONVSC2(SDLK_SLASH,SDL_SCANCODE_SLASH)\
*/
#define CONVSC2(__ret__,__from__) case __from__ : {return __ret__;}
#define CONVSCM\
  CONVSC2(SDLK_UNKNOWN,SDL_SCANCODE_UNKNOWN)\
  CONVSC2(SDLK_RETURN,SDL_SCANCODE_RETURN)\
  CONVSC2(SDLK_ESCAPE,SDL_SCANCODE_ESCAPE)\
  CONVSC2(SDLK_BACKSPACE,SDL_SCANCODE_BACKSPACE)\
  CONVSC2(SDLK_TAB,SDL_SCANCODE_TAB)\
  CONVSC2(SDLK_SPACE,SDL_SCANCODE_SPACE)\
  CONVSC2(SDLK_PLUS,SDL_SCANCODE_PLUS)\
  CONVSC2(SDLK_COMMA,SDL_SCANCODE_COMMA)\
  CONVSC2(SDLK_MINUS,SDL_SCANCODE_MINUS)\
  CONVSC2(SDLK_0,SDL_SCANCODE_0)\
  CONVSC2(SDLK_1,SDL_SCANCODE_1)\
  CONVSC2(SDLK_2,SDL_SCANCODE_2)\
  CONVSC2(SDLK_3,SDL_SCANCODE_3)\
  CONVSC2(SDLK_4,SDL_SCANCODE_4)\
  CONVSC2(SDLK_5,SDL_SCANCODE_5)\
  CONVSC2(SDLK_6,SDL_SCANCODE_6)\
  CONVSC2(SDLK_7,SDL_SCANCODE_7)\
  CONVSC2(SDLK_8,SDL_SCANCODE_8)\
  CONVSC2(SDLK_9,SDL_SCANCODE_9)\
  CONVSC2(SDLK_SEMICOLON,SDL_SCANCODE_SEMICOLON)\
  CONVSC2(SDLK_EQUALS,SDL_SCANCODE_EQUALS)\
  CONVSC2(SDLK_LEFTBRACKET,SDL_SCANCODE_LEFTBRACKET)\
  CONVSC2(SDLK_BACKSLASH,SDL_SCANCODE_BACKSLASH)\
  CONVSC2(SDLK_RIGHTBRACKET,SDL_SCANCODE_RIGHTBRACKET)\
  CONVSC2(SDLK_a,SDL_SCANCODE_A)\
  CONVSC2(SDLK_b,SDL_SCANCODE_B)\
  CONVSC2(SDLK_c,SDL_SCANCODE_C)\
  CONVSC2(SDLK_d,SDL_SCANCODE_D)\
  CONVSC2(SDLK_e,SDL_SCANCODE_E)\
  CONVSC2(SDLK_f,SDL_SCANCODE_F)\
  CONVSC2(SDLK_g,SDL_SCANCODE_G)\
  CONVSC2(SDLK_h,SDL_SCANCODE_H)\
  CONVSC2(SDLK_i,SDL_SCANCODE_I)\
  CONVSC2(SDLK_j,SDL_SCANCODE_J)\
  CONVSC2(SDLK_k,SDL_SCANCODE_K)\
  CONVSC2(SDLK_l,SDL_SCANCODE_L)\
  CONVSC2(SDLK_m,SDL_SCANCODE_M)\
  CONVSC2(SDLK_n,SDL_SCANCODE_N)\
  CONVSC2(SDLK_o,SDL_SCANCODE_O)\
  CONVSC2(SDLK_p,SDL_SCANCODE_P)\
  CONVSC2(SDLK_q,SDL_SCANCODE_Q)\
  CONVSC2(SDLK_r,SDL_SCANCODE_R)\
  CONVSC2(SDLK_s,SDL_SCANCODE_S)\
  CONVSC2(SDLK_t,SDL_SCANCODE_T)\
  CONVSC2(SDLK_u,SDL_SCANCODE_U)\
  CONVSC2(SDLK_v,SDL_SCANCODE_V)\
  CONVSC2(SDLK_w,SDL_SCANCODE_W)\
  CONVSC2(SDLK_x,SDL_SCANCODE_X)\
  CONVSC2(SDLK_y,SDL_SCANCODE_Y)\
  CONVSC2(SDLK_z,SDL_SCANCODE_Z)\
  CONVSC2(SDLK_CAPSLOCK,SDL_SCANCODE_CAPSLOCK)\
  CONVSC2(SDLK_F1,SDL_SCANCODE_F1)\
  CONVSC2(SDLK_F2,SDL_SCANCODE_F2)\
  CONVSC2(SDLK_F3,SDL_SCANCODE_F3)\
  CONVSC2(SDLK_F4,SDL_SCANCODE_F4)\
  CONVSC2(SDLK_F5,SDL_SCANCODE_F5)\
  CONVSC2(SDLK_F6,SDL_SCANCODE_F6)\
  CONVSC2(SDLK_F7,SDL_SCANCODE_F7)\
  CONVSC2(SDLK_F8,SDL_SCANCODE_F8)\
  CONVSC2(SDLK_F9,SDL_SCANCODE_F9)\
  CONVSC2(SDLK_F10,SDL_SCANCODE_F10)\
  CONVSC2(SDLK_F11,SDL_SCANCODE_F11)\
  CONVSC2(SDLK_F12,SDL_SCANCODE_F12)\
  CONVSC2(SDLK_PRINTSCREEN,SDL_SCANCODE_PRINTSCREEN)\
  CONVSC2(SDLK_INSERT,SDL_SCANCODE_INSERT)\
  CONVSC2(SDLK_HOME,SDL_SCANCODE_HOME)\
  CONVSC2(SDLK_PAGEUP,SDL_SCANCODE_PAGEUP)\
  CONVSC2(SDLK_DELETE,SDL_SCANCODE_DELETE)\
  CONVSC2(SDLK_END,SDL_SCANCODE_END)\
  CONVSC2(SDLK_PAGEDOWN,SDL_SCANCODE_PAGEDOWN)\
  CONVSC2(SDLK_RIGHT,SDL_SCANCODE_RIGHT)\
  CONVSC2(SDLK_LEFT,SDL_SCANCODE_LEFT)\
  CONVSC2(SDLK_DOWN,SDL_SCANCODE_DOWN)\
  CONVSC2(SDLK_UP,SDL_SCANCODE_UP)\
  CONVSC2(SDLK_NUMLOCKCLEAR,SDL_SCANCODE_NUMLOCKCLEAR)\
  CONVSC2(SDLK_KP_MINUS,SDL_SCANCODE_KP_MINUS)\
  CONVSC2(SDLK_KP_PLUS,SDL_SCANCODE_KP_PLUS)\
  CONVSC2(SDLK_LCTRL,SDL_SCANCODE_LCTRL)\
  CONVSC2(SDLK_LSHIFT,SDL_SCANCODE_LSHIFT)\
  CONVSC2(SDLK_LALT,SDL_SCANCODE_LALT)\
  CONVSC2(SDLK_RSHIFT,SDL_SCANCODE_RSHIFT)\
  CONVSC2(SDLK_KP_5,SDL_SCANCODE_KP_5)


SDL_KeyCode convertScanCodeToSymbolic(int scancode) {
  switch(scancode) {
    CONVSCM
  }
  return SDLK_UNKNOWN;
}

bool addKeyboardEvents() {
  int key = glNextKey();
  if (key == GL_VK_END) {
    SDL_Quit();
  }
  if (key != 0) {
    if (key < 128 && key >= 32) {
      SDL_Event *v = &events[currentEvent];
      v->type = SDL_TEXTINPUT;
      v->text.text[0] = (unsigned char)(key);
      v->text.text[1] = 0;
      currentEvent++;
    }
  }
  for (int i = 0; i < sizeof(keyPressed); i++) {
    if (keyPressed[i] != keyPressed_last[i]) {
      keyPressed_last[i] = keyPressed[i];
      SDL_Event *v = &events[currentEvent];
      v->type = keyPressed[i] ? SDL_KEYDOWN : SDL_KEYUP;
      v->key.state = keyPressed[i] ? 1 : 0;
      v->key.keysym.scancode = i;
      v->key.keysym.sym = convertScanCodeToSymbolic(i);
      v->key.keysym.mod = 0;
      if (keyPressed[SDL_SCANCODE_LCTRL]) v->key.keysym.mod|=(int)KMOD_LCTRL;
      if (keyPressed[SDL_SCANCODE_RCTRL]) v->key.keysym.mod|=(int)KMOD_RCTRL;
      if (keyPressed[SDL_SCANCODE_LALT]) v->key.keysym.mod|=(int)KMOD_LALT;
      if (keyPressed[SDL_SCANCODE_RALT]) v->key.keysym.mod|=(int)KMOD_RALT;
      if (keyPressed[SDL_SCANCODE_LSHIFT]) v->key.keysym.mod|=(int)KMOD_LSHIFT;
      if (keyPressed[SDL_SCANCODE_RSHIFT]) v->key.keysym.mod|=(int)KMOD_RSHIFT;
      currentEvent++;
    }
  }
  return currentEvent>0;
}

Uint32 sdlMouseButtonState = 0;
bool buttonPressed_last[3] = {0};
double sdlMouseX = 0;
double sdlMouseY = 0;
double sdlMouseRelX = 0;
double sdlMouseRelY = 0;

Uint32 SDL_GetMouseState(int *x, int *y) {
  if (x != NULL) *x=(int)sdlMouseX;
  if (y != NULL) *y=(int)sdlMouseY;
  return sdlMouseButtonState;
}

Uint32 SDL_GetRelativeMouseState(int *x, int *y) {
  if (x != NULL) *x=(int)sdlMouseRelX;
  if (y != NULL) *y=(int)sdlMouseRelY;
  return sdlMouseButtonState;
}

bool addMouseEvents() {
  double dx,dy;
  glNextMouseDelta(&dx,&dy);
  sdlMouseRelX = (int)dx;
  sdlMouseRelY = (int)dy;
  sdlMouseX += dx;
  sdlMouseY += dy;
  if (sdlMouseX<0)sdlMouseX=0;
  if (sdlMouseY<0)sdlMouseY=0;
  if (sdlMouseX>glFrameBufferWidth-1) sdlMouseX=glFrameBufferWidth-1;
  if (sdlMouseY>glFrameBufferHeight-1) sdlMouseY=glFrameBufferHeight-1;
  sdlMouseButtonState = 0;
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
      sdlMouseButtonState|=(Uint32)j;
      v->button.button = (Uint8)j;
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
  return currentEvent>0;
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

int SDL_PushEvent(SDL_Event *event) {
  return 0;
}

int SDL_PollEvent(SDL_Event *event) {
  if (currentEvent <= 0) {
    currentEvent = 0;
    addKeyboardEvents();
    addMouseEvents();
    return 0;  
  }
  currentEvent--;
  if (event != NULL) *event = events[currentEvent];
  return 1;
}

int SDL_WaitEvent(SDL_Event *event) {
  while(!SDL_PollEvent(event));
  if (event==NULL) currentEvent++;
  return 1;
}

Uint8 SDL_EventState(Uint32 type, int state) {
  return 0;
}

const char *SDL_GetError() {
  return "";
}

int SDL_ShowCursor(int toggle) {
  return 0;
}

size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen) {
  int i;
  for (i = 0; i < maxlen-1; i++) {
    if (src[i]==0x00) break;
    dst[i]=src[i];
  }
  dst[i]=0x00;
  return i;
}

int SDL_SetRelativeMouseMode(bool enabled) {
  return 0;
}

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes) {
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

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, SDL_AudioFormat src_format, Uint8 src_channels, int src_rate, SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate) {
  return 0;
}

SDL_Result SDL_ConvertAudio(SDL_AudioCVT *cvt) {
  return SDL_OK;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat * format, Uint8 r, Uint8 g, Uint8 b) {
  switch(format->BitsPerPixel) {
  case 32:return (int)r|((int)g<<8)|((int)b<<16)|0xff000000;
  }
  return rand()*0x01020304;
}

SDL_Keymod SDL_GetModState() {
  int r = (int)KMOD_NONE;
  if (keyPressed[SDL_SCANCODE_LCTRL]) r|=(int)KMOD_LCTRL;
  if (keyPressed[SDL_SCANCODE_RCTRL]) r|=(int)KMOD_RCTRL;
  if (keyPressed[SDL_SCANCODE_LALT]) r|=(int)KMOD_LALT;
  if (keyPressed[SDL_SCANCODE_RALT]) r|=(int)KMOD_RALT;
  if (keyPressed[SDL_SCANCODE_LSHIFT]) r|=(int)KMOD_LSHIFT;
  if (keyPressed[SDL_SCANCODE_RSHIFT]) r|=(int)KMOD_RSHIFT;
  return (SDL_Keymod)r;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
  RALLOC(SDL_Surface,r);
  r->format = _RALLOC(SDL_PixelFormat,1);
  r->format->BitsPerPixel = (Uint8)depth;
  r->pixels = (Uint8*)malloc(width*height*(depth/8));
  r->pitch = width*(depth/8);
  r->w = width;
  r->h = height;
  return r;
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
  return NULL;
}

SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *a, SDL_Surface *b) {
  return NULL;
}

void SDL_FreeSurface(SDL_Surface *a) {
  if (a!=NULL) {
    RDELETE(a->pixels);
    RDELETE(a->format);
  }
  RDELETE(a);
}

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
  RALLOC(SDL_Window,r);
  r->x = x;
  r->y = y;
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

void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon) {
}

void SDL_DestroyWindow(SDL_Window *window) {
  RDELETE(window);
}

void SDL_GetWindowSize(SDL_Window *window, int *w, int *h) {
  if (w != NULL) *w = window->w;
  if (h != NULL) *h = window->h;
}

Uint32 SDL_GetWindowFlags(SDL_Window *window) {
  return window->flags;
}

void SDL_SetWindowPosition(SDL_Window *window, int x, int y) {
  window->x = x;
  window->y = y;
}

void SDL_SetWindowSize(SDL_Window *window, int w, int h) {
  glDone();
  if (!glVesa(w,h,32)) {
    printf("Vesa: SDL window:%dx%d failed!\n",w,h);
    SDL_Quit();
  }
  window->w = w;
  window->h = h;
}

SDL_Result SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags) {
  return SDL_OK;
}

SDL_Result SDL_GetDisplayBounds(int displayIndex, SDL_Rect *rect) {
  return SDL_OK;
}

int SDL_GetWindowDisplayIndex(SDL_Window *window) {
  return 0;
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags) {
  RALLOC(SDL_Renderer,r);
  r->window = window;
  return r;
}
                                            
SDL_Result SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture) {
  renderer->renderTarget = texture;
  return SDL_OK;
}

SDL_Result SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h) {
  if (renderer != NULL & w != NULL && renderer->window != NULL) *w = renderer->window->w;
  if (renderer != NULL & h != NULL && renderer->window != NULL) *h = renderer->window->h;
  return SDL_OK;
}

SDL_Result SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h) {
  renderer->logical_w = w;
  renderer->logical_h = h;
  return SDL_OK;
}

void SDL_DestroyRenderer(SDL_Renderer *renderer) {
  RDELETE(renderer);
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) {
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

SDL_Result SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h) {
  if (texture!=NULL) *w = texture->w;
  if (texture!=NULL) *h = texture->h;
  return SDL_OK;
}

SDL_Result SDL_LockTexture(SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch) {
  if (pixels != NULL) *pixels = (void*)texture->pixels;
  if (pitch != NULL) *pitch = texture->pitch;
  return SDL_OK;
}

SDL_Result SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch) {
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = texture->w;
  int dy1 = texture->h;
  int sx0 = dx0;
  int sy0 = dy0;
  int sx1 = dx1;
  int sy1 = dy1;
  Uint8 *s = (Uint8*)pixels;
  Uint8 *d = (Uint8*)texture->pixels;
  s += sx0*4+sy0*pitch;
  d += dx0*4+dy0*texture->pitch;
  for (int y = dy0; y < dy1; y++) {
    Uint32 *s2 = (Uint32*)s;
    Uint32 *d2 = (Uint32*)d;
    for (int x = dx0; x < dx1; x++) {
      *d2=*s2;
      s2++;
      d2++;
    }
    s += pitch;
    d += texture->pitch;
  }
  return SDL_OK;
}

void SDL_UnlockTexture(SDL_Texture *t) {
}

SDL_Result SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode blendMode) {
  texture->blendMode = blendMode;
  return SDL_OK;
}

SDL_Result SDL_SetTextureAlphaMod(SDL_Texture *texture, Uint8 alpha) {
  texture->modMode = alpha;
  return SDL_OK;
}

SDL_PixelFormat *SDL_AllocFormat(Uint32 pixel_format) {
  RALLOC(SDL_PixelFormat,r);
  switch(pixel_format) {
  case SDL_PIXELFORMAT_RGB888: {
    r->BitsPerPixel = 32;
  } break;
  }
  return r;
}

void SDL_FreeFormat(SDL_PixelFormat *format) {
  RDELETE(format);
}

void SDL_ShowWindow(SDL_Window *window) {
}

SDL_Result SDL_RenderClear(SDL_Renderer *renderer) {
  memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
  return SDL_OK;
}

void SDL_RenderPresent(SDL_Renderer *renderer) {
  glRefresh();
}

SDL_Result SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect) {
  if (texture == NULL) return SDL_ERROR;
  if (glFrameBuffer == NULL) return SDL_ERROR;
  if (glFrameBufferWidth != renderer->window->w) return SDL_ERROR;
  if (glFrameBufferHeight != renderer->window->h) return SDL_ERROR;
  int sx0 = 0;
  int sy0 = 0;
  int sx1 = sx0+texture->w;
  int sy1 = sy0+texture->h;
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = renderer->window->w;
  int dy1 = renderer->window->h;
  if (srcrect != NULL) {
    sx0 = srcrect->x;
    sy0 = srcrect->y;
    sx1 = sx0+srcrect->w;
    sy1 = sy0+srcrect->h;
  }
  if (dstrect != NULL) {
    dx0 = dstrect->x;
    dy0 = dstrect->y;
    dx1 = dx0+dstrect->w;
    dy1 = dy0+dstrect->h;
  }
  bool sameSize=((sx1-sx0)==(dx1-dx0))&&((sy1-sy0)==(dy1-dy0));
  int minx = 0;
  int miny = 0;
  int maxx = renderer->window->w;
  int maxy = renderer->window->h;
  if (dx0>=maxx) return SDL_ERROR;
  if (dx1<=minx) return SDL_ERROR;
  if (dy0>=maxy) return SDL_ERROR;
  if (dy1<=miny) return SDL_ERROR;
  if (sameSize) {
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
      if (sy >= 0 && sy < texture->h) {
        for (int x = dx0; x < dx1; x++) {
          const int sx = x - dx0 + sx0;
          if (sx >= 0 && sx < texture->w) {
            glFrameBuffer[x+y*glFrameBufferWidth] = texture->pixels[sx+sy*texture->w];
          }
        }
      }
    }
  } else { // sameSize
    double sfx0 = sx0;
    double sfx1 = sx1;
    double sfy0 = sy0;
    double sfy1 = sy1;
    double ax = (double)(sx1-sx0)/(dx1-dx0);
    double ay = (double)(sy1-sy0)/(dy1-dy0);
    if (dx0<minx) {
      sfx0 += (minx-dx0)*ax;
      dx0 = minx;
    }
    if (dy0<miny) {
      sfy0 += (miny-dy0)*ay;
      dy0 = miny;
    }
    if (dx1>maxx) {
      sfx1 -= (dx1-maxx)*ax;
      dx1 = maxx;
    }
    if (dy1>maxy) {
      sfy1 -= (dy1-maxy)*ay;
      dy1 = maxy;
    }
    for (int y = dy0; y < dy1; y++) {
      const int sy = (int)((y - dy0)*ay + sfy0);
      if (sy >= 0 && sy < texture->h) {
        for (int x = dx0; x < dx1; x++) {
          const int sx = (int)((x - dx0)*ax + sx0);
          if (sx >= 0 && sx < texture->w) {
            glFrameBuffer[x+y*glFrameBufferWidth] = texture->pixels[sx+sy*texture->w];
          }
        }
      }
    }
  } // sameSize
  return SDL_OK;
}

const char *SDL_GetPixelFormatName(Uint32 format) {
  return "";
}

bool SDL_HasClipboardText() {
  return false;
}

char *SDL_GetClipboardText() {
  return "";
}

bool SDL_HasScreenKeyboardSupport() {
  return false;
}

void SDL_SaveBMP(SDL_Surface *surface, const char *file) {
}

const char *SDL_GetCurrentVideoDriver() {
  return "WatcomGL";
}

const char *SDL_GetCurrentAudioDriver() {
  return "AudioSilence";
}

SDL_Result SDL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 format, void *pixels, int pitch) {
  return SDL_ERROR;
}

SDL_RWops *SDL_RWFromMem(void *mem, int size) {
  return NULL;
}

SDL_Surface *SDL_LoadBMP_RW(SDL_RWops * src, int freesrc) {
  return NULL;
}

#ifdef __cplusplus
}
#endif // __cplusplus
