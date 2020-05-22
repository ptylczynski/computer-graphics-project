#version 330

// _N_[a-zA-Z0-9] normalized value

uniform sampler2D textureMap;
uniform sampler2D specularMap;

// final pixel color
out vec4 pixelColor;

in vec2 iTextureCoord;
in vec4 _vertexNormal;
in vec4 _toLight;
in vec4 _toObserver;

void main(void) {
	// normalized vector from vertex to light
	vec4 _N_toLight = normalize(_toLight);

	// normalized vector from light to vertex
	vec4 _N_fromLight = -_N_toLight;

	// normalized normal vector
	vec4 _N_normal = normalize(_vertexNormal);

	// normalized vector from vertex to observer
	vec4 _N_toObserver = normalize(_toObserver);
	// normalized reflected vector of light ray
	vec4 _N_refkectedLight = reflect(_N_fromLight, _N_normal);

	// color of albedo
	vec4 albedo = texture(textureMap, iTextureCoord);

	// color of reflection dot
	vec4 reflectionDotColor = texture(specularMap, iTextureCoord);

	float normalToLightCosine = dot(_N_normal, _N_toLight);
	float _N_normalToLightCosine = clamp(normalToLightCosine, 0 ,1);


	float reflectedToObserverCosine = pow(clamp(dot(_N_refkectedLight, _N_toObserver), 0, 1), 50);
	float _N_reflectedToObserverCosine = clamp(reflectedToObserverCosine, 0, 1);

	pixelColor = 
		vec4(albedo.rgb * _N_normalToLightCosine, albedo.a) +
		vec4(reflectionDotColor.rgb*_N_reflectedToObserverCosine, 0);
}
