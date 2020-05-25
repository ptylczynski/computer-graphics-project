#version 330

// _[a-zA-Z0-9] normalized vector
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

in vec4 vertexPosition;
in vec4 vertexNormal;
in vec2 textureCoord;

out vec4 VertexPosition;
out vec4 VertexNormal;
out vec2 TextureCoords;

vec3 toWorldCoord(vec3 a){
    return (M * vec4(a,1)).xyz;
}

void main(void) {
    VertexNormal = vertexNormal;
    VertexPosition = vertexPosition;
    TextureCoords = textureCoord;
    gl_Position=P * V * M *vertexPosition;
}
