#version 150

in Vertex
{
	vec3 normal;
	vec2 texcoords;
    float fresnelTerm;
    vec3 reflectedDir;
    vec3 refractedDir;
    vec3 positionWS;
} IN;

uniform vec3 u_eyePosition;

// CubeMap stockant le rendu dynamique
uniform samplerCube u_dynamicCubeMap;
// CubeMap stockant la skybox
uniform samplerCube u_cubeMap;

out vec4 Fragment;

void main(void)
{
    vec3 N = normalize(IN.normal);
    vec4 ambient_env = texture(u_cubeMap, N);
    
    vec3 cubeReflectDir = normalize(IN.reflectedDir);
    vec4 env = texture(u_cubeMap, cubeReflectDir);
    vec4 local_env = texture(u_dynamicCubeMap, cubeReflectDir);
    
    vec3 cubeRefractDir = normalize(IN.refractedDir);
    vec4 transparent_env = texture(u_cubeMap, cubeRefractDir);

    // blending avec alpha premutiplie (cf. operateur "over" de Porter & Duff)
    // on interprete les valeurs RGBA de la source comme une contributation a l'image finale.
    // RGB = contribution de la couleur a l'image
    // Alpha = facteur de recouvrement. 0.0 = preserve la couleur du fond, 1.0 = recouvrement total par la source.
    //
    // comme la cubemap dynamique est effacee avec un alpha de 0.0 mais que les polygones sont opaques
    // au final la couche alpha de la cubemap dynamique agit comme un masque pour un calque.
    //
    vec4 specular_reflect = local_env + (1.0-local_env.a)*env;
    vec4 specular = specular_reflect;// * IN.fresnelTerm + transparent_env * (1.0 - IN.fresnelTerm);
    vec4 ambient = ambient_env;

    Fragment = /*ambient +*/ specular;
}
