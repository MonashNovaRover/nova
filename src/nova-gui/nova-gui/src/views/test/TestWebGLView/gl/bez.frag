#version 300 es

precision mediump float;
in vec3 vIndex;

out vec4 fragColor;

void main() {

    fragColor = vec4(0.8, 1.0, 0.8, 1.0);
    fragColor = vec4(vIndex, 1.0);

    float valueFromX = 2. * vIndex.x * vIndex.z;
    if (valueFromX < vIndex.y)
            discard;
}




