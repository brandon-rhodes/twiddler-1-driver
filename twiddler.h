/*

  Copyright 1996, Massachusetts Institute of Technology. All Rights Reserved.

--------------------------------------------------------------------
Permission to use, copy, or modify this software and its documentation
for educational and research purposes only and without fee is hereby
granted, provided that this copyright notice appear on all copies and
supporting documentation.  For any other uses of this software, in
original or modified form, including but not limited to distribution
in whole or in part, specific prior permission must be obtained from
M.I.T. and the authors.  These programs shall not be used, rewritten,
or adapted as the basis of a commercial software or hardware product
without first obtaining appropriate licenses from M.I.T.  M.I.T. makes
no representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

---------------------------------------------------------------------
*/

/* Twiddler.h file */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>


struct data
{
  int index;
  int bucket;
  int horiz_tilt, vert_tilt;  
};

struct dxdy
{
  int dx;
  int dy;
  struct dxdy *next;
};

typedef struct data DATA;

struct ent
{ 
  int index;
  char* str;
};
typedef struct ent entry;

struct state
{
  DATA last_state,cur_state,last_event;
  int repeat_count; 
  int time_count;
  char flags;
  /* FLAGS FORMAT:
  (LSB) 0  events_enabled 
        1  repeat_enabled
	2  mouse_enabled - not used / global instead
	3  CAPS_LOCK - this acts as if SHIFT was held down for a-z only
        4  NUM_LOCK - this acts as if NUM key is held down for 0-9 only
        5  SCROLL_LOCK - Pause/Unpause tty.
        6  ?
        7  ?
	*/ 
};
typedef struct state *STATE;

/* Function Def's */
void cropline (char **line);
int processbuffer (unsigned char *buffer,STATE events);
void getport (char *arg,char *port);
int open_serial (char *device);
void quit (int signo);
int get_thumb (char *cur_string,char *word,int *index);
int get_keyword (char **cur_string,char *key,int line);
int get_num (char **cur_string,char *key,int line);
int get_string(char **cur_string,char *key,int line);
int parse_line (int *bucket,char *line, char **map,int linenum);
int init(STATE states,int argc,char **argv);
void root_error (void);
int update (void);
int unique();
int read_file (char *filename);
void get_text (STATE events,char *text);
void raise_case (char *line);
void free_table (void);
void init_table (void);
char *search_table(int super_index);
void insert (int bucket,entry *map);
void getXresolution (int *x_res, int *y_res);
void kill_filter (struct dxdy *filter, int filter_level);
struct dxdy *setup_filter (int filter_level);
struct dxdy *filter_mouse (struct dxdy *filter, int filter_level,int *dx, int * dy);
void reinit (int signo);
/* DEFINES */

#define TWID_FILE "/tmp/twid.pid"
#define TWID_TEMP "/tmp/twid.fifo"
#define DEFAULT_FILE "/etc/twid_defaults.ini"
#define DEFAULT_DEVICE "/dev/cua1"
#define MAXLINE 120 
#define CLI_OPTIONS "kf:p:x:"
#define SHIFT_INDEX 0x0001
#define NUM_INDEX   0x0002
#define FUNC_INDEX  0x0004
#define CTRL_INDEX  0x0008
#define ALT_INDEX   0x0010
#define MOUSE_INDEX 0x0020
#define LEFT_MASK   1
#define MID_MASK    2
#define RIGHT_MASK  3

#define M_OFF 0
#define M_FIRST_HIT 1
#define M_ON 2
#define M_2ND_HIT 3

#define LEFT_BUTTON 1
#define MID_BUTTON 2
#define RIGHT_BUTTON 3

#define MOUSE_REVERSE 0x54
#define MOUSE_HIRES 0x30
#define MOUSE_LORES 0x20
#define M_DOUBLE_CLICK 4

#define EVENT_ENABLE 0x01
#define REPEAT_ENABLE 0x02
#define MOUSE_ENABLE 0x04
#define CAPS_LOCK 0x08
#define NUM_LOCK 0x10
#define SCROLL_LOCK 0x20

#define REPEAT_THRESH 15 /* at 2400 baud this is about 1/3 of a second */
#define REPEAT_TIME_THRESH 20

#define SCROLL_ON_INDEX 2113
#define SCROLL_OFF_INDEX 2098
#define BACKSPACE_CODE 8
#define ENTER_CODE 13
#define ESCAPE_CODE 27
#define TAB_CODE 9

/* here's what's mapped in defaults..
Need :
other ctrl's?
*/
#ifdef NEVER_DEFINED
I just wanted to comment all this out 
for reference
DEFAULTS
---------
unsigned char[200] =
{
/* look below at ctrl codes for these...
"0     00L0 = 8", /* 'BS' */
"0     MM0M = 9", /* 'TAB' */
"0     000L = 13", /* 'CR' '\n' */ 
"0     RRR0 = 27", /* 'ESC' */
"0     L000 = 32", /* ' ' */ 
"0     R0L0 = 33", /* '!' */  
"0     R0M0 = 34", /* '"' */   
"0     0M0L = 35", /* '#' */ 
"0     0LR0 = 36", /* '$' */ 
"0     00RM = 37", /* '%' */ 
"0     0L0L = 38", /* '&' */ 
"0     RM00 = 39", /* ''' */ 
"0     0LL0 = 40", /* '(' */ 
"0     0RL0 = 41", /* ')' */ 
"0     00LM = 42", /* '*' */ 
"0     00ML = 43", /* '+' */ 
"0     R0R0 = 44", /* ',' */ 
"0     R00L = 45", /* '-' */ 
"0     RR00 = 46", /* '.' */ 
"0     MMM0 = 47", /* '/' */ 
"NUM   0L00 = 48", /* '0' */ 
"NUM   R000 = 49", /* '1' */ 
"NUM   0R00 = 50", /* '2' */ 
"NUM   00R0 = 51", /* '3' */ 
"NUM   000R = 52", /* '4' */ 
"NUM   M000 = 53", /* '5' */ 
"NUM   0M00 = 54", /* '6' */ 
"NUM   00M0 = 55", /* '7' */ 
"NUM   000M = 56", /* '8' */ 
"NUM   L000 = 57", /* '9' */ 
"0     00RL = 58", /* ':' */ 
"0     R00R = 59", /* ';' */ 
"NUM   M00L = 60", /* '<' */ 
"0     00MR = 61", /* '=' */ 
"NUM   R0R0 = 62", /* '>' */ 
"0     RL00 = 63", /* '?' */ 
"0     00LR = 64", /* '@' */ 
"SHIFT R000 = 65", /* 'A' */ 
"SHIFT 0R00 = 66", /* 'B' */ 
"SHIFT 00R0 = 67", /* 'C' */ 
"SHIFT 000R = 68", /* 'D' */ 
"SHIFT M000 = 69", /* 'E' */ 
"SHIFT 0M00 = 70", /* 'F' */ 
"SHIFT 00M0 = 71", /* 'G' */ 
"SHIFT 000M = 72", /* 'H' */ 
"SHIFT LR00 = 73", /* 'I' */ 
"SHIFT L0R0 = 74", /* 'J' */ 
"SHIFT L00R = 75", /* 'K' */ 
"SHIFT LM00 = 76", /* 'L' */ 
"SHIFT L0M0 = 77", /* 'M' */ 
"SHIFT L00M = 78", /* 'N' */ 
"SHIFT LL00 = 79", /* 'O' */ 
"SHIFT L0L0 = 80", /* 'P' */
"SHIFT L00L = 81", /* 'Q' */ 
"SHIFT MR00 = 82", /* 'R' */ 
"SHIFT M0R0 = 83", /* 'S' */ 
"SHIFT M00R = 84", /* 'T' */ 
"SHIFT MM00 = 85", /* 'U' */ 
"SHIFT M0M0 = 86", /* 'V' */ 
"SHIFT M00M = 87", /* 'W' */ 
"SHIFT ML00 = 88", /* 'X' */ 
"SHIFT M0L0 = 89", /* 'Y' */ 
"SHIFT M00L = 90", /* 'Z' */ 
"NUM   RM00 = 91", /* '[' */ 
"0     LLL0 = 92", /* '\' */ 
"NUM   R00M = 93", /* ']' */ 
"NUM   LM00 = 94", /* '^' */ 
"0     0R0L = 95", /* '_' */ 
"NUM   R0L0 = 96", /* '`' */
"0     R000 = 97", /* 'a' */ 
"0     0R00 = 98", /* 'b' */ 
"0     00R0 = 99", /* 'c' */ 
"0     000R = 100", /* 'd' */
"0     M000 = 101", /* 'e' */
"0     0M00 = 102", /* 'f' */
"0     00M0 = 103", /* 'g' */
"0     000M = 104", /* 'h' */
"0     LR00 = 105", /* 'i' */
"0     L0R0 = 106", /* 'j' */
"0     L00R = 107", /* 'k' */
"0     LM00 = 108", /* 'l' */
"0     L0M0 = 109", /* 'm' */
"0     L00M = 110", /* 'n' */
"0     LL00 = 111", /* 'o' */
"0     L0L0 = 112", /* 'p' */
"0     L00L = 113", /* 'q' */
"0     MR00 = 114", /* 'r' */
"0     M0R0 = 115", /* 's' */
"0     M00R = 116", /* 't' */
"0     MM00 = 117", /* 'u' */
"0     M0M0 = 118", /* 'v' */
"0     M00M = 119", /* 'w' */
"0     ML00 = 120", /* 'x' */
"0     M0L0 = 121", /* 'y' */
"0     M00L = 122", /* 'z' */
"NUM   R00L = 123", /* '{' */
"NUM   LLR0 = 124", /* '|' */
"NUM   LL0R = 125", /* '}' */
"NUM   LLM0 = 126", /* '~' */
"0     0L00 = 127", /* 'DEL' */
/* Special Keys */
"0     0MR0 = 27,91,65", /* arrow up */
"0     0ML0 = 27,91,66", /* arrow down */
"0     0M0M = 27,91,67", /* arrow right */
"0     0MM0 = 27,91,68", /* arrow left */

/* insert 27,91,50,126 
   del 127 or 27,91,51,126
   home 27,91,49,126
   emd 27,91,52,126
pgup 27,91,53,126
pgdn 27,91,54,126
pause 27,91,80
printscr 28
ctrl [ 27
ctrl ] 29

WRITE IN THE LOCK KEYS TOO!
done!

*/
/* F1-F12 */
"FUNC  R000 = 27,91,91,65",
"FUNC  0R00 = 27,91,91,66",
"FUNC  00R0 = 27,91,91,67",
"FUNC  000R = 27,91,91,68",
"FUNC  M000 = 27,91,91,69",
"FUNC  0M00 = 27,91,49,55,126",
"FUNC  00M0 = 27,91,49,56,126",
"FUNC  000M = 27,91,49,57,126",
"FUNC  L000 = 27,91,50,48,126",
"FUNC  0L00 = 27,91,50,49,126",
"FUNC  00L0 = 27,91,50,51,126",
"FUNC  000L = 27,91,50,52,126",



/* Control codes */
"CTRL  R000 = 1", /* 'a' */
"CTRL  0R00 = 2", /* 'b' */
"CTRL  00R0 = 3", /* 'c' */
"CTRL  000R = 4", /* 'd' */
"CTRL  M000 = 5", /* 'e' */
"CTRL  0M00 = 6", /* 'f' */
"CTRL  00M0 = 7", /* 'g' */
"CTRL  000M = 8", /* 'h' */
"CTRL  LR00 = 9", /* 'i' */
"CTRL  L0R0 = 10", /* 'j' */
"CTRL  L00R = 11", /* 'k' */
"CTRL  LM00 = 12", /* 'l' */
"CTRL  L0M0 = 13", /* 'm' */
"CTRL  L00M = 14", /* 'n' */
"CTRL  LL00 = 15", /* 'o' */
"CTRL  L0L0 = 16", /* 'p' */
"CTRL  L00L = 17", /* 'q' */
"CTRL  MR00 = 18", /* 'r' */
"CTRL  M0R0 = 19", /* 's' */
"CTRL  M00R = 20", /* 't' */
"CTRL  MM00 = 21", /* 'u' */
"CTRL  M0M0 = 22", /* 'v' */
"CTRL  M00M = 23", /* 'w' */
"CTRL  ML00 = 24", /* 'x' */
"CTRL  M0L0 = 25", /* 'y' */
"CTRL  M00L = 26", /* 'z' */


/* Built in Macros */
"0     0RR0 = \"the\"",
"0     0RM0 = \"of\"",
"0     0R0R = \"to\"",
"0     0M0R = \"ed\"",
"0     R00M = \"and\"",
"0     0R0M = \"in\"",
"0     0L0M = \"ion\"",
"0     00MM = \"ing\"",
"EOF"
};
#endif












 











