#version 300 es

precision mediump float;
in vec4 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    gl_Position = aPosition;
    vTexCoord = aTexCoord;// vec2(0.5) + 0.5*aPosition.xy;
}