/*
 * File:   xmemUSB.h
 * Author: root
 *
 * Created on July 8, 2013, 4:53 AM
 */

#ifndef XMEMUSB_H
#define	XMEMUSB_H
#include <xmem.h>
#include <Usb.h>
#include <stdio.h>

#ifndef MAX_USB_MODULES
#define MAX_USB_MODULES 10
#endif

#ifndef CONSOLE
#define CONSOLE 0
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
#ifndef XMEM_MULTIPLE_APP
uint8_t USB_ISR_PROTECTED_CALL_START();
uint8_t USB_ISR_PROTECTED_CALL_END();
#define USB_ISR_PROTECTED_CALL() for(char __ToDo = USB_ISR_PROTECTED_CALL_START(); __ToDo;  __ToDo = USB_ISR_PROTECTED_CALL_END())
#endif
#endif	/* XMEMUSB_H */
