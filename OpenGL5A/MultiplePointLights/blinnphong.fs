 #version 150

 // Direction VERS la camera exprimee dans le repere CAMERA
const vec3 E = vec3(0.0, 0.0, 1.0);

const vec4 globalAmbient = vec4(0.0, 0.0, 0.0, 1.0);

uniform sampler2D u_sampler;

in Vertex
{
    vec3 normal;
    vec2 texcoords;
    vec3 positionVS;    // VS = View Space
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
    vec3 H = normalize(L + E);

    // calcul du cosinus de l'angle entre les deux vecteurs
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float specularPower = (NdotL == 0.0) ? 0.0 : pow(NdotH, material.specular.w);

    return material.ambient * (globalAmbient + (attenuation * u_Lights[index].ambient)) + 
            material.diffuse * (attenuation * u_Lights[index].diffuse) +
            vec4(material.specular.xyz, 1.0) * ((attenuation * specularPower) * u_Lights[index].specular);

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

    Fragment = texture(u_sampler, IN.texcoords) * color;
}