/*
 * main.c
 *
 * Copyright (c) 2014 Jeremy Garff <jer @ jers.net>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     1.  Redistributions of source code must retain the above copyright notice, this list of
 *         conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright notice, this list
 *         of conditions and the following disclaimer in the documentation and/or other materials
 *         provided with the distribution.
 *     3.  Neither the name of the owner nor the names of its contributors may be used to endorse
 *         or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>

#include "board_info.h"
#include "clk.h"
#include "gpio.h"
#include "pwm.h"

#include "ws2811.h"

//#define DEBUG
#define MAX_INTENSITY	64		// out of 255

#define ARRAY_SIZE(stuff)                        (sizeof(stuff) / sizeof(stuff[0]))

#define TARGET_FREQ                              WS2811_TARGET_FREQ
#define GPIO_PIN                                 18
#define DMA                                      5

#define WIDTH                                    8
#define HEIGHT                                   8
#define LED_COUNT                                (WIDTH * HEIGHT)


ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

struct pixel {
	float r;
	float g;
	float b;
};
struct pixel matrix[WIDTH*HEIGHT];


int dotspos[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
ws2811_led_t dotcolors[] =
{
    0x200000,  // red
    0x202000,  // yellow
    0x002020,  // lightblue
    0x200010,  // pink
    0x002000,  // green
    0x100010,  // purple
    0x000020,  // blue
    0x201000,  // orange
};

void renderMatrix()
{
	int i;
	for (i=0; i<WIDTH*HEIGHT; i++) {
		struct pixel* p = &matrix[i];
		setLED(i, p->r, p->g, p->b);
	}
}

void clearMatrix()
{
	for (int i=0; i<WIDTH*HEIGHT; i++) {
		matrix[i] = 0;
	}
}

static void ctrl_c_handler(int signum)
{
	clearMatrix();
	renderMatrix();
	ws2811_render(&ledstring);

    ws2811_fini(&ledstring);
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
}

uint32_t getScaled(float val)
{
	int v2 = (int)(val*MAX_INTENSITY);
	return v2;
}

void setLED(int i, float r, float g, float b)
{
	uint32_t finalVal = getScaled(r)<<16 | getScaled(g)<<8 | getScaled(b);
	ledstring.channel[0].leds[i] = finalVal;
#ifdef DEBUG
	printf("(r,g,b) = (%f, %f, %f)\n", r, g, b);
	printf("int (r,g,b) = (%d, %d, %d)\n", getScaled(r), getScaled(g), getScaled(b));
#endif
}

int main(int argc, char *argv[])
{
    int ret = 0;
	float time=0;
	int i;

    if (board_info_init() < 0)
    {
        return -1;
    }

    setup_handlers();

    if (ws2811_init(&ledstring))
    {
        return -1;
    }

	// reset matrix with
	clearMatrix();

    while (1)
    {
		// rainbow below
		for (i=0; i<WIDTH*HEIGHT; i++) {
			matrix[i].r = (sin(time*17 + (float)i/100)*0.5+0.5)*1;
			matrix[i].g = (sin(1+time*19 + (float)i/100)*0.5+0.5)*1;
			matrix[i].b = (sin(2+time*13 + (float)i/100)*0.5+0.5)*1;
		}

		// render matrix into ledstring
		renderMatrix();

        if (ws2811_render(&ledstring))
        {
            ret = -1;
            break;
        }

		// refresh every 5 ms (200HZ)
        usleep(5000);
		// advance time by 1.6ms
		time+=0.01666667;
    }

    ws2811_fini(&ledstring);

    return ret;
}

