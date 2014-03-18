#include <xmem.h>
#include <xmemUSB.h>
#include <Usb.h>
#include <stdio.h>

extern "C" unsigned int freeHeap();

#define MAX_USB_MODULES 10
extern USB Usb;

volatile USB_Module_Calls USB_Module[MAX_USB_MODULES + 1] = {NULL};

// USB task

void USB_main(void) {

        uint8_t last_state = 1;
        uint8_t current_state;
        uint8_t i = 0;
        while (USB_Module[i]) {
                if (!USB_Module[i](1, 0, 0)) return; // Runs Init
                i++;
        }

        while (Usb.Init(1000) == -1);

        for (;;) {
                Usb.Task();
                current_state = Usb.getUsbTaskState();
                i = 0;
                while (USB_Module[i]) {
                        if (!USB_Module[i](2, current_state, last_state)) xmem::Yield(); //Poll, sleep if work is done
                        i++;
                }
                last_state = current_state;

        }
}

static FILE tty_stdio;
static FILE tty_stderr;
static uint8_t PID_of_USB;

// Fun! Serial stuff is thread safe!

static int tty_stderr_putc(char c, FILE *t) {
        if(c == '\n') USB_HOST_SERIAL.write('\r');
        USB_HOST_SERIAL.write(c);
}

static int tty_std_putc(char c, FILE *t) {
        if(c == '\n') USB_HOST_SERIAL.write('\r');
        KONSOLE.write(c);

}

static int tty_std_getc(FILE *t) {
        while (!KONSOLE.available());
        return KONSOLE.read();
}

void USB_Setup(USB_Module_Calls func[]) {

        // Set up stdio/stderr
        tty_stdio.put = tty_std_putc;
        tty_stdio.get = tty_std_getc;
        tty_stdio.flags = _FDEV_SETUP_RW;
        tty_stdio.udata = 0;
        stdout = &tty_stdio;
        stdin = &tty_stdio;

        tty_stderr.put = tty_stderr_putc;
        tty_stderr.get = NULL;
        tty_stderr.flags = _FDEV_SETUP_WRITE;
        tty_stderr.udata = 0;
        stderr = &tty_stderr;

//#if CONSOLE == 0
//        // Ensure TX is off
//        _SFR_BYTE(UCSR0B) &= ~_BV(TXEN0);
//#endif
//        USB_HOST_SERIAL.begin(CONSOLE_BAUD_RATE);
//        // Do not start primary Serial port if already started.
//#if CONSOLE == 0
//        if (bit_is_clear(UCSR0B, TXEN0)) {
//#endif
                KONSOLE.begin(CONSOLE_BAUD_RATE);
//#if CONSOLE == 0
//        }
//#endif
#if USB_HOST_SERIAL_NUM != CONSOLE
USB_HOST_SERIAL.begin(CONSOLE_BAUD_RATE);
#endif

        uint8_t i = 0;
        while (func[i]) {
                USB_Module[i] = func[i];
                if (!USB_Module[i](0, 0, 0)) return; // Runs Setup on each
                i++;
                if (i == MAX_USB_MODULES) {
                        break;
                }
        }
        USB_Module[i] = NULL;
        PID_of_USB = xmem::SetupTask(USB_main
#ifdef _MAX_SS
#if _MAX_SS != 512
                , 0x1000 /* adjust stack 1000 4k, c00 3k, 800 2k*/
#endif
#endif
                );
        if (PID_of_USB) xmem::StartTask(PID_of_USB);
}

