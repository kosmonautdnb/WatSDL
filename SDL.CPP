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

void SDL_SetBiosCursor(int x, int y) {
    union REGS regs;
    regs.x.eax = 0x200;
    regs.h.bh = 0;
    regs.h.dl = x;
    regs.h.dh = y;
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

SDL_MessageBoxButtonData _SDL_MessageBoxButtonData(int flags, int buttonid, const char *text) {
  SDL_MessageBoxButtonData r; r.flags = flags; r.buttonid = buttonid; r.text = text; return r;
}

SDL_Result SDL_ShowMessageBox(SDL_MessageBoxData *a, int *b) {
  return SDL_ERROR;
}

SDL_Result SDL_ShowSimpleMessageBox(int flags, const char *title, const char *message, SDL_Window *window) {
  return SDL_ERROR;
}

void SDL_Log(const char *a) {;}
void SDL_GetVersion(SDL_version *a) {a->minor = SDL_MINOR_VERSION;a->major = SDL_MAJOR_VERSION;a->patch = SDL_PATCHLEVEL;}

unsigned short SDL_SwapLE16(unsigned short a) {
  return a;
}

unsigned int SDL_SwapLE32(unsigned int a) {
  return a;
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

SDL_Result SDL_Init(int flags) {
  sdl_initialized |= flags;
  installKeyboardHandler();
  installTimerIrq();
  SDL_SetSeconds(0);
  //glWaitVSync = GL_TRUE;
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
  double b = SDL_GetTicks();
  while(fabs((double)SDL_GetTicks()-b)<a) {;}
}

void SDL_Quit() {
  glDone();
  uninstallKeyboardHandler();
  uninstallTimerIrq();
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

const char *SDL_JoystickNameForIndex(int a) {
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

SDL_GameController *SDL_GameControllerOpen(int a) {
  return 0;
}

int SDL_GameControllerGetButton(SDL_GameController *a,int b) {
  return 0;
}

int SDL_GameControllerGetAxis(SDL_GameController *a,int b) {
  return 0;
}

void SDL_GameControllerClose(SDL_GameController *a) {
}

int SDL_GetTicks() {
  return (int)(SDL_GetSeconds()*1000.0);
}

SDL_Event events[256];
int currentEvent=0;

Uint8 keyPressed_last[0x100] = {0};

Uint8 *SDL_GetKeyboardState(void *a) {
  return keyPressed_last;
}

void SDL_ResetKeyboard() {
  memset(keyPressed_last,0,sizeof(keyPressed_last));
  memset((void*)keyPressed,0,sizeof(keyPressed));
}

const char *SDL_GetKeyName(SDL_Keycode a) {
  static char buffer[2];
  buffer[1] = 0;
  buffer[0] = (int)a & 255;
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
#define CONVSC2(__ret__,__from__) case __from__ : {return __ret__;} break;
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


SDL_KeyCode convertScanCodeToSymbolic(int a) {
  switch(a) {
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
      v->text.text[0] = key;
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

bool buttonPressed_last[3] = {0};
double sdlMouseX = 0;
double sdlMouseY = 0;

bool addMouseEvents() {
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
  if (e != NULL) *e = events[currentEvent];
  return true;
}

bool SDL_WaitEvent(SDL_Event *e) {
  while(!SDL_PollEvent(e));
  if (e==NULL) currentEvent++;
  return true;
}

void SDL_EventState(SDL_EventType a, int b) {
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
  RALLOC(SDL_Surface,r);
  r->format = _RALLOC(SDL_PixelFormat,1);
  r->format->BitsPerPixel = cl;
  r->pixels = (Uint8*)malloc(w*h*(cl/8)); // 8bit
  r->pitch = w;
  r->w = w;
  r->h = h;
  return r;
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int bpp, int pitch, int rmask, int gmask, int bmask, int amask) {
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

void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon) {
}

void SDL_DestroyWindow(SDL_Window *a) {
  RDELETE(a);
}

void SDL_GetWindowSize(SDL_Window *a, int *w, int *h) {
  if (w != NULL) *w = a->w;
  if (h != NULL) *h = a->h;
}

int SDL_GetWindowFlags(SDL_Window *a) {
  return a->flags;
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
                                            
void SDL_SetRenderTarget(SDL_Renderer *a, SDL_Texture *b) {
  a->renderTarget = b;
}

void SDL_GetRendererOutputSize(SDL_Renderer *a, int *w, int *h) {
  if (a != NULL & w != NULL && a->window != NULL) *w = a->window->w;
  if (a != NULL & h != NULL && a->window != NULL) *h = a->window->h;
}

void SDL_RenderSetLogicalSize(SDL_Renderer *a, int w, int h) {
  a->logical_w = w;
  a->logical_h = h;
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

SDL_Result SDL_UpdateTexture(SDL_Texture *t, const void *a, const void *pixels, int pitch) {
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = t->w;
  int dy1 = t->h;
  int sx0 = dx0;
  int sy0 = dy0;
  int sx1 = dx1;
  int sy1 = dy1;
  Uint8 *s = (Uint8*)pixels;
  Uint8 *d = (Uint8*)t->pixels;
  s += sx0*4+sy0*pitch;
  d += dx0*4+dy0*t->pitch;
  for (int y = dy0; y < dy1; y++) {
    Uint32 *s2 = (Uint32*)s;
    Uint32 *d2 = (Uint32*)d;
    for (int x = dx0; x < dx1; x++) {
      *d2=*s2;
      s2++;
      d2++;
    }
    s += pitch;
    d += t->pitch;
  }
  return SDL_OK;
}

void SDL_UnlockTexture(SDL_Texture *t) {
}

void SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode b) {
  texture->blendMode = b;
}

void SDL_SetTextureAlphaMod(SDL_Texture *texture, int b) {
  texture->modMode = b;
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

int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, SDL_Rect *src_rect, SDL_Rect *dst_rect) {
  if (t == NULL) return SDL_ERROR;
  if (glFrameBuffer == NULL) return SDL_ERROR;
  if (glFrameBufferWidth != r->window->w) return SDL_ERROR;
  if (glFrameBufferHeight != r->window->h) return SDL_ERROR;
  int sx0 = 0;
  int sy0 = 0;
  int sx1 = sx0+t->w;
  int sy1 = sy0+t->h;
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = r->window->w;
  int dy1 = r->window->h;
  if (src_rect != NULL) {
    sx0 = src_rect->x;
    sy0 = src_rect->y;
    sx1 = sx0+src_rect->w;
    sy1 = sy0+src_rect->h;
  }
  if (dst_rect != NULL) {
    dx0 = dst_rect->x;
    dy0 = dst_rect->y;
    dx1 = dx0+dst_rect->w;
    dy1 = dy0+dst_rect->h;
  }
  bool sameSize=((sx1-sx0)==(dx1-dx0))&&((sy1-sy0)==(dy1-dy0));
  int minx = 0;
  int miny = 0;
  int maxx = r->window->w;
  int maxy = r->window->h;
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
      if (sy >= 0 && sy < t->h) {
        for (int x = dx0; x < dx1; x++) {
          const int sx = x - dx0 + sx0;
          if (sx >= 0 && sx < t->w) {
            glFrameBuffer[x+y*glFrameBufferWidth] = t->pixels[sx+sy*t->w];
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
      if (sy >= 0 && sy < t->h) {
        for (int x = dx0; x < dx1; x++) {
          const int sx = (int)((x - dx0)*ax + sx0);
          if (sx >= 0 && sx < t->w) {
            glFrameBuffer[x+y*glFrameBufferWidth] = t->pixels[sx+sy*t->w];
          }
        }
      }
    }
  } // sameSize
  return SDL_OK;
}

const char *SDL_GetPixelFormatName(unsigned int a) {
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

void SDL_SaveBMP(SDL_Surface *a, const char *path) {
}

const char *SDL_GetCurrentVideoDriver() {
  return "WatcomGL";
}

const char *SDL_GetCurrentAudioDriver() {
  return "AudioSilence";
}

SDL_Result SDL_RenderReadPixels(SDL_Renderer *a, int b, SDL_PixelFormatEnum format, void *pixels, int pitch) {
  return SDL_ERROR;
}

SDL_RWops *SDL_RWFromMem(void *a, int b) {
  return NULL;
}

SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *a,int b) {
  return NULL;
}

#ifdef __cplusplus
}
#endif // __cplusplus
