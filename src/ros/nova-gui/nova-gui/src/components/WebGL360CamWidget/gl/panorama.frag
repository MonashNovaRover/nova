#version 300 es

precision mediump float;
in vec2 vTexCoord;
in vec2 vRotator;

uniform float fov;
uniform vec2 mousePos;
uniform sampler2D camera;

out vec4 fragColor;

const float PI = 3.141592653589793238462643383279502884197169399375105820;

void main() {
    // Just convert the angle directly to a sample point. No need to do any actual equirectangular projection.
    vec2 fastTexCoord = vec2(
        mod(0.5 + (vRotator.x / (2.0 * PI)), 1.0),
        0.5 + 0.5 * sin(vRotator.y)
    );

    // Sample at the projected point
    fragColor = texture(camera, fastTexCoord, 0.0);
}
