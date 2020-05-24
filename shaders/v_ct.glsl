#version 330

// _[a-zA-Z0-9] normalized vector
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPosition[2];
uniform vec3 cameraPosition;

// model coord
in vec4 vertexPosition;
in vec4 vertexNormal;
in vec2 textureCoord;

// general

// texture coordinates
out vec2 TexCoords;

// normal coordinates
out vec3 Normal;

// vector from pointing light from vertex
out vec3 ToLight[2];

// vertex position in world coordinates
out vec3 WorldPos;

// vector pointing camera from vertex
out vec3 ToCamera;

vec3 toWorldCoord(vec3 a){
    return (M * vec4(a,1)).xyz;
}

void main(void) {
    WorldPos = toWorldCoord(vertexPosition.xyz);
    Normal = toWorldCoord(vertexNormal.xyz);
    TexCoords = textureCoord;
    for(int i = 0; i < 2; i++){
        ToLight[i] = lightPosition[i] - WorldPos;
    }
    ToCamera = cameraPosition - WorldPos;
    gl_Position=P * V * M *vertexPosition;
}
