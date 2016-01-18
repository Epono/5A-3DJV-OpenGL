 
 #version 150

#extension GL_ARB_explicit_attrib_location : enable

const float PI = 3.14159265;

const vec4 globalAmbient = vec4(0.0, 0.0, 0.0, 1.0);

uniform sampler2D u_samplerPosition;
uniform sampler2D u_samplerNormal;
uniform sampler2D u_samplerAlbedo;

struct PointLight
{
    vec4 position;      // pos.w = radius
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

layout(std140) uniform ViewProj
{
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;
    mat4 u_inverseProjectionMatrix;
};

uniform vec2 u_invScreenSize;
uniform int u_lightIndex;

layout(std140) uniform Lights
{
    PointLight u_Lights[128];
};

layout(std140) uniform Material
{
    vec4 emissive;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;  // specular.w = shininess	
} material;

in vec3 viewDirVS;

out vec4 Fragment;

//
// L'illumination s'effectue en passes multiples mais l'avantage par rapport a un rendu forward multipass
// standard est que l'on n'affiche qu'un seul objet par lumiere, une representation simplifiee de celle-ci.
// Notez qu'actuellement, pour une point light, on se sert d'une sphere 3D relativement bien detaillee.
// Ce niveau de detail est inutile car comme vous pouvoiez le voir dans l'equation, l'illumination est calculee
// de maniere implicite en tenant compte du radius de la lumiere ponctuelle et de la distance au fragment.
// Du coup on pourrait tres bien remplacer l'affichage de la sphere par un simple carre et on aurait le meme rendu.
//
vec4 CalcPointLight(int index, vec3 positionVS, vec3 N)
{
    float radius = u_Lights[index].position.w;
    vec3 L = (u_Lights[index].position.xyz - positionVS) / radius;
    float attenuation = clamp(1.0 - dot(L, L), 0.0, 1.0);
    L = normalize(L);
    vec3 V = viewDirVS;//normalize(-positionVS); 
    vec3 H = normalize(L + V);
    
    // calcul du cosinus de l'angle entre les deux vecteurs
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float shininess = material.specular.w;

    // le principal probleme du modele Phong, et donc aussi Blinn-Phong concerne la conservation de l'energie
    // lorsque le coefficient "shininess" est faible le modele semble ajouter de l'energie et en perd lorsque
    // le coefficient est plus eleve. 
    // Pour le modele de Phong, on peut normaliser la contribution speculaire en multipliant par (shininess + 2.0) / (2.0 * PI)
    // et pour le modele Blinn-Phong en multipliant par (shininess + 8.0) / (8.0 * PI)
    float blinnPhongNormalisation = (shininess + 8.0) / (8.0 * PI);

    float specularFactor =  step(0.0, NdotL) * blinnPhongNormalisation * pow(NdotH, shininess);
    float diffuseFactor = NdotL; // * (1.0 - specularFactor)

    return material.ambient * (globalAmbient + (attenuation * u_Lights[index].ambient)) + 
            material.diffuse * u_Lights[index].diffuse * (attenuation * diffuseFactor) +
            vec4(material.specular.xyz, 1.0) * u_Lights[index].specular * (attenuation * specularFactor);

    // note micro optimisation: (float * float) * vec peut etre plus rapide que 
    // float * vec * float et float * (float * vec), voire float * float * vec
}

void main(void)
{    
    // On calcule les coordonnees normalisees d'un pixel des images sources en se servant 
    // de la position actuelle du fragment dans le repere de l'ecran (gl_FragCoord) que l'on va 
    // diviser par les dimensions de l'ecran car le repere de gl_FragCoord est celui du viewport
    // Note: les valeurs de gl_FragCoord sont en fait les positions du "centre" des pixels de l'ecran.
    // Voyez un pixel comme un petit rectangle, le rasterizer dessine generalement le pixel au centre de ce petit rectangle
    // ce qui implique que le domaine de gl_FragCoord est [(0.5, 0.5) - (width-0.5)(height-0.5)]
    vec2 texcoord = gl_FragCoord.xy * u_invScreenSize;

    // on lit le contenu des g-buffers afin de recuperer les donnees necessaires a l'illumination
    // position du fragment et normale tout deux dans le repere de la camera, couleur diffuse du fragment.
    vec3 positionVS = texture(u_samplerPosition, texcoord).xyz;
    vec3 normal = texture(u_samplerNormal, texcoord).xyz;
    vec4 diffuseColor = texture(u_samplerAlbedo, texcoord);

    vec4 color = CalcPointLight(u_lightIndex, positionVS.xyz, normal);
    Fragment = diffuseColor * color;
}