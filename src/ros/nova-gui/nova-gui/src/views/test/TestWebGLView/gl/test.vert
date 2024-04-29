#version 300 es

precision mediump float;
in vec4 aPosition;
in vec4 aLinePosition;

out vec2 vTexCoord;

void main() {
    gl_Position = aPosition;
    vTexCoord = vec2(0.5) + 0.5*aPosition.xy;
}