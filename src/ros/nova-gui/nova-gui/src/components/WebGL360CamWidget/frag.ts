const Frag = `#version 300 es

precision mediump float;
//in vec4 v_color;
in vec2 uv;
in vec2 v_texCoord;
out vec4 fragColor;

uniform float time;
uniform sampler2D webcam;
//uniform sampler2D rover; 



void main() {
  // Time varying pixel color
  vec3 col = 0.5 + 0.5 * cos(time + vec3(v_texCoord.xy, 0.0) + vec3(0,2,4));
  
  //vec4 roverCol = texture(webcam, v_texCoord);
  //roverCol = vec4(roverCol.xyz * roverCol.w, roverCol.w); 
  
  vec4 webcamCol = texture(webcam, v_texCoord);
  webcamCol = vec4(webcamCol.xyz * webcamCol.w, webcamCol.w); 
  webcamCol = (webcamCol * vec4(col, 1.0));
   
  
  
  //vec4 mixImageCol = webcamCol + roverCol; // mix(webcamCol, roverCol, roverCol.w);

  // Output to screen
  fragColor = webcamCol;
  // texture ((vec4(v_texCoord, 0.0, 1.0) + vec4(col, 1.0)) * 0.5);
}
`
export default Frag;