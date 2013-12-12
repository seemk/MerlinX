#include "console.h"
#include <avr/pgmspace.h>
#include "usb/usb_serial.h"
#include <stdio.h>
#include <stdarg.h>

void console_send_str(const char *s)
{
	char c;
	while (1) {
		c = pgm_read_byte(s++);
		if (!c) break;
		usb_serial_putchar(c);
	}
}

uint8_t console_recv_str(char *buf, uint8_t size)
{
	int16_t r;
	uint8_t count=0;

	while (count < size) {
		r = usb_serial_getchar();
		if (r != -1) {
			if (r == '\r' || r == '\n') return count;
			if (r >= ' ' && r <= '~') {
				*buf++ = r;
				usb_serial_putchar(r);
				count++;
			}
			} else {
			if (!usb_configured() ||
			!(usb_serial_get_control() & USB_SERIAL_DTR)) {
				return 255;
			}
		}
	}
	return count;
}

uint8_t console_status(uint8_t prev_status)
{
	uint8_t attached = prev_status;
	if(usb_serial_get_control() & USB_SERIAL_DTR)
	{
		if(!attached)
		{
			attached = 1;
			usb_serial_flush_input();
			console_send_str(PSTR("Hello there."));
		}
	}
	else
	{
		if(attached)
		{
			attached = 0;
		}
	}
	
	return attached;
}

void console_init()
{
	usb_init();
}

uint8_t console_configured()
{
	return usb_configured();
}

int8_t console_write(const char *buffer, uint16_t size)
{
	return usb_serial_write((const uint8_t*)buffer, size);
}

int8_t console_write_fmt(const char* fmt, ...)
{
	char buf[32];
	va_list args;
	va_start(args, fmt);
	uint16_t size = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return console_write(buf, size);
}