#version 450

layout(location=0) in vec4 vertex;
layout(location=1) in vec4 normal;

layout(std140, set=0, binding=0) uniform uniformMvp
{
	mat4 mvp;
};

layout(location=0) out vec4 varying_vertex;
layout(location=1) out vec4 varying_normal;

void main()
{
	varying_vertex = vertex;
	varying_normal = normal;
	gl_Position    = mvp*vertex;
}
