#version 300 es

precision mediump float;
in vec2 vTexCoord;
in vec2 vRotator;

uniform float fov;
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
    const vec2 aspect = vec2(1.0, 0.75);
    const float mouseScale = 0.002;

    // These are euler angle for the (x, z) axes.
    vec2 rotator = vRotator;//(vTexCoord * aspect * fov * PI / 360.0) - (mousePos * mouseScale);

    vec2 c = cos(rotator);
    vec2 s = sin(rotator);

    mat3 rotation = mat3(
        c.x   ,  -s.x  ,  0.0,
        s.x * c.y, c.x*c.y, -s.y,
        s.x * s.y, c.x*s.y,  c.y
    );


    vec3 panoramaDir = rotation * vec3(0.0, 1.0, 0.0);

    vec2 texCoord = equirectangular(panoramaDir);


    // Sample at the projected point
    vec4 webcamCol = texture(camera, texCoord, 0.0);

    // Output to screen
    fragColor = webcamCol;
}
