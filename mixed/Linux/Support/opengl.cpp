/*
 * Copyright (c) 2016, Timothy Baldwin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of RISC OS Open Ltd nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <GL/glut.h>
#include <GL/freeglut.h>
#include <unistd.h>
#include <thread>
#include <array>
#include <algorithm>
#include <iostream>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <poll.h>

#include <getopt.h>

using std::cerr;
using std::endl;

#define NUM_SCREENS 3 

#include "protocol.h"
#include "Keyboard.h"
#include <chrono>


const int refresh_period = 20;
const int mode_change = 5555;
const int screen_update = 5554;
const size_t screen_size = 1024*1024*32;
int sig_fd, sockets[2];
bool swapmouse;
int log2bpp = 3;
int height = 480;
int width = 640;
int screen_fd = -1;
int no_updates = 0;

// Display size
#define SCREEN_WIDTH 2560
#define SCREEN_HEIGHT 1440

void setupTexture();
volatile int draw_pixels=1;
unsigned int *screen[NUM_SCREENS];

typedef struct {
  unsigned int *location;
  unsigned int bpp;
  unsigned int w,h;
} curr_screen_t;

curr_screen_t current_screen;


static unsigned short keycode[256];
static unsigned short shctrl[256];



void init_keyboard(){
  for (int i=0;i<256;i++)
  {
    keycode[i]=0;
    shctrl[i]=0;
  }
  keycode['a']=KeyNo_LetterA;
  keycode['b']=KeyNo_LetterB;
  keycode['c']=KeyNo_LetterC;
  keycode['d']=KeyNo_LetterD;
  keycode['e']=KeyNo_LetterE;
  keycode['f']=KeyNo_LetterF;
  keycode['g']=KeyNo_LetterG;
  keycode['h']=KeyNo_LetterH;
  keycode['i']=KeyNo_LetterI;
  keycode['j']=KeyNo_LetterJ;
  keycode['k']=KeyNo_LetterK;
  keycode['l']=KeyNo_LetterL;
  keycode['m']=KeyNo_LetterM;
  keycode['n']=KeyNo_LetterN;
  keycode['o']=KeyNo_LetterO;
  keycode['p']=KeyNo_LetterP;
  keycode['q']=KeyNo_LetterQ;
  keycode['r']=KeyNo_LetterR;
  keycode['s']=KeyNo_LetterS;
  keycode['t']=KeyNo_LetterT;
  keycode['u']=KeyNo_LetterU;
  keycode['v']=KeyNo_LetterV;
  keycode['w']=KeyNo_LetterW;
  keycode['x']=KeyNo_LetterX;
  keycode['y']=KeyNo_LetterY;
  keycode['z']=KeyNo_LetterZ;
  keycode[' ']=KeyNo_Space;
  
  
  
  
  keycode['1'] = keycode['!'] = KeyNo_Digit1;shctrl['!']=1;
  keycode['2'] = keycode['@'] = KeyNo_Digit2;shctrl['@']=1;
  keycode['3'] = keycode['#'] = KeyNo_Digit3;shctrl['#']=1;
  keycode['4'] = keycode['$'] = KeyNo_Digit4;shctrl['$']=1;
  keycode['5'] = keycode['%'] = KeyNo_Digit5;shctrl['%']=1;
  keycode['6'] = keycode['^'] = KeyNo_Digit6;shctrl['^']=1;
  keycode['7'] = keycode['&'] = KeyNo_Digit7;shctrl['&']=1;
  keycode['8'] = keycode['*'] = KeyNo_Digit8;shctrl['*']=1;
  keycode['9'] = keycode['('] = KeyNo_Digit9;shctrl['(']=1;
  keycode['0'] = keycode[')'] = KeyNo_Digit0;shctrl[')']=1;


  keycode['-'] = keycode['_'] = KeyNo_Minus;shctrl['_']=1;
  keycode['='] = keycode['+'] = KeyNo_Equals;shctrl['+']=1;
  keycode['\\'] = keycode['|'] = KeyNo_BackSlash;shctrl['|']=1;
  keycode['['] = keycode['{'] = KeyNo_OpenSquare;shctrl['{']=1;
  keycode[']'] = keycode['}'] = KeyNo_CloseSquare;shctrl['}']=1;
  keycode[';'] = keycode[':'] = KeyNo_SemiColon;shctrl[';']=1;
  keycode['\''] = keycode['"'] = KeyNo_Tick;shctrl['"']=1;
  keycode[','] = keycode['<'] = KeyNo_Comma;shctrl['<']=1;
  keycode['.'] = keycode['>'] = KeyNo_Dot;shctrl['>']=1;
  keycode['/'] = keycode['?'] = KeyNo_Slash;shctrl['?']=1;
  

  for (int k=0;k<26;k++)
    { keycode[k+'A']=keycode[k+'a'];shctrl[k+'A']=1;
      keycode[k+1]=keycode[k+'a'];}
  keycode['\b']=KeyNo_BackSpace;
  keycode['\r']=KeyNo_Return;
  keycode['\t']=KeyNo_Tab;
  keycode[27] = KeyNo_Escape;
}
static int oldscreennumber=0,screennumber=0,display_size=1;
int display_width=SCREEN_WIDTH,display_height=SCREEN_HEIGHT;
static volatile int buttons=0;
bool shiftkey=false;
// difficult to process shift click...
void report_changed_shift_ctrl_alt_key(){
  static int old_mods;
  int mods=glutGetModifiers();
  report r;
  if (((mods^old_mods)&GLUT_ACTIVE_SHIFT)==GLUT_ACTIVE_SHIFT){
    if(mods&GLUT_ACTIVE_SHIFT){
      r.reason = report::ev_keydown;
      r.key.code = KeyNo_ShiftRight;
      write(sockets[0], &r, sizeof(r));
    } else {
      r.reason = report::ev_keyup;
      r.key.code = KeyNo_ShiftRight;
      write(sockets[0], &r, sizeof(r));
    }
  }
  if (((mods^old_mods)&GLUT_ACTIVE_CTRL)==GLUT_ACTIVE_CTRL){
    if(mods&GLUT_ACTIVE_CTRL){
      r.reason = report::ev_keydown;
      r.key.code = KeyNo_CtrlLeft;
      write(sockets[0], &r, sizeof(r));
    } else {
      r.reason = report::ev_keyup;
      r.key.code = KeyNo_CtrlLeft;
      write(sockets[0], &r, sizeof(r));
    }
  }
  if (((mods^old_mods)&GLUT_ACTIVE_ALT)==GLUT_ACTIVE_ALT){
    if(mods&GLUT_ACTIVE_ALT){
      r.reason = report::ev_keydown;
      r.key.code = KeyNo_AltLeft;
      write(sockets[0], &r, sizeof(r));
    } else {
      r.reason = report::ev_keyup;
      r.key.code = KeyNo_AltLeft;
      write(sockets[0], &r, sizeof(r));
    }
  }
  old_mods=mods;
}
void OnMouseClick(int button, int state, int x, int y)
{
  report r;
   x=x*current_screen.w/display_width;
   y=current_screen.h-y*current_screen.h/display_height;
   buttons=button;
   
   
   report_changed_shift_ctrl_alt_key();
     
   switch (state) {
     case GLUT_DOWN:
        r.reason = report::ev_keydown;
        r.key.code = 0x70 + button;
        write(sockets[0], &r, sizeof(r));
        break;
      case GLUT_UP:
        r.reason = report::ev_keyup;
        r.key.code = 0x70 + button;
        write(sockets[0], &r, sizeof(r));
        break;
     }
}


void My_mouse_routine(int x, int y){
  report r;
  x=x*current_screen.w/display_width;
  y=current_screen.h-y*current_screen.h/display_height;
  report_changed_shift_ctrl_alt_key();
  r.reason = report::ev_mouse;
  r.mouse.x = x;
  r.mouse.y = y;
  r.mouse.buttons = buttons;
  write(sockets[0], &r, sizeof(r));

}

void mouseWheel(int button, int dir, int x, int y)
{
    if (dir > 0)
    {
        // Zoom in
    }
    else
    {
        // Zoom out
    }

    return;
}


void myupfunc(unsigned char key, int x, int y){
    report r;
    r.reason = report::ev_keyup;
    r.key.code = keycode[key];
    write(sockets[0], &r, sizeof(r));    
    report_changed_shift_ctrl_alt_key();
}
void mydownfunc(unsigned char key, int x, int y){
    report r;
    report_changed_shift_ctrl_alt_key();
    r.reason = report::ev_keydown;
    r.reason = report::ev_keydown;
    r.key.code = keycode[key];
    write(sockets[0], &r, sizeof(r));
    if (keycode[key]==0)
      cerr << "Key pressed:" << (unsigned int) key << "\n";
}

unsigned short special_to_code(int key) {
  switch(key) {
    case GLUT_KEY_F1:         return KeyNo_Function1;// F1 function key.
    case GLUT_KEY_F2:         return KeyNo_Function2;// F2 function key.
    case GLUT_KEY_F3:         return KeyNo_Function3;// F3 function key.
    case GLUT_KEY_F4:         return KeyNo_Function4;// F4 function key.
    case GLUT_KEY_F5:         return KeyNo_Function5;// F5 function key.
    case GLUT_KEY_F6:         return KeyNo_Function6;// F6 function key.
    case GLUT_KEY_F7:         return KeyNo_Function7;// F7 function key.
    case GLUT_KEY_F8:         return KeyNo_Function8;// F8 function key.
    case GLUT_KEY_F9:         return KeyNo_Function9;// F9 function key.
    case GLUT_KEY_F10:        return KeyNo_Function10;// F10 function key.
    case GLUT_KEY_F11:        return KeyNo_Function11;// F11 function key.
    case GLUT_KEY_F12:        return KeyNo_Function12; // F12 function key.
    case GLUT_KEY_LEFT:       return KeyNo_CursorLeft; // Left directional key.
    case GLUT_KEY_UP:         return KeyNo_CursorUp;// Up directional key.
    case GLUT_KEY_RIGHT:      return KeyNo_CursorRight;// Right directional key.
    case GLUT_KEY_DOWN:       return KeyNo_CursorDown;// Down directional key.
    case GLUT_KEY_PAGE_UP:    return KeyNo_PageUp; // Page up directional key.
    case GLUT_KEY_PAGE_DOWN:  return KeyNo_PageDown;// Page down directional key.
    case GLUT_KEY_HOME:       return KeyNo_Home;// Home directional key.
    case GLUT_KEY_END:        return KeyNo_Break;// End directional key.
    case GLUT_KEY_INSERT:     return KeyNo_Insert; //Inset directional key.
    default:
      break;
  }
  return 0;
}

void myspecialup(int key, int x, int y)
{
  report r;
  report_changed_shift_ctrl_alt_key();
  r.reason = report::ev_keyup;
  r.key.code = special_to_code(key);
  if (r.key.code!=0)
    write(sockets[0], &r, sizeof(r));
    
}
void myspecialdown(int key, int x, int y)
{
  report r;
  report_changed_shift_ctrl_alt_key();
  r.reason = report::ev_keydown;
  r.key.code = special_to_code(key);
  if (r.key.code!=0)
    write(sockets[0], &r, sizeof(r));
    
}

// Setup Texture
void setupTexture()
{
    // Create a texture 
    glTexImage2D(GL_TEXTURE_2D, 0, 3, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, (GLvoid*)current_screen.location);
 
    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
 
    // Enable textures
    glEnable(GL_TEXTURE_2D);
        draw_pixels=1;
}
 
void updateTexture()
{   
    display_width = glutGet(GLUT_WINDOW_WIDTH);
    display_height = glutGet(GLUT_WINDOW_HEIGHT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, display_width, display_height, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, display_width, display_height);
    // Update Texture
    switch(current_screen.bpp) {
      case 1:
        
        glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, current_screen.w, current_screen.h, GL_RGB,  GL_UNSIGNED_BYTE_3_3_2, (GLvoid*)current_screen.location);
        break;
      case 2:
        glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, current_screen.w, current_screen.h, GL_RGBA,  GL_UNSIGNED_SHORT_1_5_5_5_REV, (GLvoid*)current_screen.location);
        break;
      default:
        glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, current_screen.w, current_screen.h, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, (GLvoid*)current_screen.location);
        break;
    }
 

    glBegin( GL_QUADS );
    
        glTexCoord2d(0.0, 0.0);                                                                glVertex2d(0.0, 0.0);
        glTexCoord2d(1.0*current_screen.w/SCREEN_WIDTH, 0.0);                                  glVertex2d(display_width, 0.0);
        glTexCoord2d(1.0*current_screen.w/SCREEN_WIDTH, 1.0*current_screen.h/SCREEN_HEIGHT);   glVertex2d(display_width, display_height);
        glTexCoord2d(0.0, 1.0*current_screen.h/SCREEN_HEIGHT);                                 glVertex2d(0.0, display_height);

    glEnd();
}
 
void display()
{
  // Clear framebuffer
  //glClear(GL_COLOR_BUFFER_BIT);
  // Draw pixels to texture
  updateTexture();
  // Swap buffers!
  glutSwapBuffers();    
}
 
void reshape_window(GLsizei w, GLsizei h)
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
 
    // Resize quad
    display_width = w;
    display_height = h;
    draw_pixels=1;
    glutPostRedisplay();
}

int init_my_GL(int argc, char **argv) 
{       
    // Setup OpenGL
    glutInit(&argc, argv);          
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
 
    glutInitWindowSize(current_screen.w, current_screen.w);
    glutInitWindowPosition(320, 320);
    glutCreateWindow("RISC OS 5");
 
    glutDisplayFunc(display);
    glutMouseFunc(OnMouseClick);
    glutMotionFunc( My_mouse_routine );
    glutPassiveMotionFunc( My_mouse_routine );   
    glutReshapeFunc(reshape_window);
    
    glutKeyboardUpFunc(myupfunc);
    glutKeyboardFunc(mydownfunc);
    glutMouseWheelFunc(mouseWheel);
    glutSpecialFunc(myspecialdown);
    glutSpecialUpFunc(myspecialup);
    
    setupTexture();

    return 0;
}

inline off_t get_file_size(int fd) {
  struct stat s;
  s.st_size = 0;
  fstat(fd, &s);
  return s.st_size;
}

void *pixels;
auto start = std::chrono::steady_clock::now();
void interact_rule()
{
  
    report r;

    /*{
      struct signalfd_siginfo s;
      while(read(sig_fd, &s, sizeof(s)) > 0);

      int status;
      if (waitpid(pid, &status, WNOHANG) == pid) return WEXITSTATUS(status);
    }*/


      command c;
      int numscr;
      char buf[CMSG_SPACE(sizeof(int)) * 2];

      struct iovec iov = {
        .iov_base = &c,
        .iov_len = sizeof(c),
      };

      struct msghdr msg = {
        msg.msg_control = buf,
        msg.msg_controllen = sizeof(buf),
        msg.msg_iov = &iov,
        msg.msg_iovlen = 1,
      };

      int s = recvmsg(sockets[0], &msg, MSG_DONTWAIT);

      if (s >= 4) {

      for (struct cmsghdr *i = CMSG_FIRSTHDR(&msg); i != NULL; i = CMSG_NXTHDR(&msg, i)) {
        if (i->cmsg_level == SOL_SOCKET && i->cmsg_type == SCM_RIGHTS) {
          close(screen_fd);
          screen_fd = *(int *)CMSG_DATA(i);
          mmap(pixels, screen_size, PROT_READ, MAP_FIXED | MAP_SHARED, screen_fd, 0);
        }
      }

      switch (c.reason) {
        case command::c_mode_change:
          height = c.mode.vidc[11];
          width = c.mode.vidc[5];
          log2bpp = c.mode.vidc[1];
          screennumber = 0;
          cerr << "Set mode " << log2bpp << ' ' << height << ' ' << width << endl;

          switch (log2bpp) {
            case 3:
              display_size = width*height;
              for (numscr=0;numscr<NUM_SCREENS;numscr++) 
                screen[numscr] = (unsigned int*) (pixels + numscr*display_size);
              current_screen.bpp=1;
              break;
            case 4:
              display_size = width*height*2;
              for (numscr=0;numscr<NUM_SCREENS;numscr++)
                screen[numscr] = (unsigned int*) (pixels + numscr*display_size);
              current_screen.bpp=2;
              break;
            case 5:
              display_size = width*height*4;
              for (numscr=0;numscr<NUM_SCREENS;numscr++)
                screen[numscr] = (unsigned int*) (pixels + numscr*display_size);
              current_screen.bpp=4;
              break;
          }
          current_screen.location = (unsigned int*)pixels;
          current_screen.w = width;
          current_screen.h = height;
          if (width==SCREEN_WIDTH && height == SCREEN_HEIGHT)
            glutFullScreen();
          else
            glutReshapeWindow(width, height);
          
          glutPostRedisplay();
          r.reason = report::ev_mode_sync;
          write(sockets[0], &r.reason, sizeof(r.reason));
          break;
        case command::c_activescreen:
          // swap buffer, the buffer number we compute from the offset.
          if ( 3 > ( c.activescreen.address/display_size )) {
            screennumber = c.activescreen.address/display_size;
            current_screen.location = screen[screennumber];
          }
          break;
        case command::c_startscreen:
          // swap buffer, the buffer number we compute from the offset. 
          if ( 3 > ( c.startscreen.address/display_size )) {
            screennumber = c.startscreen.address/display_size;
            // screen swap later..
            
          }
          break;
        case command::c_suspend:
          no_updates = c.suspend.delay;
          //update_screen();
          break;
        case command::c_set_palette: {
          /*
          SDL_Palette *p;
          if (c.palette.type == 0) {
            p = palette;
          } else if (c.palette.type == 2) {
            p = cursor_surface->format->palette;
          } else {
            break;
          }
          p->colors[c.palette.index].r = 0xFF & (c.palette.colour >> 8);
          p->colors[c.palette.index].g = 0xFF & (c.palette.colour >> 16);
          p->colors[c.palette.index].b = 0xFF & (c.palette.colour >> 24);
          if (c.palette.type == 2) goto update_cursor;
          for (numscr=0;numscr<NUM_SCREENS;numscr++)
              SDL_SetSurfacePalette(screen[numscr], palette);
          */
          break;
        }
        case command::c_pointer: {
          /*
          uint8_t *src = c.pointer.data;
          uint8_t *dst = (uint8_t *)cursor_surface->pixels;
          int i = c.pointer.height * 8;
          do {
            int j = 4;
            unsigned s = *src++;
            do {
              *dst++ = s & 3;
              s >>= 2;
            } while (--j);
          } while(--i);
          std::memset(dst, 0, (32 - c.pointer.height) * 32);
          cursor_active_x = c.pointer.active_x;
          cursor_active_y = c.pointer.active_y;
          */
          }
        update_cursor: {
          /*
          SDL_Cursor* cursor2 = SDL_CreateColorCursor(cursor_surface, cursor_active_x, cursor_active_y);
          SDL_SetCursor(cursor2);
          SDL_FreeCursor(cursor);
          cursor = cursor2;
          */
          break;
        }
      }
    }
    auto thismoment = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = thismoment-start;
    if (diff.count() >0.02) {
       display();
       start=thismoment;
     }
}



int main(int argc, char **argv) {

  struct option opts[] = {
    {"chromebook", no_argument, nullptr, 'c'},
    {"swapmouse", no_argument, nullptr, 's'},
    {nullptr, 0, nullptr, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "+cs", opts, nullptr)) != -1) {
    switch (opt) {
      case 's':
        swapmouse = true;
        break;
      case 'c':
        break;
    }
  }

  
  {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    sig_fd = signalfd(-1, &sigset, SFD_CLOEXEC | SFD_NONBLOCK);
  }

  socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets);

  pid_t self = getpid();
  pid_t pid = fork();
  if (!pid) {
    prctl(PR_SET_PDEATHSIG, SIGTERM, 0, 0, 0);
    int socket = fcntl(sockets[1], F_DUPFD, 31); // 31 for compatibilty with early RISC OS
    char s[40];
    std::cerr << "STARTING RISCOS\n";
    sprintf(s, "RISC_OS_SocketKVM_Socket=%i", socket);
    putenv(s);

    sigset_t sigset;
    sigemptyset(&sigset);
    sigprocmask(SIG_SETMASK, &sigset, nullptr);

    if (getppid() == self) execvp(argv[optind], argv + optind);
    _exit(1);
  }
  close(sockets[1]);

  int cursor_active_x = 0;
  int cursor_active_y = 0;

  pixels = mmap(0, screen_size, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  current_screen.location = (unsigned int *)pixels;
  current_screen.bpp=4;
  current_screen.w=1024;
  current_screen.h=600;
  init_keyboard();
  init_my_GL(argc,argv);

  uint32_t buttons = 0;
  glutIdleFunc(interact_rule);
  glutMainLoop();
}