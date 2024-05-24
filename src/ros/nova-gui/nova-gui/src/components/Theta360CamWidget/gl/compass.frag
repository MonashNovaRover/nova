#version 300 es

precision mediump float;
in vec2 vTexCoord;
in vec2 vRotator;

uniform vec2 mousePos;
uniform vec2 resolution;
uniform float compassAngle;

uniform sampler2D camera;
uniform sampler2D compass;

out vec4 fragColor;

const float PI = 3.141592653589793238462643383279502884197169399375105820;

void main() {
    vec2 aspect = resolution / max(resolution.x, resolution.y);

    // Just convert the angle directly to a sample point. No need to do any actual equirectangular projection.
    vec2 compassCoord = vec2(
        mod((vRotator.x - compassAngle) / (2.*PI), 1.0),
        1.0 - aspect.y * (1.0 - (vTexCoord.y)) - 0.5
    );

    vec4 compassCol = texture(compass, compassCoord);

    // Threshold
    if (compassCol.w < 0.5) {
        discard;
    }

    fragColor = vec4(compassCol.xyz * compassCol.w, compassCol.w);
}