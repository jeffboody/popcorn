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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "popcorn"
#include "libcc/math/cc_mat4f.h"
#include "libcc/math/cc_vec2f.h"
#include "libcc/math/cc_vec4f.h"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "libcc/cc_timestamp.h"
#include "libvkk/vkk_platform.h"
#include "popcorn_cockpit.h"
#include "popcorn_renderer.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
popcorn_renderer_newUniformSetFactory(popcorn_renderer_t* self)
{
	ASSERT(self);

	vkk_uniformBinding_t ub_array[] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_VS,
		},
	};

	self->usf0 = vkk_uniformSetFactory_new(self->engine,
	                                       VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                       1, ub_array);
	if(self->usf0 == NULL)
	{
		return 0;
	}

	return 1;
}

static int
popcorn_renderer_newPipelineLayout(popcorn_renderer_t* self)
{
	ASSERT(self);

	self->pl = vkk_pipelineLayout_new(self->engine,
	                                  1, &self->usf0);
	if(self->pl == NULL)
	{
		return 0;
	}

	return 1;
}

static int
popcorn_renderer_newGraphicsPipeline(popcorn_renderer_t* self)
{
	ASSERT(self);

	vkk_renderer_t* rend;
	rend = vkk_engine_defaultRenderer(self->engine);

	vkk_vertexBufferInfo_t vbi[] =
	{
		// layout(location=0) in vec4 vertex;
		{
			.location   = 0,
			.components = 4,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
		// layout(location=1) in vec2 uv;
		{
			.location   = 1,
			.components = 2,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
		// layout(location=2) in vec4 rgba;
		{
			.location   = 2,
			.components = 4,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
	};

	vkk_graphicsPipelineInfo_t gpi =
	{
		.renderer          = rend,
		.pl                = self->pl,
		.vs                = "shaders/cube_vert.spv",
		.fs                = "shaders/cube_frag.spv",
		.vb_count          = 3,
		.vbi               = vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_LIST,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 1,
		.depth_write       = 1,
		.blend_mode        = 0
	};

	self->gp = vkk_graphicsPipeline_new(self->engine,
	                                    &gpi);
	if(self->gp == NULL)
	{
		return 0;
	}

	return 1;
}

static void
popcorn_renderer_keyPress(popcorn_renderer_t* self,
                          int keycode, int meta)
{
	ASSERT(self);

	if(keycode == VKK_KEYCODE_ESCAPE)
	{
		// double tap back to exit
		double t1 = cc_timestamp();
		if((t1 - self->escape_t0) < 0.5)
		{
			vkk_engine_platformCmd(self->engine,
			                       VKK_PLATFORM_CMD_EXIT, NULL);
		}
		else
		{
			self->escape_t0 = t1;
		}
	}
}

static void
popcorn_renderer_reset(popcorn_renderer_t* self)
{
	ASSERT(self);

	self->speed        = 0.0f;
	self->acceleration = 0.0f;
	cc_vec3f_load(&self->position, 0.0f, 0.0f, 0.0f);
	cc_quaternion_identity(&self->attitude);
}

/***********************************************************
* public                                                   *
***********************************************************/

popcorn_renderer_t*
popcorn_renderer_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	popcorn_renderer_t* self;
	self = (popcorn_renderer_t*)
	       CALLOC(1, sizeof(popcorn_renderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine    = engine;
	self->escape_t0 = cc_timestamp();

	// default attitude
	cc_quaternion_identity(&self->attitude);

	if(popcorn_renderer_newUniformSetFactory(self) == 0)
	{
		goto fail_usf;
	}

	if(popcorn_renderer_newPipelineLayout(self) == 0)
	{
		goto fail_pl;
	}

	if(popcorn_renderer_newGraphicsPipeline(self) == 0)
	{
		goto fail_gp;
	}

	const cc_vec4f_t A = { .x=-1.0f, .y= 1.0f, .z= 1.0f, .w=1.0f };
	const cc_vec4f_t B = { .x=-1.0f, .y=-1.0f, .z= 1.0f, .w=1.0f };
	const cc_vec4f_t C = { .x= 1.0f, .y= 1.0f, .z= 1.0f, .w=1.0f };
	const cc_vec4f_t D = { .x= 1.0f, .y=-1.0f, .z= 1.0f, .w=1.0f };
	const cc_vec4f_t E = { .x=-1.0f, .y= 1.0f, .z=-1.0f, .w=1.0f };
	const cc_vec4f_t F = { .x=-1.0f, .y=-1.0f, .z=-1.0f, .w=1.0f };
	const cc_vec4f_t G = { .x= 1.0f, .y= 1.0f, .z=-1.0f, .w=1.0f };
	const cc_vec4f_t H = { .x= 1.0f, .y=-1.0f, .z=-1.0f, .w=1.0f };
	cc_vec4f_t xyzw[] =
	{
		A, B, D, A, D, C, // top
		E, F, H, E, H, G, // bottom
		C, G, H, C, H, D, // right
		A, E, F, A, F, B, // left
		A, E, G, A, G, C, // back
		B, F, H, B, H, D, // front
	};

	const cc_vec2f_t UV00 = { .x=0.0f, .y=0.0f };
	const cc_vec2f_t UV01 = { .x=0.0f, .y=1.0f };
	const cc_vec2f_t UV10 = { .x=1.0f, .y=0.0f };
	const cc_vec2f_t UV11 = { .x=1.0f, .y=1.0f };
	cc_vec2f_t uv[] =
	{
		UV00, UV01, UV11, UV00, UV11, UV10, // top
		UV00, UV01, UV11, UV00, UV11, UV10, // bottom
		UV00, UV01, UV11, UV00, UV11, UV10, // right
		UV00, UV01, UV11, UV00, UV11, UV10, // left
		UV00, UV01, UV11, UV00, UV11, UV10, // back
		UV00, UV01, UV11, UV00, UV11, UV10, // front
	};

	const cc_vec4f_t TT = { .x=0.0f, .y=0.0f, .z=1.0f, .w=1.0f };
	const cc_vec4f_t BO = { .x=0.0f, .y=1.0f, .z=1.0f, .w=1.0f };
	const cc_vec4f_t RR = { .x=1.0f, .y=0.0f, .z=0.0f, .w=1.0f };
	const cc_vec4f_t LL = { .x=1.0f, .y=0.0f, .z=1.0f, .w=1.0f };
	const cc_vec4f_t BA = { .x=0.0f, .y=1.0f, .z=0.0f, .w=1.0f };
	const cc_vec4f_t FF = { .x=1.0f, .y=1.0f, .z=0.0f, .w=1.0f };
	cc_vec4f_t rgba[] =
	{
		TT, TT, TT, TT, TT, TT, // top
		BO, BO, BO, BO, BO, BO, // bottom
		RR, RR, RR, RR, RR, RR, // right
		LL, LL, LL, LL, LL, LL, // left
		BA, BA, BA, BA, BA, BA, // back
		FF, FF, FF, FF, FF, FF, // front
	};

	self->ub00_mvp = vkk_buffer_new(engine,
	                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                VKK_BUFFER_USAGE_VERTEX,
	                                0, NULL);
	if(self->ub00_mvp == NULL)
	{
		goto fail_ub00_mvp;
	}

	self->vb_xyzw = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               sizeof(xyzw),
	                               (const void*) xyzw);
	if(self->vb_xyzw == NULL)
	{
		goto fail_vb_xyzw;
	}

	self->vb_uv = vkk_buffer_new(engine,
	                             VKK_UPDATE_MODE_STATIC,
	                             VKK_BUFFER_USAGE_VERTEX,
	                             sizeof(uv),
	                             (const void*) uv);
	if(self->vb_uv == NULL)
	{
		goto fail_vb_uv;
	}

	self->vb_rgba = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               sizeof(rgba),
	                               (const void*) rgba);
	if(self->vb_rgba == NULL)
	{
		goto fail_vb_rgba;
	}

	vkk_uniformAttachment_t ua_array0[] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub00_mvp
		},
	};

	self->us0_mvp = vkk_uniformSet_new(engine, 0, 1,
	                                   ua_array0,
	                                   self->usf0);
	if(self->us0_mvp == NULL)
	{
		goto fail_us0_mvp;
	}

	self->cockpit = popcorn_cockpit_new(engine);
	if(self->cockpit == NULL)
	{
		LOGE("invalid cockpit");
		goto fail_cockpit;
	}

	// success
	return self;

	// failure
	fail_cockpit:
		vkk_uniformSet_delete(&self->us0_mvp);
	fail_us0_mvp:
		vkk_buffer_delete(&self->vb_rgba);
	fail_vb_rgba:
		vkk_buffer_delete(&self->vb_uv);
	fail_vb_uv:
		vkk_buffer_delete(&self->vb_xyzw);
	fail_vb_xyzw:
		vkk_buffer_delete(&self->ub00_mvp);
	fail_ub00_mvp:
		vkk_graphicsPipeline_delete(&self->gp);
	fail_gp:
		vkk_pipelineLayout_delete(&self->pl);
	fail_pl:
		vkk_uniformSetFactory_delete(&self->usf0);
	fail_usf:
		FREE(self);
	return NULL;
}

void popcorn_renderer_delete(popcorn_renderer_t** _self)
{
	// *_self can be null
	ASSERT(_self);

	popcorn_renderer_t* self = *_self;
	if(self)
	{
		popcorn_cockpit_delete(&self->cockpit);
		vkk_uniformSet_delete(&self->us0_mvp);
		vkk_buffer_delete(&self->vb_rgba);
		vkk_buffer_delete(&self->vb_uv);
		vkk_buffer_delete(&self->vb_xyzw);
		vkk_buffer_delete(&self->ub00_mvp);
		vkk_graphicsPipeline_delete(&self->gp);
		vkk_pipelineLayout_delete(&self->pl);
		vkk_uniformSetFactory_delete(&self->usf0);
		FREE(self);
		*_self = NULL;
	}
}

void popcorn_renderer_draw(popcorn_renderer_t* self)
{
	ASSERT(self);

	vkk_renderer_t* rend;
	rend = vkk_engine_defaultRenderer(self->engine);

	float clear_color[4] =
	{
		0.0f, 0.0f, 0.0f, 1.0f
	};
	if(vkk_renderer_beginDefault(rend,
	                             VKK_RENDERER_MODE_DRAW,
	                             clear_color) == 0)
	{
		return;
	}

	uint32_t width;
	uint32_t height;
	vkk_renderer_surfaceSize(rend, &width, &height);

	// perspective projection
	float      w      = (float) width;
	float      h      = (float) height;
	float      fovy   = (h > w) ? 60.0f : 45.0f;
	float      aspect = w/h;
	float      near   = 0.001f;
	float      far    = 1000.0f;
	cc_mat4f_t pm;
	cc_mat4f_perspective(&pm, 1,
	                     fovy, aspect,
	                     near, far);

	// remap orientation
	// see the principle axes of an aircraft
	// https://en.wikipedia.org/wiki/Euler_angles
	cc_mat4f_t mvm; // model-view-matrix
	cc_mat4f_t mnm; // model-normal-matrix
	cc_mat4f_lookat(&mvm, 1,
	                0.0f, 0.0f, 0.0f,
	                1.0f, 0.0f, 0.0f,
	                0.0f, 0.0f, -1.0f);
	cc_mat4f_copy(&mvm, &mnm);

	// head rotation
	float rx = -60.0f*self->rx;
	float ry = 30.0f*self->ry;
	cc_mat4f_rotate(&mvm, 0, rx, 0.0f, 0.0f, 1.0f);
	cc_mat4f_rotate(&mvm, 0, ry, 0.0f, 1.0f, 0.0f);

	// compute the attitude change
	float rate  = 45.0f/60.0f;
	float yaw   = rate*(self->yaw1 - self->yaw2);
	float pitch = -rate*self->pitch;
	float roll  = -rate*self->roll;
	cc_quaternion_t q;
	cc_quaternion_loadeuler(&q, roll, pitch, yaw);

	// update attitude
	// post multiply the current attitude
	// Flight Simulators and Quaternions
	// https://flylib.com/books/en/2.208.1.130/1/
	cc_quaternion_rotateq(&q, &self->attitude);
	cc_quaternion_copy(&q, &self->attitude);

	// attitude rotation
	cc_mat4f_rotateq(&mvm, 0, &self->attitude);
	cc_mat4f_rotateq(&mnm, 0, &self->attitude);

	// compute direction
	cc_vec3f_t direction;
	cc_vec3f_load(&direction, -mnm.m20, -mnm.m21, -mnm.m22);

	// update speed
	self->speed += 0.0001f*self->acceleration;
	if(self->speed < 0.0f)
	{
		self->speed = 0.0f;
	}
	else if(self->speed > 0.005f)
	{
		self->speed = 0.005f;
	}

	// update position
	cc_vec3f_t velocity;
	cc_vec3f_muls_copy(&direction, self->speed, &velocity);
	cc_vec3f_addv(&self->position, &velocity);
	if((self->position.x < -1.0f) ||
	   (self->position.x >  1.0f) ||
	   (self->position.y < -1.0f) ||
	   (self->position.y >  1.0f) ||
	   (self->position.z < -1.0f) ||
	   (self->position.z >  1.0f))
	{
		// reset on collision
		popcorn_renderer_reset(self);
	}
	cc_mat4f_translate(&mvm, 0, -self->position.x,
	                   -self->position.y, -self->position.z);

	// finalize mvp
	cc_mat4f_t mvp;
	cc_mat4f_mulm_copy(&pm, &mvm, &mvp);

	// draw cube
	vkk_uniformSet_t* us_array[] =
	{
		self->us0_mvp,
	};

	vkk_buffer_t* vb_array[] =
	{
		self->vb_xyzw,
		self->vb_uv,
		self->vb_rgba,
	};

	vkk_renderer_bindGraphicsPipeline(rend, self->gp);
	vkk_renderer_updateBuffer(rend, self->ub00_mvp,
	                          sizeof(cc_mat4f_t),
	                          (const void*) &mvp);
	vkk_renderer_bindUniformSets(rend, 1, us_array);
	vkk_renderer_draw(rend, 36, 3, vb_array);

	// draw cockpit
	popcorn_cockpit_draw(self->cockpit, fovy, aspect, rx, ry);

	vkk_renderer_end(rend);
}

void popcorn_renderer_event(popcorn_renderer_t* self,
                            vkk_event_t* event)
{
	ASSERT(self);
	ASSERT(event);

	if((event->type == VKK_EVENT_TYPE_KEY_UP) ||
	   ((event->type == VKK_EVENT_TYPE_KEY_DOWN) &&
	    (event->key.repeat)))
	{
		vkk_eventKey_t* e = &event->key;

		popcorn_renderer_keyPress(self, e->keycode, e->meta);
	}
	else if(event->type == VKK_EVENT_TYPE_AXIS_MOVE)
	{
		vkk_eventAxis_t* e = &event->axis;

		if(e->axis == VKK_AXIS_X1)
		{
			self->roll = e->value;
		}
		else if(e->axis == VKK_AXIS_Y1)
		{
			self->pitch = e->value;
		}
		else if(e->axis == VKK_AXIS_X2)
		{
			self->rx = e->value;
		}
		else if(e->axis == VKK_AXIS_Y2)
		{
			self->ry = e->value;
		}
		else if(e->axis == VKK_AXIS_LT)
		{
			self->yaw1 = e->value;
		}
		else if(e->axis == VKK_AXIS_RT)
		{
			self->yaw2 = e->value;
		}
	}
	else if(event->type == VKK_EVENT_TYPE_BUTTON_UP)
	{
		vkk_eventButton_t* e = &event->button;

		if(e->button == VKK_BUTTON_A)
		{
			self->acceleration += 1.0f;
		}
		else if(e->button == VKK_BUTTON_B)
		{
			self->acceleration -= 1.0f;
		}
		else if(e->button == VKK_BUTTON_X)
		{
			popcorn_renderer_reset(self);
		}
	}
	else if(event->type == VKK_EVENT_TYPE_BUTTON_DOWN)
	{
		vkk_eventButton_t* e = &event->button;

		if(e->button == VKK_BUTTON_A)
		{
			self->acceleration -= 1.0f;
		}
		else if(e->button == VKK_BUTTON_B)
		{
			self->acceleration += 1.0f;
		}
	}
}
