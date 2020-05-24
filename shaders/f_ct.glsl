#version 330 core
out vec4 FragColor;

// texture coordinates
in vec2 TexCoords;

// normal coordinates
in vec3 Normal;

// interpolated vector pointing light from vertex
in vec3 ToLight[2];

// interpolated vector pointing camera from vertex
in vec3 ToCamera;

// parameters of material
uniform float metallic;
uniform float roughness;
uniform float ao;
uniform sampler2D textureMap;
uniform sampler2D specularMap;

const float PI = 3.14159265359;
 
// pron: /freh-nel/
// describe proportion of light reflected to refracted
// varying on angle of observeation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

// approximation of normals pointing towards halfway vector (H)
// as alpha used roughness
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

// describe amount of surface unvisible due
// to micro-structur of material covering or obstaackling itself
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

// smith equation utilising SchlikGGC to address both
// number of facets covered by each other (L part) 
// and by geometry shadowing (V part)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main()
{		
    vec3 lightColors[2] = {
        vec3(1,0.5,0.5),
        vec3(0.5,0.5,1)
    };

    vec3 albedo = texture(textureMap, TexCoords).xyz;
    vec3 reflectionColor = texture(specularMap, TexCoords).xyz;
    vec3 N = normalize(Normal);
    vec3 V = normalize(ToCamera);

    // each material has its own fresnel coefficient, however
    // variation of them for non conducting materials is close
    // to zero, so assumption of equal (approx. 0.04 value) 
    // makes do and hold
    // for metalic materials which range of occurence is wide
    // interpolation of coefficient is crucial
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // integral over irradiation -> total refraction over all
    // light source
    // works perfectly if any emitive materials used
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 2; ++i) 
    {
        vec3 L = normalize(ToLight[i]);
        vec3 H = normalize(L + V);

        // total radiance - light througcomming solid angle and alighting
        // surface. Both solid angle and surfece are small enought to be
        // one pixel and single ray - vector from light
        float distance    = length(ToLight[i]);
        float attenuation = 1.0 / (distance * distance) * 600;
        vec3 radiance     = lightColors[i] * attenuation;        
        
        // cook-torrance brdm
        // define contriution of each ray to final reflection value
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F * reflectionColor;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        float NdotL = max(dot(N, L), 0.0);
        // grand total 
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
    // adding ambient color
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    // adjustement of gamma (?)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    FragColor = vec4(color, 1.0);
}  