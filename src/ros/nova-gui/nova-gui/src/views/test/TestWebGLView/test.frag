#version 300 es

precision mediump float;
in vec2 vTexCoord;

//uniform sampler2D image;
//uniform vec2 offset;

out vec4 fragColor;

void main() {
    // vec2 samplePoint = vTexCoord + offset;
    // samplePoint = vec2(mod(samplePoint.x, 1.0), mod(samplePoint.y, 1.0));

    // texture(image, samplePoint),
    // Sample at the projected point
    vec4 texCol = (vec4(1.0)) * 0.5;

    fragColor = texCol;
}
