const Frag = `#version 300 es

precision mediump float;
//in vec4 v_color;
in vec2 uv;
in vec2 v_texCoord;
out vec4 fragColor;

uniform float time;

uniform sampler2D test_image;

void main() {
  // Time varying pixel color
  vec3 col = 0.5 + 0.5 * cos(time + vec3(v_texCoord.xy, 0.0) + vec3(0,2,4));

  // Output to screen
  fragColor = (vec4(v_texCoord, 0.0, 1.0) + vec4(col, 1.0)) * 0.5;
}
`
export default Frag;