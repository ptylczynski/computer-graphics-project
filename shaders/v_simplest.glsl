#version 330

// _[a-zA-Z0-9] normalized vector

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 light1Position;
uniform vec4 light2Position;

//Atrybuty
in vec4 vertexPosition;
in vec4 vertexNormal;
in vec2 textureCoord;

// general
out vec2 iTextureCoord;
out vec4 _toObserver;
out vec4 _vertexNormal;

// for light 1
out vec4 _toLight1;
out float distanceToLight1;

// for light 2
out vec4 _toLight2;
out float distanceToLight2;

vec4 toViewerCoord(vec4 a){
    return V * M * a;
}

void main(void) {
    // vector from vertex position to light position
    vec4 toLight1 = V * light1Position - toViewerCoord(vertexPosition);
    _toLight1 = normalize(toLight1);

     vec4 toLight2 = V * light2Position - toViewerCoord(vertexPosition);
    _toLight2 = normalize(toLight2);

    // vector from vertex position to viewer position
    // viewer position in viewer coord is coord system origin
    vec4 toObserver = vec4(0, 0, 0, 1) - toViewerCoord(vertexPosition);
    _toObserver = normalize(toObserver);

    // normal in viewer coord
    vec4 VertexNormal = toViewerCoord(vertexNormal);
    _vertexNormal = normalize(VertexNormal);

    distanceToLight1 = distance(M * vertexPosition, light1Position);
    distanceToLight2 = distance(M * vertexPosition, light2Position);

    iTextureCoord = textureCoord;
    gl_Position=P * toViewerCoord(vertexPosition);
}
