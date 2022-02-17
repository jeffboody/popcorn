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

#ifndef popcorn_cockpit_H
#define popcorn_cockpit_H

#include "libcc/cc_list.h"
#include "libvkk/vkk.h"

typedef struct
{
	uint32_t      ic;
	vkk_buffer_t* ib;
	vkk_buffer_t* vbnb[2];
} popcorn_part_t;

typedef struct popcorn_cockpit_s
{
	vkk_engine_t*            engine;
	vkk_uniformSetFactory_t* usf0;
	vkk_pipelineLayout_t*    pl;
	vkk_graphicsPipeline_t*  gp;
	vkk_buffer_t*            ub00_mvp;
	vkk_uniformSet_t*        us0;
	cc_list_t*               parts;
} popcorn_cockpit_t;

popcorn_cockpit_t* popcorn_cockpit_new(vkk_engine_t* engine);
void               popcorn_cockpit_delete(popcorn_cockpit_t** _self);
void               popcorn_cockpit_draw(popcorn_cockpit_t* self,
                                        float fovy,
                                        float aspect,
                                        float rx,
                                        float ry);

#endif
