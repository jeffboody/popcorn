/*
 * Copyright (c) 2022 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>

#define LOG_TAG "popcorn"
#include "libcc/cc_log.h"
#include "libvkk/vkk_platform.h"
#include "popcorn_renderer.h"

/***********************************************************
* callbacks                                                *
***********************************************************/

void* popcorn_onCreate(vkk_engine_t* engine)
{
	ASSERT(engine);

	return (void*) popcorn_renderer_new(engine);
}

void popcorn_onDestroy(void** _priv)
{
	ASSERT(_priv);

	popcorn_renderer_delete((popcorn_renderer_t**) _priv);
}

void popcorn_onPause(void* priv)
{
	ASSERT(priv);

	// ignore
}

void popcorn_onDraw(void* priv)
{
	ASSERT(priv);

	popcorn_renderer_draw((popcorn_renderer_t*) priv);
}

void popcorn_onEvent(void* priv, vkk_event_t* event)
{
	ASSERT(priv);
	ASSERT(event);

	popcorn_renderer_event((popcorn_renderer_t*) priv, event);
}

vkk_platformInfo_t VKK_PLATFORM_INFO =
{
	.app_name    = "Popcorn",
	.app_version =
	{
		.major = 1,
		.minor = 0,
		.patch = 0,
	},
	.app_dir     = "Popcorn",
	.onCreate    = popcorn_onCreate,
	.onDestroy   = popcorn_onDestroy,
	.onPause     = popcorn_onPause,
	.onDraw      = popcorn_onDraw,
	.onEvent     = popcorn_onEvent,
};
