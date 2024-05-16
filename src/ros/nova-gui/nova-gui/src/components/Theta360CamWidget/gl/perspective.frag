#version 300 es

precision mediump float;
in vec3 vViewDir;
in vec2 vTexCoord;

uniform vec2 mousePos;
uniform sampler2D camera;

out vec4 fragColor;

/**
 *  Takes a direction, and turns it into texture coordinates for an equirectangular texture
 *  @param dir a unit vector for the view direction, where z is the vertical axis
 *  @returns a equirectangular projection of the input vector
 */
vec2 equirectangular(vec3 dir) {
    const float PI = 3.141592653589793238462643383279502884197169399375105820;

    vec2 flattenedDir = normalize(vec2(dir.x, dir.y));
    float lateralAngle = acos(flattenedDir.y) * sign(dir.x);
    if (flattenedDir.y >= 1.) {
        lateralAngle = 0.;  // Fix seam at flattenedDir.y == 1
    }

    return vec2(
        0.5 + lateralAngle / (2.0 * PI),
        0.5 + 0.5 * dir.z
    );
}

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

const float offset = 0.01;

vec2 fisheye(vec2 pos) {
    vec2 b[2];

    if (pos.x < 0.5) {
        return lb[0] + pos * (lb[1] - lb[0]) * vec2(2.,1.);
    } else {
        return rb[0] + (pos - vec2(.5, 0.)) * (rb[1] - rb[0]) * vec2(2.,1.) + vec2(.5, 0.);
    }
}

vec2 dir_to_spherical(vec3 dir) {
    return vec2(.5) + .5 * vec2(-(.5 + .5*dir.y) * sign(dir.x), dir.z);
}

void main() {
    // Get the view direction from the vertex shader
    vec3 viewDir = normalize(vViewDir);

    // Sample at the projected point
    vec4 webcamCol = texture(camera, equirectangular(viewDir), 0.0);

    // Output to screen
    fragColor = webcamCol;
}