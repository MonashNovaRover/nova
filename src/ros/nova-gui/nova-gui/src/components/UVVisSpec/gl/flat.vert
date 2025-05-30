#version 300 es

precision mediump float;
in vec4 aPosition;

void main() {
    gl_Position = aPosition;
}