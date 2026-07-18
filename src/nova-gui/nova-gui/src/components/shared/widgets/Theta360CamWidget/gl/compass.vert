#version 300 es

precision mediump float;
in vec4 aPosition;

// uniform float fov;
uniform float compassAngle;
uniform vec2 mousePos;
uniform vec2 resolution;

out vec2 vRotator;
out vec2 vTexCoord;

const float PI = 3.141592653589793238462643383279502884197169399375105820;

void main() {
    vec2 aspect = resolution / max(resolution.x, resolution.y);

    vTexCoord = vec2(.5) + .5 * vec2(aPosition.x, aPosition.y);
    vRotator = (aPosition.xy * aspect * PI) - vec2(mousePos.x, -mousePos.y);

    gl_Position = aPosition;
}