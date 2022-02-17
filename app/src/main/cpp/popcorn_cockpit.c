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
#include "libgltf/gltf.h"
#include "libpak/pak_file.h"
#include "popcorn_cockpit.h"

/***********************************************************
* private                                                  *
***********************************************************/

static popcorn_part_t*
popcorn_part_new(vkk_engine_t* engine, gltf_file_t* file,
                 gltf_primitive_t* primitive)
{
	ASSERT(engine);
	ASSERT(file);
	ASSERT(primitive);

	gltf_accessor_t* aib;
	gltf_accessor_t* anb = NULL;
	gltf_accessor_t* avb = NULL;

	// get accessors
	cc_listIter_t* iter;
	iter = cc_list_head(primitive->attributes);
	aib  = gltf_file_getAccessor(file, primitive->indices);
	while(iter)
	{
		gltf_attribute_t* attr;
		attr = (gltf_attribute_t*) cc_list_peekIter(iter);

		if(strcmp(attr->name, "POSITION") == 0)
		{
			avb = gltf_file_getAccessor(file, attr->accessor);
		}
		else if(strcmp(attr->name, "NORMAL") == 0)
		{
			anb = gltf_file_getAccessor(file, attr->accessor);
		}

		iter = cc_list_next(iter);
	}
	if((aib == NULL) || (anb == NULL) || (avb == NULL))
	{
		LOGE("invalid accessors=%p,%p,%p",
		     aib, anb, avb);
		return NULL;
	}

	// check for required component types
	if((aib->componentType != GLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ||
	   (anb->componentType != GLTF_COMPONENT_TYPE_FLOAT)          ||
	   (avb->componentType != GLTF_COMPONENT_TYPE_FLOAT))
	{
		LOGE("invalid componentType=0x%X,0x%X,0x%X",
		     aib->componentType,
		     anb->componentType,
		     avb->componentType);
		return NULL;
	}

	// require bufferViews
	if((aib->has_bufferView == 0) ||
	   (anb->has_bufferView == 0) ||
	   (avb->has_bufferView == 0))
	{
		LOGE("invalid bufferView=%u,%u,%u",
		     (uint32_t) aib->has_bufferView,
		     (uint32_t) anb->has_bufferView,
		     (uint32_t) avb->has_bufferView);
		return NULL;
	}

	// get bufferViews
	gltf_bufferView_t* bvib;
	gltf_bufferView_t* bvnb;
	gltf_bufferView_t* bvvb;
	bvib = gltf_file_getBufferView(file, aib->bufferView);
	bvnb = gltf_file_getBufferView(file, anb->bufferView);
	bvvb = gltf_file_getBufferView(file, avb->bufferView);
	if((bvib == NULL) || (bvnb == NULL) || (bvvb == NULL))
	{
		LOGE("invalid bufferView=%p,%p,%p",
		     bvib, bvnb, bvvb);
		return NULL;
	}

	// disallow byteStride
	if(bvib->has_byteStride ||
	   bvnb->has_byteStride ||
	   bvvb->has_byteStride)
	{
		LOGE("invalid byteStride=%u,%u,%u",
		     (uint32_t) bvib->has_byteStride,
		     (uint32_t) bvnb->has_byteStride,
		     (uint32_t) bvvb->has_byteStride);
		return NULL;
	}

	// get buffers
	const char* bib = gltf_file_getBuffer(file, bvib);
	const char* bnb = gltf_file_getBuffer(file, bvnb);
	const char* bvb = gltf_file_getBuffer(file, bvvb);
	if((bib == NULL) || (bnb == NULL) || (bvb == NULL))
	{
		LOGE("invalid buffers=%p,%p,%p",
		     bib, bnb, bvb);
		return NULL;
	}

	popcorn_part_t* self;
	self = (popcorn_part_t*)
	       CALLOC(1, sizeof(popcorn_part_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ic = aib->count;
	self->ib = vkk_buffer_new(engine,
	                          VKK_UPDATE_MODE_STATIC,
	                          VKK_BUFFER_USAGE_INDEX,
	                          bvib->byteLength,
	                          bib);
	if(self->ib == NULL)
	{
		goto fail_ib;
	}

	self->vbnb[0] = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               bvvb->byteLength,
	                               bvb);
	if(self->vbnb[0] == NULL)
	{
		goto fail_vb;
	}

	self->vbnb[1] = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               bvnb->byteLength,
	                               bnb);
	if(self->vbnb[1] == NULL)
	{
		goto fail_nb;
	}

	// success
	return self;

	// failure
	fail_nb:
		vkk_buffer_delete(&self->vbnb[0]);
	fail_vb:
		vkk_buffer_delete(&self->ib);
	fail_ib:
		FREE(self);
	return NULL;
}

static void popcorn_part_delete(popcorn_part_t** _self)
{
	ASSERT(_self);

	popcorn_part_t* self = *_self;
	if(self)
	{
		vkk_buffer_delete(&self->vbnb[1]);
		vkk_buffer_delete(&self->vbnb[0]);
		vkk_buffer_delete(&self->ib);
		FREE(self);
		*_self = NULL;
	}
}

static int
popcorn_cockpit_parseNode(popcorn_cockpit_t* self,
                          gltf_file_t* file,
                          gltf_node_t* node)
{
	ASSERT(self);
	ASSERT(file);
	ASSERT(node);

	gltf_mesh_t* mesh;
	mesh = gltf_file_getMesh(file, node->mesh);
	if(mesh == NULL)
	{
		return 0;
	}

	popcorn_part_t* part;

	cc_listIter_t* iter;
	iter = cc_list_head(mesh->primitives);
	while(iter)
	{
		gltf_primitive_t* primitive;
		primitive = (gltf_primitive_t*) cc_list_peekIter(iter);

		// require indexed triangles
		if((primitive->has_indices == 0) ||
		   (primitive->mode != GLTF_PRIMITIVE_MODE_TRIANGLES))
		{
			// ignore
			iter = cc_list_next(iter);
			continue;
		}

		part = popcorn_part_new(self->engine, file, primitive);
		if(part == NULL)
		{
			return 0;
		}

		if(cc_list_append(self->parts, NULL,
		                  (const void*) part) == NULL)
		{
			goto fail_append;
		}

		iter = cc_list_next(iter);
	}

	// success
	return 1;

	// failure
	fail_append:
		popcorn_part_delete(&part);
	return 0;
}

static int
popcorn_cockpit_parseScene(popcorn_cockpit_t* self,
                           gltf_file_t* file,
                           gltf_scene_t* scene)
{
	ASSERT(self);
	ASSERT(file);
	ASSERT(scene);

	cc_listIter_t* iter = cc_list_head(scene->nodes);
	while(iter)
	{
		uint32_t* nd = (uint32_t*) cc_list_peekIter(iter);

		gltf_node_t* node = gltf_file_getNode(file, *nd);
		if(node == NULL)
		{
			return 0;
		}

		// ignore nodes w/o a mesh
		if(node->has_mesh == 0)
		{
			iter = cc_list_next(iter);
			continue;
		}

		if(popcorn_cockpit_parseNode(self, file, node) == 0)
		{
			return 0;
		}

		iter = cc_list_next(iter);
	}

	return 1;
}

static int
popcorn_cockpit_parseFile(popcorn_cockpit_t* self,
                          gltf_file_t* file)
{
	ASSERT(self);
	ASSERT(file);

	gltf_scene_t* scene;
	scene = gltf_file_getScene(file, file->scene);
	if(scene == NULL)
	{
		return 0;
	}

	return popcorn_cockpit_parseScene(self, file, scene);
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
		// layout(location=0) in vec3 vertex;
		{
			.location   = 0,
			.components = 3,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
		// layout(location=1) in vec3 normal;
		{
			.location   = 1,
			.components = 3,
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

	self->parts = cc_list_new();
	if(self->parts == NULL)
	{
		goto fail_parts;
	}

	char fname[256];
	snprintf(fname, 256, "%s/resource.pak",
	         vkk_engine_internalPath(engine));

	pak_file_t* pak;
	pak = pak_file_open(fname, PAK_FLAG_READ);
	if(pak == NULL)
	{
		goto fail_open;
	}

	size_t size;
	size = pak_file_seek(pak, "models/bat-rider.glb");
	if(size == 0)
	{
		LOGE("pak_file_seek failed");
		goto fail_seek;
	}

	gltf_file_t* file = gltf_file_openf(pak->f, size);
	if(file == NULL)
	{
		goto fail_gltf;
	}

	if(popcorn_cockpit_parseFile(self, file) == 0)
	{
		goto fail_parse;
	}

	gltf_file_close(&file);
	pak_file_close(&pak);

	// success
	return self;

	// failure
	fail_parse:
		gltf_file_close(&file);
	fail_gltf:
	fail_seek:
		pak_file_close(&pak);
	fail_open:
	{
		cc_listIter_t* iter = cc_list_head(self->parts);
		while(iter)
		{
			popcorn_part_t* part;
			part = (popcorn_part_t*)
			       cc_list_remove(self->parts, &iter);
			popcorn_part_delete(&part);
		}
		cc_list_delete(&self->parts);
	}
	fail_parts:
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
		FREE(self);
	return NULL;
}

void popcorn_cockpit_delete(popcorn_cockpit_t** _self)
{
	ASSERT(_self);

	popcorn_cockpit_t* self = *_self;
	if(self)
	{
		cc_listIter_t* iter = cc_list_head(self->parts);
		while(iter)
		{
			popcorn_part_t* part;
			part = (popcorn_part_t*)
			       cc_list_remove(self->parts, &iter);
			popcorn_part_delete(&part);
		}

		cc_list_delete(&self->parts);
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

	cc_listIter_t* iter = cc_list_head(self->parts);
	while(iter)
	{
		popcorn_part_t* part;
		part = (popcorn_part_t*)
		       cc_list_peekIter(iter);

		vkk_renderer_drawIndexed(rend, part->ic, 2,
		                         VKK_INDEX_TYPE_USHORT,
		                         part->ib, part->vbnb);

		iter = cc_list_next(iter);
	}
}
