#include <xmemUSB.h>

extern USB Usb;

volatile USB_Module_Calls USB_Module[MAX_USB_MODULES + 1] = {NULL};

#if defined(__AVR__)
extern "C" unsigned int freeHeap();
static FILE tty_stdio;
static FILE tty_stderr;
static uint8_t PID_of_USB;
#elif defined(__arm__) && defined(CORE_TEENSY)
#include <sys/stat.h>
#define USB_TASK_INTERVAL 10000
static IntervalTimer USB_timed_isr;
static uint8_t last_state = 1;
static uint8_t current_state;

// USB task isr

void Do_USB(void) {
        USB_ISR_PROTECTED_CALL() {
                int i;
                // These can take a while...
                Usb.Task();
                current_state = Usb.getUsbTaskState();
                i = 0;
                while(USB_Module[i]) {
                        USB_Module[i](2, current_state, last_state); // Runs each module
                        i++;
                }
                last_state = current_state;
        }

}

uint8_t USB_ISR_PROTECTED_CALL_START() {
        USB_timed_isr.end();
        return 1;
}

uint8_t USB_ISR_PROTECTED_CALL_END() {
        USB_timed_isr.begin(Do_USB, USB_TASK_INTERVAL);
        return 0;
}


#endif

// USB task for avr, setup-completion for teensy 3.x

void USB_main(void) {
        uint8_t i = 0;
#if  defined(__arm__) && defined(CORE_TEENSY)

        USB_ISR_PROTECTED_CALL() {
#endif
                while(USB_Module[i]) {
                        if(!USB_Module[i](1, 0, 0)) return; // Runs Init on each
                        i++;
                }

                while(Usb.Init(1000) == -1);
#if  defined(__arm__) && defined(CORE_TEENSY)
        //                USB_timed_isr.begin(Do_USB, USB_TASK_INTERVAL); // .01 second

        }
#else
                for(;;) {
                        Usb.Task();
                        current_state = Usb.getUsbTaskState();
                        i = 0;
                        while(USB_Module[i]) {
                                if(!USB_Module[i](2, current_state, last_state)) xmem::Yield(); //Poll, sleep if work is done
                                i++;
                        }
                        last_state = current_state;

                }
#endif
}

#if defined(__AVR__)

// Fun! Serial stuff is thread safe!

static int tty_stderr_putc(char c, FILE *t) {
        if(c == '\n') USB_HOST_SERIAL.write('\r');
        USB_HOST_SERIAL.write(c);
}

static int tty_std_putc(char c, FILE *t) {
        if(c == '\n') KONSOLE.write('\r');
        KONSOLE.write(c);

}

static int tty_std_getc(FILE *t) {
        while(!KONSOLE.available());
        return KONSOLE.read();
}
#else
extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for(j = 0; j < len; j++) {
                        if(fd == 1)
                                KONSOLE.write(*ptr++);
                        else if(fd == 2)
                                USB_HOST_SERIAL.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if(len > 0 && fd == 0) {
                        while(!KONSOLE.available());
                        *ptr = KONSOLE.read();
                        return 1;
                }
                return 0;
        }

        int _fstat(int fd, struct stat *st) {
                memset(st, 0, sizeof (*st));
                st->st_mode = S_IFCHR;
                st->st_blksize = 1024;
                return 0;
        }

        int _isatty(int fd) {
                return (fd < 3) ? 1 : 0;
        }
}
#endif

void USB_Setup(USB_Module_Calls func[]) {

#if CONSOLE == 0
        while(!KONSOLE);
#endif
        delay(100); // Stupid, but needed (argh!)
#if defined(__AVR__)
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
#elif defined(__arm__) && defined(CORE_TEENSY)
        //setbuffer(stdout, NULL, 0);
        //setvbuf (stdout, NULL, _IONBF, 0);
#endif
        KONSOLE.begin(CONSOLE_BAUD_RATE);

#if USB_HOST_SERIAL_NUM != CONSOLE
        USB_HOST_SERIAL.begin(CONSOLE_BAUD_RATE);
#endif

        uint8_t i = 0;
        while(func[i]) {
                USB_Module[i] = func[i];
                if(!USB_Module[i](0, 0, 0)) return; // Runs Setup on each
                i++;
                if(i == MAX_USB_MODULES) {
                        break;
                }
        }
        USB_Module[i] = NULL;
#if defined(__AVR__)
        PID_of_USB = xmem::SetupTask(USB_main
#ifdef _MAX_SS
#if _MAX_SS != 512
                , 0x1000 /* adjust stack 1000 4k, c00 3k, 800 2k*/
#endif
#endif
                );
        if(PID_of_USB) xmem::StartTask(PID_of_USB);
#endif
}
