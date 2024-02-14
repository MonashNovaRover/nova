const Frag = `#version 300 es

precision highp float;
in vec3 vColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
`
export default Frag;