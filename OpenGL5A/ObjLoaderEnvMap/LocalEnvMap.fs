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


// Le principe est simple, on specifie les limites de la bounding box qui va 
// determiner quelles reflections/refractions locales doivent etre prise en compte
// puis on lance un rayon du centre de la bounding box vers 
vec3 CalcLocalReflection(vec3 R, vec3 positionWS)
{
    // Valeur en dur ! Utilisez des variables uniformes a la place 
    // note: la bounding box n'est pas necessairement un cube (parallepipede)
    // ni meme positionnee a l'origine du monde
    const vec3 bboxMin = vec3(-2.0);
    const vec3 bboxMax = vec3(2.0);
    const vec3 bboxCenter = vec3(0.0);

    // calcul de la distance (cf. algorithme d'intersection ray/box)
    vec3 intersectMin = (bboxMin - positionWS) / R;
    vec3 intersectMax = (bboxMax - positionWS) / R;
    vec3 intersect = max(intersectMin, intersectMax);
    float distance = min(min(intersect.x, intersect.y), intersect.z);

    // ray = origin + t*dir
    vec3 intersectWS = positionWS + distance * R;
    // les coordonnees de texture d'une cube map sont exprimees dans le repere local
    // Il s'agit d'une direction relative au centre de la cube map
    return intersectWS - bboxCenter;
}

// note: on peut pousser le concept plus loin en integrant dans une meme cube map les ombres
// Encore plus fort, utiliser les cube maps locales pour des reflexions poussees (comme dans Remember Me par ex.)
// ou pour calculer une light probe (eventuellement en precalculant les coefficients ambient et diffus 
// a l'aide de la technique de Spherical Harmonics, qui permet de recalculer en temps reel l'illumination avec un faible cout gpu)


void main(void)
{
    // Pour une scene exterieure, la composante ambiante peut provenir de la sky box
    // en interieur on pourra sampler localement et/ou globalement selon le rendu desire
    vec3 N = normalize(IN.normal);
    vec4 ambient_env = texture(u_cubeMap, N);

    vec3 cubeReflectDir = normalize(IN.reflectedDir);
    vec3 localR = CalcLocalReflection(IN.positionWS, cubeReflectDir);
    vec4 local_env = texture(u_dynamicCubeMap, localR);    
    vec3 R = CalcLocalReflection(IN.positionWS, cubeReflectDir);
	vec4 env = texture(u_cubeMap, cubeReflectDir);

    // on peut appliquer la meme idee pour obtenir une refraction locale 
    // (je vous laisse faire cela en exercice, quasiment un simple copier/coller)
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
