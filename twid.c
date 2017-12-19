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
/* Twiddler Driver for Linux 
 * 8/14/95
 * written by:Jeff Levine
 * mouse support added 1/15/96
 * <tornado@media.mit.edu>
 * patch to memory allocation for DEFAULT_FILE added 9/29/97
 * <rhodes@media.mit.edu>
*/

#include "twiddler.h"

/* 98.01.07 Added by DHMSpector <spector@zeitgeist.com> to add capability 
 * to use other serial ports. (e.g., a RocketPort board, like I have)
 */
#define EXTENDED_SERIAL 1

int twidfd,ttyfd,fifo; /* globally known for signal catcher to close */
int xterm_on, mouse_on, mouse_reverse, mouse_res=1;
entry ***table; /* everybody needs to know these two */
unsigned char table_size[32];
FILE *a2x_file = NULL; 
/*Display *theDisplay = NULL;
Window rootwindow;*/
int FILTER_LEVEL;
int X_VT, current_vt;
struct dxdy *mouse_filter;

main (int argc, char ** argv)
{
  int i,j;
  char device[20],port [2],text[MAXLINE],*filename;
  unsigned char byte[1],buffer[5];
  struct state states;
  FILE *temp;
  struct vt_stat vst;
  states.flags = 0;
  if (signal(SIGINT,quit)==SIG_ERR)
    { 
      printf ("Can't get signal SIGINT \n");
      exit (-1);
    }
  /* let's catch some other signals here to close properly */
  if (signal(SIGTERM,quit)==SIG_ERR)
    {
      printf ("Can't get signal SIGTERM \n");
      exit (-1);
    }
  /* initialize state, set defaults, and open ports... all that stuff */
  init (&states,argc,argv);
  unlink (TWID_TEMP);
  /* now we'll enter the read/write loop using 5 byte blocks */
  while (1)
    {
      ttyfd = open ("/dev/console", O_WRONLY); /* this makes sure we are current*/
      ioctl (ttyfd,VT_GETSTATE, &vst);
      close (ttyfd);
      if (current_vt != vst.v_active)
	{
	  current_vt = vst.v_active;
	  if (current_vt == X_VT)
	    {
	      xterm_on = 1;
	      a2x_file = popen  ("a2x -a","w");
	      if (!a2x_file)
		{
		  printf (
			   "Could not Open a2x!! X support will not work!\n");
		}
	    }
	  else
	    {
	      xterm_on = 0;
	      if (a2x_file)
		pclose (a2x_file);
	      a2x_file = NULL;
	    }
	}
      if (i=open("/tmp/twid_signal",O_RDONLY)!=-1)
	{
	  reinit (1);
	  close (i);
	  unlink ("/tmp/twid_signal");
	}
      for (i=0;i<5;i++)
	{
	  while (read (twidfd, buffer+i, 1)<=0);
	  
	  if ((i==0)&&(buffer[0]&0x80))
	    i--; /* we're out of sync... let's keep trying */
	}
#ifdef DEBUG
      printf ("Buffer Contents: ");
      for (i=0;i<5;i++)
	printf ("%x ",buffer[i]);
      printf ("\n");
#endif 
      if (processbuffer (buffer,&states))
	/* if there has been a change in state, or we are in repeat
	   mode, then we will write the buffer contents */
	{
	  if (xterm_on) 
	    {
	      get_text (&states,text);
	      fprintf (a2x_file,"%s",text);
	      fflush (a2x_file);
	      /* write to a2x_file with a2x modifiers */
	    }
	  else
	    {
	      get_text (&states,text);
	      ttyfd = open ("/dev/console",O_WRONLY);
	      for (i=0; i<strlen(text); i++)
		ioctl (ttyfd,TIOCSTI,text+i);
	      close (ttyfd);
	      /* I believe that the above line is Linux specific..
		 any porting would have to take this into account */
	    }
	}
    }
}
/*
void getXresolution (int *x_res, int *y_res )
{
  XWindowAttributes WA;
  if (theDisplay==NULL)
    {
      theDisplay = XOpenDisplay (":0.0");
      rootwindow = DefaultRootWindow (theDisplay);
    }
  XGetWindowAttributes ( theDisplay, rootwindow, &WA);
  
  *x_res = WA.width;
  *y_res = WA.height;
}
*/
int processbuffer (unsigned char *buffer, STATE events)
{
  
  /* returns 1 if new event has been added, 0 otherwise */
  
  /* event enabled upon key press
     event generated upon key release
     unless repeat (then it waits for a time and the generates an event */
  int index=0;
  char CAPS_CODE [8]={27,27,27,27,27,0};
  char NUM_CODE [8]={27,27,27,27,27,1,0};  
  char SCROLL_CODE [8]={27,27,27,27,27,2,0};
  char X_TOGGLE [8]={27,27,27,27,27,3,0}; /* with vt switching I think his is obsolete now */
  char NULL_CODE [8]={27,27,27,27,27,4,0};
  char VT_CODE [8]={27,27,27,27,27,5};
  char CTALDEL_CODE [8]={27,27,27,27,27,6};
  char mouse_but, *temp;
  static int mouse_but_state = M_OFF;
  static int lastvert = 0;
  static int lasthoriz = 0;
  int dx,dy,cx,cy;
  int x_res, y_res;
  index |= ((buffer[0]&0x7f) | ((buffer[1]&0x3f)<<7));
  
  events->last_state = events->cur_state;
  events->cur_state.index = index;
  
  if (events->cur_state.index>events->last_state.index)
    (events->flags |= EVENT_ENABLE);
  
  if (events->time_count<REPEAT_TIME_THRESH)
    events->time_count++;
if (xterm_on)
  {
    /* handle mouse togggle */
    mouse_but = buffer[1]&0x40;
    if (mouse_but)
      {
	switch (index&0x30)
	  {
	  case (MOUSE_HIRES):
	    mouse_res=2;
	    break;
	  case (MOUSE_LORES):
	    mouse_res=1;
	    break;
	  default: break;
	    mouse_res=0;
	  }
	events->cur_state.horiz_tilt = 0 | (((buffer[3]&0x7c)>>2) | 
					    ((buffer[4]&0x07)<<5));
	events->cur_state.vert_tilt = 0 | ((buffer[2]&0x7f) | 
					   ((buffer[3]&0x01)<<7));
	if (!(buffer[3]&0x02))
	  events->cur_state.vert_tilt += 256;
	if (!(buffer[4]&0x08))
	  events->cur_state.horiz_tilt += 256;
	/* flip the high order bit to get offset binary,
	   then just use integer subtract... etc.. */
    
	/* move mouse if need be. */
	/********************************************************************/ 
	/* I used to get the screen size, but now it's not neededc
	/*	getXresolution (&x_res,&y_res);  */
	/********************************************************************/
	/* during get text, we'll use MOUSE as a modifier
	   that really means "in .mouse mode, these are active" */
	cx = -(events->cur_state.horiz_tilt - 256);
	cy = -(events->cur_state.vert_tilt - 256);
	if (mouse_reverse) cy=-cy;
	if (!mouse_on)
	  {
	    kill_filter (mouse_filter,FILTER_LEVEL);
	    mouse_on=1;
	    dx =0;
	    dy=0;
	  }
	else 
	  {
	    dx = cx-lasthoriz;
	    dy = cy-lastvert;
	    mouse_filter = filter_mouse (mouse_filter,FILTER_LEVEL,&dx,&dy);
	  }
	if (dx || dy)
	  {
	    switch (mouse_res)
	      {
	      case (0):
		fprintf (a2x_file, "%c%c%d %d%c",20,4,2*dx,3*dy,20); 
		fflush (a2x_file);
		break;
	      case (1):
		/* LO-RES */
		fprintf (a2x_file, "%c%c%d %d%c",20,4,3*dx,5*dy,20); 
		fflush (a2x_file);
		break;
	      case (2):
		/* X-HI-RES */
		fprintf (a2x_file, "%c%c%d %d%c",20,4,dx,dy,20); 
		fflush (a2x_file);
	      }
	  }
	/* move mouse*/
	lasthoriz = cx;
	lastvert = cy;

	if (!(events->flags&EVENT_ENABLE))
	  return (0); /* if key events not enabled, let's return 0 */
	
	/* check to get special mouse events */
	index &= 0xff;
	if (index==0)
	  {
	    events->flags &= ~EVENT_ENABLE;
	    fprintf (a2x_file,"%c%c0%c",20,2,20);
	    fflush (a2x_file);
	    return (0);
	  }
	if (events->cur_state.index > events->last_state.index)
	  switch (index&0xcf)
	    {
	    case (LEFT_BUTTON):
	      fprintf (a2x_file,"%c%c1%c",20,2,20);
	      fflush(a2x_file);
	      return (0);
	    case (MID_BUTTON):
	      fprintf (a2x_file,"%c%c2%c",20,2,20);
	      fflush (a2x_file);
	      return (0);
	    case (RIGHT_BUTTON):
	      fprintf (a2x_file,"%c%c3%c",20,2,20);
	      fflush(a2x_file);
	      return(0);
	    }

	if (events->cur_state.index < events->last_state.index)
	  {
	    switch (events->last_state.index)
	    {
	    case (MOUSE_REVERSE):
	      events->flags &= ~EVENT_ENABLE;
	      mouse_reverse = 1-mouse_reverse;
	      return (0);
	    default: break;
	      /* we dont have any other keys to be pressed during mouse mode,
		 but here's where normal style "hardwired" key presses 
		 like mouse_reverse would go */
	    }
	  }
	return (0);
      } 
    else 
      {
	fprintf(a2x_file,"%c%c0%c",20,2,20);
	fflush (a2x_file);
	mouse_on = 0;
      }
  }
  
  if (!(events->flags&EVENT_ENABLE))
    return (0); /* if key events not enabled, let's return 0 */
  
  
  /* if the last state was a mapped character and we just released */
  if (events->cur_state.index < events->last_state.index)
    {
      temp = search_table(events->last_state.index);
      /* first let's check for special cases */
      if (temp)
	{
	  if (!strcmp (temp,CAPS_CODE))
	    {
	      events->flags &= ~EVENT_ENABLE;
	      if ((events->flags)&CAPS_LOCK)
		events->flags &= ~(CAPS_LOCK);
	      else
		events->flags |= CAPS_LOCK;
	      return (0); /* not really an event */
	    }
	  if (!strncmp (temp, VT_CODE,6))
	    {	      
	      int term = temp[6];
	      ttyfd = open ("/dev/console",O_RDWR);
	      ioctl (ttyfd, VT_ACTIVATE, term);	   
	      close (ttyfd);
	      /** LINUX SPECIFIC VT SWITCHING **/
	      return (0);
	    }
	  if (!strncmp (temp,CTALDEL_CODE,6))
	    {	      
	      char *shutdown_command=temp+6;
	      system (shutdown_command); 
	      return (0);
	    }
	  if (!strcmp (temp,NUM_CODE))
	    {
	      events->flags &= ~EVENT_ENABLE;
	      if (events->flags&NUM_LOCK)
		events->flags &= ~(NUM_LOCK);
	      else
		events->flags |= NUM_LOCK;
	      return (0); /* not really an event */
	    }      
	  if (!strcmp (temp,X_TOGGLE))
	    {
	      events->flags &= ~EVENT_ENABLE;
	      X_VT = current_vt;
	      return (0); /* not really an event */
	    }
	  if (!strcmp (temp,NULL_CODE))
	    {
	      events->flags &= ~EVENT_ENABLE;
	      if (xterm_on) 
		{
		  fprintf (a2x_file,"%c",0);
		  fflush (a2x_file);
		}
	      else
		{
		  char null=0;
		  ttyfd = open ("/dev/console",O_WRONLY);
		  ioctl (ttyfd,TIOCSTI,&null);
		  close (ttyfd);
		} 
	      return (0); /* not really an event */
	    }
	  if (!strcmp (temp,SCROLL_CODE))
	    {
	      events->flags &= ~EVENT_ENABLE;
	      if (events->flags&SCROLL_LOCK)
		events->flags &= ~(SCROLL_LOCK);
	      else
		events->flags |= SCROLL_LOCK;
	      events->last_event=events->last_state;
	      if (events->flags&SCROLL_LOCK)
		events->last_event.index=SCROLL_ON_INDEX;
	      else
		events->last_event.index=SCROLL_OFF_INDEX;
	      return (1); /* scroll lock produces an event */
	    }
	}
      /* generate event based on the last state */
      if (events->repeat_count>=REPEAT_THRESH)/* we just repeated some stuff */
	events->flags &= ~REPEAT_ENABLE;
      else
	events->flags |= REPEAT_ENABLE; /* else leave repeat option open */
      events->time_count = 0;
      events ->repeat_count=0; 
      events->last_event=events->last_state;
      events->flags &= ~EVENT_ENABLE;
      return (1);
    }

  /* we just pressed something, lets check if we're ready
     to go into repeat mode */

  if ((events->flags&REPEAT_ENABLE) && 
      (events->time_count-events->repeat_count<REPEAT_TIME_THRESH) &&
      (search_table (events->cur_state.index)) &&
      (events->cur_state.index==events->last_event.index))
    events->repeat_count++; /* if state is a repeat state, and not first instance */
  else events->repeat_count=0;
  if (events->repeat_count>REPEAT_THRESH)
    events->repeat_count=REPEAT_THRESH; /*prevents uncontrolled growing
					  if we fall 
					  asleep on the space bar (so to speak :) */
  
  if (events->repeat_count == REPEAT_THRESH)
    {
      /* generate an event based onthe current state */
      events->last_event=events->cur_state;
      return(1);
    }
  return (0);
}

struct dxdy *setup_filter (int filter_level)
{
  int i;  
  struct dxdy *temp;
  struct dxdy *orig;
  orig = calloc (1,sizeof (struct dxdy));
  temp=orig;
  for (i=0; i<(filter_level-1); i++)
    {
      temp->next = calloc (1,sizeof (struct dxdy));
      temp = temp->next;
    }
  temp->next = orig;
  return orig;
}

void kill_filter (struct dxdy *filter, int filter_level)
{
  int i;
  for (i=0; i<filter_level;i++)
    {
      filter->dx=0;
      filter->dy=0;
      filter=filter->next;
    }
}

struct dxdy* filter_mouse (struct dxdy *filter, int filter_level,int *dx, int * dy)
{
  int i;
  filter->dx = *dx;
  filter->dy = *dy;
  
  *dx = 0;
  *dy = 0;
  for (i=0;i<filter_level; i++)
    {
      *dx += filter->dx;
      *dy += filter->dy;
      if (i!=(filter_level-1))
	filter=filter->next;
    }
  *dx /= filter_level;
  *dy /= filter_level;
  return filter;
}

void get_text (STATE events,char *text)
{
  char line2[MAXLINE]="",line[MAXLINE]="", *temp;
  int i;
  
  *text=0;
  temp = search_table(events->last_event.index);
  if (temp==NULL)
    {
      if (xterm_on)  /* in X if we haven't found an entry.. try
			appending with the correct modifier */
	{
	  /* a2x will accept ctrl, alt, and shift modifiers, so if we have nothing
	     else defined, we can put these modifiers in our text*/
	  
	  if (events->last_event.index &(CTRL_INDEX<<8))
	    sprintf  (line,"%s%c%c",line,20,3);
	  if (events->last_event.index &(ALT_INDEX<<8))
	    sprintf (line,"%s%c%c",line,20,13);
	  if (events->last_event.index &(SHIFT_INDEX<<8))
	    sprintf (line,"%s%c%c",line,20,19);
	  temp =search_table(events->last_event.index&0xff);
	  if (temp)
	    {
	      strcat (line,temp);
	      strcpy (text,line);
	    }
	}
      return;
    }
  strcpy (line,temp);
  if (events->flags&CAPS_LOCK)
    {
      for (i=0;i<strlen(line);i++)
	if ((line[i]>='a') && (line[i]<='z'))
	  line[i] = line[i]-('a'-'A');
    }
  if (events->flags&NUM_LOCK)
    {
      if (search_table(events->last_event.index |(NUM_INDEX<<8)))
	{
	  strcpy (line2,search_table(events->last_event.index |(NUM_INDEX<<8)));
	  if ((strlen(line2)==1) && /* works only for single characters */
	      ((*line2>='0') && (*line2<='9')))
	    strcpy (line,line2);
	}
    }
  /* scroll lock has no instantanious function... */
  strcpy (text,line);
  return;
}

void free_table()
{
  int i,j;
  for (i=0;i<32;i++)
  {
    for (j=0;j<table_size[i];j++)
      free (table[i][j]);
    free(table[i]);
    table_size[i]=0;
  }
  free(table);
}
  
void init_table()
{
  int i;
  table = (entry ***) calloc (32,sizeof(entry **));
  
  for (i=0;i<32;i++)
    table[i]=calloc(1,sizeof(entry *));
  for (i=0;i<32;i++)
    table_size[i]=0;
}

char *search_table  (int super_index)
{
  int index,bucket,temp;
  int l,m,h;

  index=super_index & 0x00ff;
  bucket = (super_index & 0x1f00)>>8;
  
  l=0; h=table_size[bucket]-1;
  
  while (l<=h)
    {
      m=(h+l)/2;
      temp=table[bucket][m]->index;
      if (temp==index)
	return (table[bucket][m]->str);
      if (temp>index)
	h=m-1;
      else
	l=m+1;
    }
  return NULL;
} 
int open_serial (char *device)
     /* opens serial port to 2400,8N1 */
{
  struct termios term;
  int fd;
  
#ifdef DEBUG  
  printf ("Opening %s...\n",device);
#endif

  if ((fd = open (device , O_RDONLY))<0)
    return (-1);
  
  if (!isatty(fd))
    {
      printf ("This is not a TTY device!\n");
      return (-1);
    }
  
  if ((tcgetattr (fd ,&term))<0)
    {
      printf ("%d - ",errno);
      printf ("Couldn't get tty info!\n");
      return (-1);
    }
  term.c_cflag &= ~CBAUD; /* init baud rate */
  term.c_cflag |= B2400;
  tcsetattr (fd,TCSAFLUSH, &term);
  tcflush (fd, TCIOFLUSH);
  
  if ((tcgetattr (fd ,&term))<0)
    {
      printf ("%d - ",errno);
      printf ("Couldn't get tty info!\n");
      return (-1);
    }
  /* put in raw data mode */
  term.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
  term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  term.c_cflag &= ~(CSIZE | PARENB);
  term.c_oflag &= ~OPOST;
  term.c_cflag |= CS8; /* set to 8 stop bits */
  term.c_cflag |= CLOCAL;
  term.c_cc[VMIN]=1;
  term.c_cc[VTIME]=0;

  term.c_cflag &= ~CBAUD; /* drops DTR without ioctl */

  tcsetattr (fd,TCSAFLUSH, &term);
  tcflush (fd, TCIOFLUSH);
  tcflow (fd,TCOOFF);
  return (fd);
}

int init(STATE states,int argc,char **argv)
{
  char opt,device[10]=DEFAULT_DEVICE,line[MAXLINE];
  FILE *pidfile;
  int i,pid,initialize=0;
  char *filename;
  
  /* let's see if we should even try */
  
  if (unique() && (getuid()>0)) root_error();
  
  if (!(filename = getenv("TWID_DEFAULTS")))
    {
      filename = calloc (strlen(DEFAULT_FILE)+1,sizeof (char));
      strcpy (filename , DEFAULT_FILE);
    }
  init_table();
  FILTER_LEVEL = 3;
  X_VT = 7;
  /* let's see what the user has in store for us */
  while ((opt=getopt(argc,argv,CLI_OPTIONS))!=-1)
    {
      switch (opt)
	{   
	case 'k':
	  if (getuid()>0)
	    root_error();
	  if (pidfile = fopen (TWID_FILE,"r"))
	    {
	      fscanf(pidfile,"%d",&pid);
	      if (kill (pid,SIGTERM)<0)
		{
		  printf ("Couldn't kill Twiddler!\n");
		  exit (-1);
		}	 
	      fclose (pidfile);
	    }
	  exit (-1);
	case 'x':
	  X_VT = atoi (optarg);
	  break;
	case 'p':
	  if (getuid()>0)
	    root_error();
#ifndef  EXTENDED_SERIAL
	  strcpy (device,"/dev/cua");
	  strncat(device,optarg,1);
#else
        strcpy (device,"/dev/");
        strncat(device,optarg,strlen(optarg));          
#endif
	  break;
	case 'f':
	  if (!unique())
	    {
	      if (pidfile = fopen (TWID_FILE,"r"))
		{
		  fscanf(pidfile,"%d",&pid);
		  fclose (pidfile);
		  pidfile = fopen (TWID_TEMP,"w"); /* actually a temp file*/
		  fprintf (pidfile,"%s",optarg);
		  fclose(pidfile);
		  system("touch /tmp/twid_signal");
		} 
	      exit (1);
	    }
	  else
	    {
	      /*  this is it! let's just get it */
	      if (read_file(filename)==-1)
		{
		  printf ("Twiddler Keymap: Error reading default file %s !!\n",filename);
		  printf ("Check that file exists and try again\n");
		  exit (-1);
		}
	      if (read_file(optarg)==-1)
		{
		  fprintf (stderr,"Twiddler Keymap: Error reading file %s!\n",optarg);
		  fprintf (stderr,"Additions will not be active...\n");
		}

	      initialize=1;
	    }
	  break;
	default: break;
	  printf ("Invalid argument...\n");
#ifndef EXTENDED_SERIAL
	  printf ("Usage: twid [-p #] [-k] [-f filename] [-x #]\n");
	  printf ("Where -p # is a number from 0 to 3\n");
	  printf ("corresponding to cua0-cua3 (COM1-COM4 in DOS)\n");
#else
        printf ("Usage: twid [-p dev] [-k] [-f filename] [-x #]\n");
        printf ("Where 'dev' is a device name, e.g. \"ttyR7\"\n");
#endif
	  printf ("-x # is a number from 1 to MAX_NR_CONSOLES\n");
	  printf ("-k will kill the driver completely\n");
	  printf ("Note: you must be root to execute any option besides -f\n");
	  printf ("Calling with no arguments resets the defaults\n");
	  quit (SIGQUIT);     
	};
    }
  if (!initialize)
    if (unique())
      {
	if (read_file(filename)==-1)
	  {
	    printf ("Twiddler Keymap: Error reading default file %s !!\n",filename);
	    printf ("Check that file exists and try again\n");
	    exit (-1);
	  }
      }
    else
      {
	if (pidfile = fopen (TWID_FILE,"r"))
	  {
	    fscanf(pidfile,"%d",&pid);
	    fclose (pidfile);
	    pidfile = fopen (TWID_TEMP,"w");
	    fprintf (pidfile ,"%s",filename);
	    fclose(pidfile);
	    system ("touch /tmp/twid_signal"); 
	    /* send signal to main program 
	       alerting it that we want to change
	       defaults*/
	  }	
	exit (1);
      }

  #ifdef DEBUG2
  {
    int i,j;
    for (i=0;i<32;i++)
      {
	if (table_size[i])
	  {
	    printf ("BUCKET %d SIZE: %d\n",i,table_size[i]);
	    for (j=0;j<table_size[i];j++)
	      printf ("Bucket: %d Elem: %d Map: %s\n",i,table[i][j]->index,table[i][j]->str);
	  }
	else printf ("Bucket %d empty\n",i);
      }
    quit(SIGTERM); /*lets not process, just print out table */
    exit (1);
  }
  #endif
  /* OK, now here we are unique and root */
#ifndef GDB  /* don't fork under debugger */
  switch (fork())
    {
    case -1: 
      printf ("Fork Error... Exiting\n");
      exit(-1);
    case 0: 
      break;
    default: exit(0);
    }
  if (setsid()<0) 
    {
      printf ("setsid error.. Exiting\n");
      exit (-1);
    }
  umask (0); /* just make sure we don't have anything funny */
#endif
  /* by now we've forked and killed papa, lets fire it up */

  /* inital states */
  states->cur_state.index=0;
  states->last_event.index=-1;
  states->last_state.index=0;

  /* OK, we're running for good, let's lock */
  /* yes this is not a great locking system or anything */
  /* but it keeps the casual user from duplicating by accident */

  unlink (TWID_FILE);
  if (!(pidfile = fopen (TWID_FILE,"w")))
    {
      printf ("Couldn't create file %s Exiting.. \n ",TWID_FILE);
      quit (SIGQUIT);
    }
  fprintf (pidfile,"%d",getpid());
  fclose (pidfile);
  /* time to open the port */
  mouse_filter = setup_filter (FILTER_LEVEL);
  twidfd = open_serial (device);
  if (twidfd<0)
    {
      printf ("Couldn't open Serial Port %s!\n",device);
      exit (-1);
    }


#ifdef DEBUG
  printf ("All ports opened...\n");
#endif
  
  /* OK, DONE! */
}

int unique()
{
  FILE *fd;
  char cmdline[50],pid[8],file[30];
  
  /* yup, I know this doesn't mean uniqueness but it probably does */
  /* so that's good enough for now */
  if (!(fd=fopen (TWID_FILE,"r")))
    {
      return(1);
    }
  /* let's try to clean up a stray twid.pid... */
  fscanf (fd,"%s", pid);
  fclose (fd);
  strcpy(file,"/proc/");
  strcat(file,pid);
  strcat(file,"/cmdline");
  if (!(fd=fopen(file,"r")))
    {
      unlink (TWID_FILE); /* there is no process running, erase .pid file */
      return (1);
    }

  fscanf (fd,"%s",cmdline);
  fclose (fd);
  if (strstr(cmdline,"twiddler")) /* finds a substring */
    return (0); /* twiddler is definately running! */
  return (1); 
/* twid.pid does not name a pid that is running a file called "twiddler" */
}

int read_file (char *filename)
{
  int linenum,index,i,bucket=0;
  char line[MAXLINE], *map;
  FILE *user;
  entry *mapping;
  
  linenum=1;
  if (!(user = fopen (filename,"r")))
    return (-1);
  while (fgets(line,MAXLINE,user))
    {
      mapping = calloc (1,sizeof(entry));
      if ((mapping->index=parse_line(&bucket,line,&map,linenum))>=0)
	{
	  mapping->str = calloc (strlen(map)+1,sizeof(char));
	  strcpy (mapping->str, map);
	  table[bucket] = realloc (table[bucket],
				   sizeof(entry *)*(table_size[bucket]+1));
	  insert (bucket,mapping);
	} 
      else
	free (mapping); /*line was bad - kill memory for it */
	
      linenum++;
    }
  fclose (user);
  return (1);
}

void insert (int bucket,entry *map)
{
  int i,j;
  entry *temp,*temp2;
  
  for (i=0;((i<table_size[bucket])&&(table[bucket][i]->index<map->index));i++);
  if (i==table_size[bucket])
    {
      table[bucket][i]=map;
      table_size[bucket]++;
      return;
    }  
  if (table[bucket][i]->index==map->index)
    {
      free (table[bucket][i]);
      table[bucket][i]=map;
    }
  else
    {
      for (j=i,temp=map;j<table_size[bucket];j++)
	{
	  temp2=table[bucket][j];
	  table[bucket][j]=temp;
	  temp=temp2;
	}
      table[bucket][j]=temp;
      table_size[bucket]++;
    }
}

int parse_line (int *bucket,char *line, char **map,int linenum)
{
  char cur_string[MAXLINE],tempmap[MAXLINE],key[MAXLINE],thumb[MAXLINE];
  int mapplace=0,index=0,i,t_index;
 
  /* format is :
     <thumb>+<thumb>... <four character code> = "string to translate"
     note the quotes, up to five entries (MOUSE is reserved) for thumb
     separated by a '+' */

  if (strlen(line)<2)
    return (-1);
  (*bucket)=0;
  t_index=0;
  raise_case (line);
  sscanf(line,"%s",thumb);
  cropline (&line);
  
  do
    {
      i=get_thumb (thumb,cur_string,&t_index);
      if (!strcmp (cur_string, "CTRL"))
	(*bucket) |= CTRL_INDEX;
      else
	if (!strcmp(cur_string, "ALT"))
	  (*bucket) |= ALT_INDEX;
	else
	  if (!strcmp(cur_string, "NUM"))
	    (*bucket) |= NUM_INDEX;
	  else
	    if (!strcmp(cur_string, "FUNC"))
	      (*bucket) |= FUNC_INDEX;
	    else
	      if (!strcmp(cur_string, "SHIFT"))
		(*bucket) |= SHIFT_INDEX;
	      else
		if (!strcmp(cur_string, "0"));
		else
		  {
		    printf ("Error in config file, line %d: %s not a valid thumb button keyword\n",linenum,cur_string);
		    return (-1);
		  }
    } while (i);
  sscanf(line,"%s",cur_string);
  cropline (&line);
  if (strlen (cur_string)!=4)
    {
      printf ("Error in config file, line %d: Finger field %s is invalid\n",linenum,cur_string);
      return(-1);
    }
  else
    for (i=0;i<4; i++)
      {
	switch (cur_string[i])
	  {
	  case 'L':
	    index += LEFT_MASK << (2*i);
	    break;
	  case 'M': 
	    index += MID_MASK << (2*i);
	    break;
	  case 'R':
	    index += RIGHT_MASK << (2*i);
	    break;
	  case '0':
	    break;
	  default: break;
	  printf ("Error in config file,line %d: Finger field %s is invalid\n",linenum,cur_string);
	  return (-1);
	  }
      }
  
  sscanf(line,"%s",cur_string);
  cropline (&line);
  
  if ((strlen(cur_string)!=1) || (cur_string[0]!='='))
    {
      printf ("Error in config file, line %d: Equal sign Missing\n",linenum);
      return(-1);
    }
  /* three ways to get stuff: keywords, numbers, and strings */
  while ((line[0]==' ')||(line[0]=='\t'))
    line++;
  while (line[0]&&line[0]!='\n')
    {
      if (line[0]=='\"') 
	{ 
	  if (!get_string (&line,key,linenum)) return (-1); 
	}
      else if ((line[0]>='0') && (line[0] <='9'))
	{ 
	  if (!get_num (&line,key,linenum)) return (-1); 
	}
      else 
	{ 
	  if (!get_keyword(&line,key,linenum)) return (-1);
	}
      strcpy (tempmap+mapplace,key);
      mapplace += strlen(key);
      while ((line[0]==' ')||(line[0]=='\t'))
	line++;
    }
  *map = calloc (strlen(tempmap)+1,sizeof(char));
  strcpy (*map,tempmap);
  return (index);
}


void raise_case (char *line)
{
int i,quote=0;
/* puts the string to all CAPS except what's in quotes */
for (i=0; i<(strlen(line)) ; i++)
  {
    if (!quote)
      {
	if (line[i]=='\"')
	  quote=1;
	else if  ((line[i]>='a')&&(line[i]<='z'))
	  line [i] += ('A'-'a');
      }
    else
      if (line[i]=='\"')
	quote=0;
  }
}

void cropline (char **line)
{
  /* gets rid of all whitespace, then one word (until next whitespace) */
  int flag=0;
  
  while (!(flag&&((**line==' ') || (**line=='\t'))))
    {
      if ((!flag) && ((**line != ' ') && (**line != '\t')))
	flag =1;
      (*line)++;
    }
}

int get_thumb (char *thumb,char *word,int *index)
{
  int begin=*index,i;

  while ((thumb[*index]!='\0') && (thumb[*index]!='+') &&
	 (thumb[*index]!='\t') && (thumb[*index]!=' '))
    (*index)++;
  
  for (i=0;i<10;i++)
    word[i]=0;
  for (i=begin;i<*index;i++)
    word[i-begin]=thumb[i];
  
  if (thumb[*index]=='+')
    {
      (*index)++;
      return (1);
    }
  else return (0);
}

int get_string(char** cur_string,char *key,int line)
{
  int index=0;

  (*cur_string)++; 
  while (**cur_string!='\"')
    {
      if (**cur_string=='\0') return (0);
      key[index]=**cur_string;
      index ++; (*cur_string)++;
    }
  key[index]='\0';
  (*cur_string)++;
  /* get rid of , */
  while ((**cur_string==' ')||(**cur_string=='\t'))
    (*cur_string)++;
  if (**cur_string==',')
   {
     (*cur_string)++;
     return (1);
   }
  if ((**cur_string=='\0')||(**cur_string=='\n'))
    return (1);
  printf ("Error in config file, line %d: String argument invalid\n",line);
  return(0);

}

int get_num (char** cur_string,char *key,int line)
{
  int num=0;
 
  while ((**cur_string>='0') && (**cur_string <='9'))
    {
      num *= 10;
      num += **cur_string-'0';
      (*cur_string)++;
    }
  key[0]=num;
  key[1]='\0';
  while ((**cur_string==' ')||(**cur_string=='\t'))
    (*cur_string)++;
  if (**cur_string==',')
    {
      (*cur_string)++;
      return (1);
    }
  if ((**cur_string=='\0')||(**cur_string=='\n'))
    return (1);
  printf ("Error in config file, line %d: Number argument invalid\n",line);
  return(0);
}

int get_keyword (char **cur_string, char *key,int line)
{
  int index,i;
  char word[20]="";
  index=0;

  while (((*cur_string)[index]!='\0') && ((*cur_string)[index]!=',') &&
	 ((*cur_string)[index]!='\t') && ((*cur_string)[index]!=' ') &&
	 ((*cur_string)[index]!='\n'))
    index++;
 
  strncpy (word,*cur_string,index);
  
  if (((*cur_string)[index]=='\0')||((*cur_string)[index]=='\n'))
    *cur_string+=index;
  else *cur_string+=index+1;
  while ((**cur_string==' ')||(**cur_string=='\t'))
    (*cur_string)++; 
  if (!strcmp (word,"BACKSPACE"))
    {
      key[0] = BACKSPACE_CODE;
      key[1]='\0';
      return (1);
    }
  if (!strcmp (word,"ENTER"))
    {
      key[0] = ENTER_CODE;
      key[1]='\0';
      return (1);
    }
  if (!strcmp (word, "CTRLALTDEL"))
   { 
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=6;
      key[6]='\0';
      return (1);
    }
  if (!strcmp (word,"ESCAPE"))
    {
      key[0] = ESCAPE_CODE;
      key[1]='\0';
      return (1);
    }
  if (!strcmp (word,"TAB"))
    {
      key[0] = TAB_CODE;
      key[1]='\0';
      return (1);
    }
  if (!strcmp (word,"NULL"))
    {
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=4;
      key[6]='\0';
      return (1);
    }
  if (!strcmp (word,"CAPS_LOCK"))
    {
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=0;
      key[6]='\0';
      return (1);
    }
  if (!strcmp (word,"SCROLL_LOCK"))
    {
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=2;
      key[6]='\0';
      return (1);
    }
  if (!strcmp (word,"VT"))
    {
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=5;
      key[6]='\0';
      return (1);
    }
  if (!strcmp (word,"NUM_LOCK"))
    {
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=1;
      key[6]='\0';
      return (1);
    }
  if (!strcmp (word,"X_TOGGLE"))
    {
      for (i=0;i<=4;i++) key[i]=27;
      key[5]=3;
      key[6]='\0';
      return (1);
    }
  
  printf ("Error in config file, line %d: %s is an Invalid Keyword\n",line,word);
  return (0);
}

void quit (int signo)
{
  if (signo==SIGQUIT)
    exit (0);
  close (twidfd);
  close (ttyfd);
  free_table();
  if (a2x_file)
    pclose (a2x_file);
  unlink (TWID_FILE);
  printf ("Twiddler Driver Killed..\n");
  exit (1);
}

void root_error ()
{
  printf ("It's not nice to try that if you aren't root!\n");
  quit(SIGQUIT);
}

void reinit (int signo)
{
  FILE *temp;
  char text [256];
  char *filename;

  if (!(filename = getenv ("TWID_DEFAULTS")))
    {
      filename = calloc (strlen(DEFAULT_FILE)+1,sizeof(char ));
      strcpy (filename , DEFAULT_FILE);
    }
  if (temp=fopen (TWID_TEMP,"r"))
    {
      fscanf (temp,"%s",text);
      fclose (temp);
      unlink (TWID_TEMP);
      free_table();
      init_table();
      if (read_file(filename)==-1)
	{
	  fprintf (stderr,"Twiddler Keymap: Error reading default file %s !!\n",filename);
	  fprintf (stderr,"Check that file exists and try again\n");
	  quit (SIGTERM);
	}
      fprintf (stdout,"\nTwiddler Defaults restored.\n");
      fflush (stdout);
      if (strcmp(text,filename))
	{
	  if (read_file(text)==-1)
	    {
	      fprintf (stderr,"Twiddler Keymap: Error reading file %s!\n",text);
	      fprintf (stderr,"Additions will not be active...\n");
	    }
	}
    }
  else
    {
      fprintf (stderr,"Could not open message file\n");
    }
  return;
}







