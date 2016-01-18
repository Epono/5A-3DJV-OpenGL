 #version 150

const float PI = 3.14159265;

const vec4 globalAmbient = vec4(0.0, 0.0, 0.0, 1.0);

uniform sampler2D u_sampler;

in Vertex
{
    vec3 normal;
    vec2 texcoords;
    vec3 positionVS;    // VS = View Space
    vec3 viewDirVS;
} IN;

struct PointLight
{
    vec4 position;      // pos.w = radius
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

uniform int u_numLights;

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

out vec4 Fragment;

vec4 CalcPointLight(int index, vec3 N)
{
    float radius = u_Lights[index].position.w;
    vec3 L = (u_Lights[index].position.xyz - IN.positionVS) / radius;
    float attenuation = clamp(1.0 - dot(L, L), 0.0, 1.0);
    L = normalize(L);
    vec3 V = IN.viewDirVS;//normalize(-IN.positionVS); 
    vec3 H = normalize(L+V);

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

    // step(edge, x) retourne 0.0 si x < edge sinon 1.0
    // permet de se passer d'un test, cad, if (NdotL == 0.0) { specularPower = 0.0 }    
    float specularFactor = step(0.0, NdotL) * blinnPhongNormalisation * pow(NdotH, shininess);
    float diffuseFactor = NdotL; // * (1.0 - specularFactor)

    return material.ambient * (globalAmbient + (attenuation * u_Lights[index].ambient)) + 
            material.diffuse * u_Lights[index].diffuse * (attenuation * diffuseFactor) +
            vec4(material.specular.xyz, 1.0) * u_Lights[index].specular * (attenuation * specularFactor);

    // note micro optimisation: (float * float) * vec peut etre plus rapide que 
    // float * vec * float et float * (float * vec), voire float * float * vec
}

void main(void)
{    
	vec3 normal = normalize(IN.normal);
	
    vec4 color = vec4(0.0);

    for (int index = 0; index < u_numLights; ++index)
    {
        color += CalcPointLight(index, normal);
    }
    color *= texture(u_sampler, IN.texcoords);
    Fragment = color;//vec4(pow(color.rgb, vec3(1.0/2.2)), color.a);
}