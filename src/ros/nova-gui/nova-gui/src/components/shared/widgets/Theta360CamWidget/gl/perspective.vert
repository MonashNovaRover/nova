#version 300 es

precision mediump float;
in vec4 aPosition;
in vec2 aTexCoord;

uniform float fov;
uniform vec2 mousePos;
uniform vec2 resolution;

out vec3 vViewDir;
out vec2 vTexCoord;

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


void main() {
    vec2 aspect = resolution / max(resolution.x, resolution.y);

    // These are euler angle for the (z, x) axes.
    vec2 rotator = -mousePos;
    mat3 rotation = eulerXZ(rotator);

    const float PI = 3.141592653589793238462643383279502884197169399375105820;
    const float DEG_TO_RAD_TIMES_TWO = PI / 360.0;

    float offsetDistance = abs(tan(fov * DEG_TO_RAD_TIMES_TWO));
    vec2 viewDirScreenOffset = aspect * aPosition.xy * offsetDistance;

    vViewDir = rotation * vec3(viewDirScreenOffset.x, 1.0, viewDirScreenOffset.y);
    vTexCoord = vec2(0.5) + 0.5 * vec2(aPosition.x, aPosition.y);

    gl_Position = aPosition;
}