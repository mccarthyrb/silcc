/************************************************************************/
/* keybrd.h		Prototypes for functions in keyboard library			*/
/************************************************************************/
#if defined(WIN32) || defined(UNIX)
#undef __far
#define __far
#endif
 
/* Function prototypes */
/* ctrlc.asm */
void __far ctrlc_off(void);		/* Disables normal ^C and ^break vectors */
void __far ctrlc_on(void);		/* Re-enables normal ^C and ^break vectors */
int __far ctrlc(void);			/* Returns TRUE if a ^C or ^break has been */
							/* pressed since the last call to ctrlc_off() */
							/* or ctrlc() */

/* break.asm */
int __far break_flag(int);	/* Set MS-DOS break flag, and returns original */
							/* setting. 0 for OFF, 1 for ON */

/* getkey.asm */
int __far get_key(void);	    /* Reads keystroke, does not strip scan code */
int __far get_keystroke(void);	    /* Reads keystroke, strips scan code */
int __far key_waiting(void);	    /* Returns TRUE if a keystroke is waiting */
int __far kbd_status(void);	    /* Returns the keyboard status byte */
int __far ext_get_key(void);	    /* Read extended keystroke, does not strip scan code */
int __far ext_get_keystroke(void);  /* Reads extended keystroke, strips scan code */
int __far ext_key_waiting(void);    /* Returns TRUE if a keystroke is waiting */
int __far ext_kbd_status(void);     /* Returns the extended keyboard status word */

/* END */
