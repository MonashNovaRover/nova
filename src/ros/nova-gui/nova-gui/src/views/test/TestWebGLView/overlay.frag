#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D image;

out vec4 fragColor;

void main() {

    vec4 texCol = texture(image, vTexCoord);

    if (texCol.a <= 0.5)
        discard;

    fragColor = texCol;
}
