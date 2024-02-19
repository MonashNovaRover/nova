#version 300 es

precision mediump float;
in vec3 vViewDir;

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

    return vec2(
        0.5 + lateralAngle / (2.0 * PI),
        0.5 + 0.5 * dir.z
    );
}

void main() {
    // Get the view direction from the vertex shader
    vec3 viewDir = normalize(vViewDir);

    // Sample at the projected point
    vec4 webcamCol = texture(camera, equirectangular(viewDir), 0.0);

    // Output to screen
    fragColor = webcamCol;
}
