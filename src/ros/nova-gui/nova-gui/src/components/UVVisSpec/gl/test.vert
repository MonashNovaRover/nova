#version 300 es

precision mediump float;
in vec4 aPosition;
//in float aLuminance;

//out vec2 vTexCoord;
//out float vLuminance;

void main() {
    gl_Position = aPosition;
    // vTexCoord = vec2(0.5) + 0.5*aPosition.xy;
    // vLuminance = aLuminance;

}