
#define DISK_READ_OVERHEAD 50  /* 50 ms read overhead */

#define BLOCK_READ_TIME 1 /* 1 ms per disk block */

/* The disk read takes an overhead of 50ms + (1ms * size of data) */

/* This is the driver routine to call to issue a
   (blocking) disk read request. An interrupt will automatically
   occur when the read has completed */

extern void disk_read_req(PID_type pid, int size); 

/* For our purposes, the reading the keyboard
   buffer takes 100 milliseconds */

#define KEYBOARD_READ_OVERHEAD 100

/* This is the driver routine to call to issue a
   (blocking) keyboard read. An interrupt will automatically
   occur when the read has completed */

extern void keyboard_read_req(PID_type pid);


/* This is the driver routine to call to issue a
   (non-blocking) disk write. It returns immediately
   (i.e. the write is assumed to take 0 time). */

extern void disk_write_req(PID_type pid);


