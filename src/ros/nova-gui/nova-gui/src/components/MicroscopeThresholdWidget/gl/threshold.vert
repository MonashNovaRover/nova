#version 300 es

precision mediump float;
in vec4 aPosition;

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
    gl_Position = aPosition;
    vTexCoord = vec2(0.5) + 0.5*aPosition.xy;
}