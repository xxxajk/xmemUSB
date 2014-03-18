/*
 * File:   xmemUSB.h
 * Author: root
 *
 * Created on July 8, 2013, 4:53 AM
 */

#ifndef XMEMUSB_H
#define	XMEMUSB_H

#ifndef CONSOLE
#define CONSOLE 1
#endif
#ifndef CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 115200
#endif
#ifndef USB_HOST_SERIAL_NUM
#define USB_HOST_SERIAL_NUM 1
#endif
#if CONSOLE
#define SET_KON(y) MAKE_KON(Serial, y)
#define MAKE_KON(x, y) x ## y
#define KONSOLE SET_KON(CONSOLE)
#else
#define KONSOLE Serial
#endif


typedef int (*USB_Module_Calls) (uint8_t, uint8_t, uint8_t);
extern void USB_Setup(USB_Module_Calls func[]);
extern void USB_main(void);


#endif	/* XMEMUSB_H */

