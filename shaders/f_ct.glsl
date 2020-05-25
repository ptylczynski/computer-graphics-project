#version 330 core
out vec4 FragmentColor;

// texture coordinates
in vec2 TextureCoords;

// normal coordinates
in vec4 VertexNormal;
in vec4 VertexPosition;
// interpolated vector pointing light from vertex

// parameters of material
uniform float metallic;
uniform float roughness;
uniform vec3 ao;
uniform sampler2D textureMap;
uniform sampler2D specularMap;
uniform vec3 cameraPosition;
uniform mat4 M;
uniform vec3 lightPosition[2];

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
    float alpha      = roughness*roughness;
    float alpha2     = alpha*alpha;
    float NdotH  = clamp(dot(N, H), 0, 1);
    float NdotH2 = NdotH*NdotH;
	
    float num   = alpha2;
    float denom = (NdotH2 * (alpha2 - 1.0) + 1.0);
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
    float NdotV = clamp(dot(N, V), 0, 1);
    float NdotL = clamp(dot(N, L), 0, 1);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main()
{	
    vec3 lightColors[] = {
        vec3(1,0.5,0.5),
        vec3(0.5,0.5,1)
    };

    vec3 albedo = texture(textureMap, TextureCoords).xyz;
    vec3 specular = texture(specularMap, TextureCoords).xyz;
    vec3 WorldPos = (M * VertexPosition).xyz; 
    vec3 N = normalize(M * VertexNormal).xyz;
    vec3 V = normalize(cameraPosition - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // equation of reflectance
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 2; ++i) 
    {
        // radiance of each light with attenuation
        vec3 L = normalize(lightPosition[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(lightPosition[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance) * 1200;
        vec3 radiance     = lightColors[i] * attenuation;        
        
        // cook-torrance element
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        // reflection to refraction
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        // grand total of reflectance
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    FragmentColor = vec4(color, 1.0);
} 