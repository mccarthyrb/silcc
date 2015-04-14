#include <conio.h>
#include "keybrd.h"

int __far get_keystroke(void)	    /* Reads keystroke, strips scan code */
{
	return getch();
}

int __far key_waiting(void)	    /* Returns TRUE if a keystroke is waiting */
{
	return kbhit();
}

int __far break_flag(int flag)	/* Set MS-DOS break flag, and returns original */
{
	return 0;
}