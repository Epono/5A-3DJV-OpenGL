 #version 330

 // Direction VERS la source lumineuse exprimee dans le repere WORLD
const vec3 L = vec3(0.0, 0.0, 1.0);

uniform vec4 u_objectColor;

uniform int u_hasNormals;
uniform int u_hasTexcoords;

in vec3 v_normal;

out vec4 Fragment;

uniform sampler2D u_texture0;
in vec2 v_texCoords;

void main(void)
{
    if(u_hasNormals == 0) {
        if(u_hasTexcoords == 0) {
            Fragment = u_objectColor;
        } else {
            vec4 texColor = texture2D(u_texture0, v_texCoords);
	        Fragment = texColor; 
        }
    } else {
        if(u_hasTexcoords == 0) {
	        // calcul du cosinus de l'angle entre les deux vecteurs
	        float NdotL = max(dot(normalize(v_normal), L), 0.0);
	        // Equation de Lambert : Intensite Reflechie = Intensite Incidente * N.L
            Fragment = u_objectColor * NdotL;
        } else {
	        float NdotL = max(dot(normalize(v_normal), L), 0.0);
            vec4 texColor = texture2D(u_texture0, v_texCoords);
	        Fragment = texColor * NdotL; 
        }
    }
    //Fragment = vec4(0.5, 0.5, 0.5, 0.5);
}