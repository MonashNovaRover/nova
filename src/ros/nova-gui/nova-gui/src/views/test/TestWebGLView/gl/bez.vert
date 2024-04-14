#version 300 es

precision mediump float;
in vec4 aPosition;
in vec3 index;

out vec2 vTexCoord;
out vec3 vIndex;

void main() {
    gl_Position = aPosition;
    vTexCoord = vec2(0.5) + 0.5*aPosition.xy;
    vIndex = index;
}
