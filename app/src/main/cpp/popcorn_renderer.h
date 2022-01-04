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
 *
 */

#ifndef popcorn_renderer_H
#define popcorn_renderer_H

#include "libcc/math/cc_quaternion.h"
#include "libcc/math/cc_vec3f.h"
#include "libvkk/vkk_platform.h"
#include "libvkk/vkk.h"
#include "popcorn_cockpit.h"

/***********************************************************
* public                                                   *
***********************************************************/

typedef struct popcorn_renderer_s
{
	vkk_engine_t*            engine;
	vkk_uniformSetFactory_t* usf0;
	vkk_pipelineLayout_t*    pl;
	vkk_graphicsPipeline_t*  gp;

	vkk_buffer_t* ub00_mvp;
	vkk_buffer_t* vb_xyzw;
	vkk_buffer_t* vb_uv;
	vkk_buffer_t* vb_rgba;

	vkk_uniformSet_t* us0_mvp;

	// escape state
	double escape_t0;

	// rotation state
	float           yaw1;
	float           yaw2;
	float           pitch;
	float           roll;
	float           rx;
	float           ry;
	cc_quaternion_t attitude;

	// position
	float      acceleration;
	float      speed;
	cc_vec3f_t position;

	// cockpit
	popcorn_cockpit_t* cockpit;
} popcorn_renderer_t;

popcorn_renderer_t* popcorn_renderer_new(vkk_engine_t* engine);
void                popcorn_renderer_delete(popcorn_renderer_t** _self);
void                popcorn_renderer_draw(popcorn_renderer_t* self);
void                popcorn_renderer_event(popcorn_renderer_t* self,
                                           vkk_event_t* event);

#endif
