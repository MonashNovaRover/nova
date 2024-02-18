#version 300 es

precision mediump float;
in vec4 aPosition;

uniform float fov;
uniform vec2 mousePos;

out vec2 vRotator;
out vec2 vTexCoord;

const float PI = 3.141592653589793238462643383279502884197169399375105820;

void main() {
    const vec2 aspect = vec2(1.0, 0.75);
    const float mouseScale = 0.002;

    vTexCoord = vec2(aPosition.x, -aPosition.y);
    vRotator = (vTexCoord * aspect * fov * PI / 180.0) - (mousePos * mouseScale);

    gl_Position = aPosition;
}