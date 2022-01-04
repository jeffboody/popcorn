#version 450

layout(location=0) in vec4 varying_vertex;
layout(location=1) in vec4 varying_normal;

layout(location=0) out vec4 fragColor;

void main()
{
	vec3 ambient = vec3(0.2, 0.2, 0.2);

	vec3  l     = vec3(0.0, 0.0, 0.0);
	vec3  n     = normalize(varying_normal.xyz);
	vec3  p     = varying_vertex.xyz;
	float ndotl = dot(n, l - p);
	if(ndotl > 0.0)
	{
		vec3  diffuse = 0.4*vec3(ndotl, ndotl, ndotl);
		vec3  color   = clamp(vec3(0.0, 0.0, 0.0),
		                      vec3(1.0, 1.0, 1.0),
		                      ambient + diffuse);
		fragColor     = vec4(color, 1.0);
	}
	else
	{
		fragColor = vec4(ambient, 1.0);
	}
}
