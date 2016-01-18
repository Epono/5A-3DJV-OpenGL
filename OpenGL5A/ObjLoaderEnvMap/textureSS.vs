#version 150

#extension GL_ARB_explicit_attrib_location : enable

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_texCoords;

// necessite un appel a glDrawArrays(GL_TRIANGLE_STRIP, 0, 4)

const vec2 g_quad[4] = vec2[]
(
	vec2(-1.0, 1.0), vec2(-1.0, -1.0),
	vec2(1.0, 1.0), vec2(1.0, -1.0)
);

/*const vec2 g_tex[4] = vec2[]
(
	vec2(0.0, 1.0), vec2(0.0, 0.0),
	vec2(1.0, 1.0), vec2(1.0, 0.0)
);*/

out vec3 v_texCoords;

void main(void)
{
	gl_Position = a_position;//vec4(g_quad[gl_VertexID], 1.0, 1.0);
	v_texCoords = (a_texCoords.xyz);//g_tex[gl_VertexID];
}