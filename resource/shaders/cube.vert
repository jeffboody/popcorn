#version 450

layout(location=0) in vec4 xyzw;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 rgba;

layout(std140, set=0, binding=0) uniform uniformMvp
{
	mat4 mvp;
};

layout(location=0) out vec2 varying_uv;
layout(location=1) out vec4 varying_rgba;

void main()
{
	varying_uv   = uv;
	varying_rgba = rgba;
	gl_Position  = mvp*xyzw;
}
