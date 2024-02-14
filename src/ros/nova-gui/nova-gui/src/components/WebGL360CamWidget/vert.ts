const Vert = `#version 300 es

precision mediump float;
in vec4 a_position;
uniform float time;
//in vec4 a_color;
//out vec4 v_color;

out vec2 uv;

void main() {
  //v_color = a_color;
  
  // transform the positions to make the frame spin in a circle
  gl_Position = vec4(a_position.x * 0.5, a_position.y * 0.5, a_position.zw)+ vec4(cos(time) * 0.5,sin(time) * 0.5,0,0);
  
  
  
  uv = (a_position.xy + vec2(1.0, 1.0)) / 2.0;
}
`
export default Vert;

