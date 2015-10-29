 #version 150

 // Direction VERS la source lumineuse exprimee dans le repere WORLD
const vec3 L = vec3(0.0, 0.0, 1.0);

uniform vec4 u_objectColor;

in vec3 v_normal;

out vec4 Fragment;

void main(void)
{
	// calcul du cosinus de l'angle entre les deux vecteurs
	float NdotL = max(dot(normalize(v_normal), L), 0.0);
	// Equation de Lambert : Intensite Reflechie = Intensite Incidente * N.L
    Fragment = u_objectColor * NdotL;
}