#version 450

layout(location=0) in vec2 varying_uv;
layout(location=1) in vec4 varying_rgba;

layout(location=0) out vec4 fragColor;

void main()
{
	// create the checkerboard pattern
	vec4  color = varying_rgba;
	vec2  m     = mod(32.0*varying_uv, 2.0);
	float c     = 0.7;
	if((m.x > 1.0) && (m.y > 1.0))
	{
		c = 0.8;
	}
	else if((m.x < 1.0) && (m.y < 1.0))
	{
		c = 0.6;
	}
	fragColor = vec4(c*color.rgb, 1.0);
}
