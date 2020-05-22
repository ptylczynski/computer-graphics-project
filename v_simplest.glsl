#version 330

// _[a-zA-Z0-9] normalized vector

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 lightPosition;

//Atrybuty
in vec4 vertexPosition;
in vec4 vertexNormal;
in vec2 textureCoord;

//Zmienne interpolowane
out vec4 _toLight;
out vec4 _vertexNormal;
out vec4 _toObserver;
out vec2 iTextureCoord;

vec4 toViewerCoord(vec4 a){
    return V * M * a;
}

void main(void) {
    // vector from vertex position to light position
    vec4 toLight = V * lightPosition - toViewerCoord(vertexPosition);
    _toLight = normalize(toLight);

    // vector from vertex position to viewer position
    // viewer position in viewer coord is coord system origin
    vec4 toObserver = vec4(0, 0, 0, 1) - toViewerCoord(vertexPosition);
    _toObserver = normalize(toObserver);

    // normal in viewer coord
    vec4 VertexNormal = toViewerCoord(vertexNormal);
    _vertexNormal = normalize(VertexNormal);

    iTextureCoord = textureCoord;
    gl_Position=P * toViewerCoord(vertexPosition);
}
