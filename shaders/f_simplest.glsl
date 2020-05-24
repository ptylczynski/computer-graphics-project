#version 330

// _N_[a-zA-Z0-9] normalized value

uniform sampler2D textureMap;
uniform sampler2D specularMap;

// final pixel color
out vec4 pixelColor;

// general
in vec2 iTextureCoord;
in vec4 _vertexNormal;
in vec4 _toObserver;

// for light 1
in vec4 _toLight1;
in float distanceToLight1;

// for light 2
in vec4 _toLight2;
in float distanceToLight2;

void main(void) {
	// normalized vector from vertex to light
	vec4 _N_toLight1 = normalize(_toLight1);
	vec4 _N_toLight2 = normalize(_toLight2);

	// normalized vector from light to vertex
	vec4 _N_fromLight1 = -_N_toLight1;
	vec4 _N_fromLight2 = -_N_toLight2;

	// normalized normal vector
	vec4 _N_normal = normalize(_vertexNormal);

	// normalized vector from vertex to observer
	vec4 _N_toObserver = normalize(_toObserver);
	// normalized reflected vector of light ray
	vec4 _N_refkectedLight1 = reflect(_N_fromLight1, _N_normal);
	vec4 _N_refkectedLight2 = reflect(_N_fromLight2, _N_normal);

	// color of albedo
	vec4 albedo = texture(textureMap, iTextureCoord);

	// color of reflection dot
	vec4 reflectionDotColor = texture(specularMap, iTextureCoord);

	float normalToLight1Cosine = dot(_N_normal, _N_toLight1);
	float normalToLight2Cosine = dot(_N_normal, _N_toLight2);

	float _N_normalToLight1Cosine = clamp(normalToLight1Cosine, 0 ,1);
	float _N_normalToLight2Cosine = clamp(normalToLight2Cosine, 0 ,1);


	float reflectedToObserverCosine1 = pow(clamp(dot(_N_refkectedLight1, _N_toObserver), 0, 1), 150);
	float reflectedToObserverCosine2 = pow(clamp(dot(_N_refkectedLight2, _N_toObserver), 0, 1), 150);
	float _N_reflectedToObserverCosine1 = clamp(reflectedToObserverCosine1, 0, 1);
	float _N_reflectedToObserverCosine2 = clamp(reflectedToObserverCosine2, 0, 1);

	float distance1Coeff = 1 / pow(distanceToLight1, 2);
	float distance2Coeff = 1 / pow(distanceToLight2, 2);
	float _N_distance1Coeff = distance1Coeff * 600; 
	float _N_distance2Coeff = distance2Coeff * 600; 
		
	vec4 pixelColor1 = 
		vec4(albedo.r * _N_normalToLight1Cosine * _N_distance1Coeff * 1.3, albedo.gb * _N_normalToLight1Cosine * _N_distance1Coeff * 0.8, albedo.a) +
		vec4(reflectionDotColor.rgb*_N_reflectedToObserverCosine1 * _N_distance1Coeff, 0);

	vec4 pixelColor2 = 
		vec4(albedo.rg * _N_normalToLight2Cosine * _N_distance2Coeff * 0.8, albedo.b * _N_normalToLight2Cosine * _N_distance2Coeff * 1.4, albedo.a) +
		vec4(reflectionDotColor.rgb*_N_reflectedToObserverCosine2 * _N_distance2Coeff, 0);
	
	vec4 shadowColor = vec4(0.1, 0.1, 0.1, 1);
	if(_N_normalToLight1Cosine < 0.1) shadowColor = vec4(0.1, 0, 0, 1);
	if(_N_normalToLight2Cosine < 0.1) shadowColor = vec4(0, 0, 0.1, 1);

	pixelColor = pixelColor1 + pixelColor2 + shadowColor;
}
