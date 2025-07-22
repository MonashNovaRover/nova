#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D image;
uniform float threshold;

out vec4 fragColor;

void main() {
    // Sample at the projected point
    vec4 texCol = texture(image, vTexCoord);

    float brightness = (texCol.x + texCol.y + texCol.z) / 3.0;

    if (brightness > threshold)
        fragColor = vec4(vec3(1.0), 1.0);
    else if (brightness == threshold)
        fragColor = vec4(vec3(0.5), 1.0);
    else
        fragColor = vec4(vec3(0.0), 1.0);
}
