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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "popcorn"
#include "libcc/math/cc_mat4f.h"
#include "libcc/math/cc_vec4f.h"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "libpak/pak_file.h"
#include "popcorn_cockpit.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
popcorn_cockpit_normal(popcorn_cockpit_t* self,
                       const char* line,
                       cc_vec4f_t* normal)
{
	ASSERT(self);
	ASSERT(line);
	ASSERT(normal);

	if(sscanf(line, "facet normal %f %f %f",
	          &normal->x, &normal->y, &normal->z) != 3)
	{
		LOGE("invalid line=%s", line);
	}
	normal->w = 1.0f;
}

static void
popcorn_cockpit_vertex(popcorn_cockpit_t* self,
                       const char* line,
                       cc_vec4f_t* normal,
                       cc_vec4f_t** _normal_array,
                       cc_vec4f_t** _vertex_array)
{
	ASSERT(self);
	ASSERT(line);
	ASSERT(normal);
	ASSERT(_normal_array);
	ASSERT(_vertex_array);

	cc_vec4f_t* normal_array = *_normal_array;
	cc_vec4f_t* vertex_array = *_vertex_array;

	cc_vec4f_t vertex;
	if(sscanf(line, "vertex %f %f %f",
	          &vertex.x, &vertex.y, &vertex.z) != 3)
	{
		LOGE("invalid line=%s", line);
		return;
	}
	vertex.w = 1.0f;

	int    idx   = self->vc;
	size_t count = self->vc + 1;

	cc_vec4f_t* tmp;

	// resize normal_array
	size_t nc = MEMSIZEPTR(normal_array)/sizeof(cc_vec4f_t);
	if(nc < count)
	{
		tmp = (cc_vec4f_t*)
		      REALLOC(normal_array, 2*count*sizeof(cc_vec4f_t));
		if(tmp == NULL)
		{
			LOGE("REALLOC failed");
			return;
		}

		normal_array   = tmp;
		*_normal_array = tmp;
	}

	// resize vertex_array
	size_t vc = MEMSIZEPTR(vertex_array)/sizeof(cc_vec4f_t);
	if(vc < count)
	{
		tmp = (cc_vec4f_t*)
		      REALLOC(vertex_array, 2*count*sizeof(cc_vec4f_t));
		if(tmp == NULL)
		{
			LOGE("REALLOC failed");
			return;
		}

		vertex_array   = tmp;
		*_vertex_array = tmp;
	}

	// store normal and vertex
	self->vc            = count;
	normal_array[idx].x = normal->x;
	normal_array[idx].y = normal->y;
	normal_array[idx].z = normal->z;
	normal_array[idx].w = 1.0f;
	vertex_array[idx].x = vertex.x;
	vertex_array[idx].y = vertex.y;
	vertex_array[idx].z = vertex.z;
	vertex_array[idx].w = 1.0f;
}

static int
popcorn_getline(int size, const char* buf,
                int* _offset, char* line)
{
	ASSERT(buf);
	ASSERT(_offset);
	ASSERT(line);

	int offset = *_offset;
	if(offset >= size)
	{
		return 0;
	}

	int len = 0;
	while(offset < size)
	{
		// check len with null character
		if(len >= 255)
		{
			LOGE("invalid len");
			return 0;
		}

		// add character
		char c = buf[offset];
		line[len]     = c;
		line[len + 1] = '\0';

		// update offsets
		++offset;
		++len;

		// check for end-of-line
		if(c == '\n')
		{
			break;
		}
	}

	*_offset = offset;

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

popcorn_cockpit_t*
popcorn_cockpit_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_renderer_t* rend;
	rend = vkk_engine_defaultRenderer(engine);

	popcorn_cockpit_t* self;
	self = (popcorn_cockpit_t*)
	       CALLOC(1, sizeof(popcorn_cockpit_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	char fname[256];
	snprintf(fname, 256, "%s/resource.pak",
	         vkk_engine_internalPath(engine));

	pak_file_t* pak;
	pak = pak_file_open(fname, PAK_FLAG_READ);
	if(pak == NULL)
	{
		goto fail_open;
	}

	int size = pak_file_seek(pak, "models/cockpit.stl");
	if(size == 0)
	{
		LOGE("pak_file_seek failed");
		goto fail_seek;
	}

	char* buf;
	buf = (char*) CALLOC(size, sizeof(char));
	if(buf == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_buf;
	}

	if(pak_file_read(pak, buf, size, 1) != 1)
	{
		LOGE("pak_file_read failed");
		goto fail_read;
	}

	cc_vec4f_t normal =
	{
		.x = 0.0f,
		.y = 0.0f,
		.z = 1.0f,
		.w = 1.0f,
	};

	cc_vec4f_t* normal_array = NULL;
	cc_vec4f_t* vertex_array = NULL;

	int  offset = 0;
	char line[256];
	while(popcorn_getline(size, buf, &offset, line))
	{
		if(strstr(line, "facet normal"))
		{
			popcorn_cockpit_normal(self, line, &normal);
		}
		else if(strstr(line, "vertex"))
		{
			popcorn_cockpit_vertex(self, line, &normal,
			                       &normal_array, &vertex_array);
		}
	}

	if(self->vc == 0)
	{
		LOGE("invalid vc=%i", self->vc);
		goto fail_vc;
	}

	vkk_uniformBinding_t ub_array0[] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_VS,
		},
	};

	self->usf0 = vkk_uniformSetFactory_new(engine,
	                                       VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                       1, ub_array0);
	if(self->usf0 == NULL)
	{
		goto fail_usf0;
	}

	vkk_uniformSetFactory_t* usf_array[] =
	{
		self->usf0,
	};

	self->pl = vkk_pipelineLayout_new(engine,
	                                  1, usf_array);
	if(self->pl == NULL)
	{
		goto fail_pl;
	}

	vkk_vertexBufferInfo_t vbi[] =
	{
		// layout(location=0) in vec4 vertex;
		{
			.location   = 0,
			.components = 4,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
		// layout(location=1) in vec4 normal;
		{
			.location   = 1,
			.components = 4,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
	};

	vkk_graphicsPipelineInfo_t gpi =
	{
		.renderer          = rend,
		.pl                = self->pl,
		.vs                = "shaders/cockpit_vert.spv",
		.fs                = "shaders/cockpit_frag.spv",
		.vb_count          = 2,
		.vbi               = vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_LIST,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 1,
		.depth_write       = 1,
		.blend_mode        = VKK_BLEND_MODE_DISABLED
	};

	self->gp = vkk_graphicsPipeline_new(engine, &gpi);
	if(self->gp == NULL)
	{
		goto fail_gp;
	}

	self->ub00_mvp = vkk_buffer_new(engine,
	                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_mat4f_t),
	                                NULL);
	if(self->ub00_mvp == NULL)
	{
		goto fail_ub00;
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

	self->us0 = vkk_uniformSet_new(engine, 0, 1,
	                               ua_array0,
	                               self->usf0);
	if(self->us0 == NULL)
	{
		goto fail_us0;
	}

	self->vbnb[0] = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               self->vc*sizeof(cc_vec4f_t),
	                               vertex_array);
	if(self->vbnb[0] == NULL)
	{
		goto fail_vb;
	}

	self->vbnb[1] = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               self->vc*sizeof(cc_vec4f_t),
	                               normal_array);
	if(self->vbnb[1] == NULL)
	{
		goto fail_nb;
	}

	FREE(vertex_array);
	FREE(normal_array);
	FREE(buf);
	pak_file_close(&pak);

	// success
	return self;

	// failure
	fail_nb:
		vkk_buffer_delete(&self->vbnb[0]);
	fail_vb:
		vkk_uniformSet_delete(&self->us0);
	fail_us0:
		vkk_buffer_delete(&self->ub00_mvp);
	fail_ub00:
		vkk_graphicsPipeline_delete(&self->gp);
	fail_gp:
		vkk_pipelineLayout_delete(&self->pl);
	fail_pl:
		vkk_uniformSetFactory_delete(&self->usf0);
	fail_usf0:
		FREE(vertex_array);
		FREE(normal_array);
	fail_vc:
	fail_read:
		FREE(buf);
	fail_buf:
	fail_seek:
		pak_file_close(&pak);
	fail_open:
		FREE(self);
	return NULL;
}

void popcorn_cockpit_delete(popcorn_cockpit_t** _self)
{
	ASSERT(_self);

	popcorn_cockpit_t* self = *_self;
	if(self)
	{
		vkk_buffer_delete(&self->vbnb[1]);
		vkk_buffer_delete(&self->vbnb[0]);
		vkk_uniformSet_delete(&self->us0);
		vkk_buffer_delete(&self->ub00_mvp);
		vkk_graphicsPipeline_delete(&self->gp);
		vkk_pipelineLayout_delete(&self->pl);
		vkk_uniformSetFactory_delete(&self->usf0);
		FREE(self);
		*_self = NULL;
	}
}

void popcorn_cockpit_draw(popcorn_cockpit_t* self,
                          float fovy, float aspect,
                          float rx, float ry)
{
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	vkk_renderer_t* rend;
	rend = vkk_engine_defaultRenderer(engine);

	float      near = 0.001f;
	float      far  = 1000.0f;
	cc_mat4f_t pm;
	cc_mat4f_t mvp;
	cc_mat4f_t mvm;
	cc_mat4f_perspective(&pm, 1,
	                     fovy, aspect,
	                     near, far);
	cc_mat4f_lookat(&mvm, 1,
	                0.0f, 0.0f, 0.0f,
	                0.0f, 1.0f, 0.0f,
	                0.0f, 0.0f, 1.0f);
	cc_mat4f_rotate(&mvm, 0, -rx, 0.0f, 0.0f, 1.0f);
	cc_mat4f_rotate(&mvm, 0, ry, 1.0f, 0.0f, 0.0f);
	cc_mat4f_mulm_copy(&pm, &mvm, &mvp);

	vkk_uniformSet_t* us_array[] =
	{
		self->us0,
	};

	vkk_renderer_clearDepth(rend);
	vkk_renderer_bindGraphicsPipeline(rend, self->gp);
	vkk_renderer_updateBuffer(rend, self->ub00_mvp,
	                          sizeof(cc_mat4f_t),
	                          (const void*) &mvp);
	vkk_renderer_bindUniformSets(rend, 1, us_array);
	vkk_renderer_draw(rend, self->vc, 2, self->vbnb);
}
