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


// Left fisheye bounds
const vec2 lb[2] = vec2[2](
    vec2(0.01, 0.1395),
    vec2(0.4715, 0.985)
);

// Right fisheye bounds
const vec2 rb[2] = vec2[2](
    vec2(0.028, 0.135),
    vec2(0.4915, 0.984)
);

vec2 fisheye(vec2 pos) {
    vec2 b[2];

    if (pos.x < 0.5) {
        return lb[0] + pos * (lb[1] - lb[0]) * vec2(2.,1.);
    } else {
        return rb[0] + (pos - vec2(.5, 0.)) * (rb[1] - rb[0]) * vec2(2.,1.) + vec2(.5, 0.);
    }
}

/**
 *  Creates a maxtrix for rotating about the X axis, then about the Z axis.
 *  @param rot a vector where the first component is the rotation about Z and the second is the rotation about X.
 */
mat3 eulerXZ(vec2 rot) {
    vec2 c = cos(rot);
    vec2 s = sin(rot);

    return mat3(
        c.x   ,  -s.x  ,  0.0,
        s.x * c.y, c.x*c.y, -s.y,
        s.x * s.y, c.x*s.y,  c.y
    );
}

vec2 dir_to_spherical(vec3 dir) {
    return vec2(.5) + .5 * vec2(-(.5 + .5*dir.y) * sign(dir.x), dir.z);
}

void main() {
    vec2 aspect = resolution / max(resolution.x, resolution.y);

    // Just convert the angle directly to a sample point. No need to do any actual equirectangular projection.
    vec2 fastTexCoord = vec2(
        mod(0.5 + (vRotator.x / (2.0 * PI)), 1.0),
        0.5 + 0.5 * sin(vRotator.y)
    );
    vec2 compassCoord = vec2(
        mod((vRotator.x - compassAngle) / (2.*PI), 1.0),
        1.0 - aspect.y * (1.0 - (vTexCoord.y)) - 0.5
    );

    vec4 cameraCol = texture(camera, fastTexCoord);
    vec4 compassCol = texture(compass, compassCoord);

    vec3 dir = eulerXZ(vRotator) * vec3(0., 1., 0.);

    compassCol = vec4(compassCol.xyz * compassCol.w, compassCol.w);

    vec4 inverted = vec4(1.0) - cameraCol;

    // Sample at the projected point
    fragColor = vec4((mix(inverted.xyz, cameraCol.xyz, compassCol.w)).xyz, 1.0);



    // fragColor = texture(camera, fisheye(dir_to_spherical(0.98*dir)), 0.0);

}