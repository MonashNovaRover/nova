#version 300 es

precision mediump float;
in vec3 vWavelengthColor;
uniform vec4 uColor;

out vec4 fragColor;

void main() {
    vec4 vWavelengthColorRGBA = vec4(vWavelengthColor, 0.0);
    fragColor = uColor + vWavelengthColorRGBA;
}
