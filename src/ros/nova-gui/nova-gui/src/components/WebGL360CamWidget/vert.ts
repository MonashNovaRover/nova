const Vert = `#version 300 es

precision highp float;
in vec4 a_position;

void main() {
  gl_Position = a_position;
}
`
export default Vert;