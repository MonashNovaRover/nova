#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D image;
uniform float count;

out vec4 fragColor;

void main() {
    vec4 texCol = texture(image, vec2(1. - (0.1*count) - vTexCoord.x, vTexCoord.y));
    texCol *= vec4(vec3(texCol.a), 1.);

    fragColor = texCol;
}
