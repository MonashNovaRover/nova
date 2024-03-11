#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D camera;
uniform float threshold;

out vec4 fragColor;

void main() {
    // Sample at the projected point
    vec4 texCol = texture(camera, vTexCoord);

    if (texCol.x + texCol.y + texCol.z < 3.0*threshold) {
        fragColor = vec4(vec3(0.0), 1.0);
    }
    else {
        fragColor = vec4(1.0);
    }

}
