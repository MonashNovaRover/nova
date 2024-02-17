const Frag = `#version 300 es

precision mediump float;
//in vec4 v_color;
in vec2 uv;
in vec2 v_texCoord;
out vec4 fragColor;

uniform float time;
uniform vec2 mousePos;

uniform sampler2D webcam;
//uniform sampler2D rover; 



void main() {
  
  
  //vec4 roverCol = texture(webcam, v_texCoord);
  //roverCol = vec4(roverCol.xyz * roverCol.w, roverCol.w); 
  
  float scale = 4.0;
  float speed = 1.0;
  vec2 velocityOffset = vec2(time) * speed;
  
  vec4 webcamCol = texture(webcam, scale * v_texCoord - 0.001 * mousePos + velocityOffset);
  webcamCol = vec4(webcamCol.xyz * webcamCol.w, webcamCol.w); 
  //webcamCol = (webcamCol * vec4(col, 1.0));
  
  // Time varying pixel color
  vec3 col = 0.6 + 0.4 * cos(vec3(4.0*time) + vec3(velocityOffset, time*speed) + vec3(v_texCoord.xy, 2.0*v_texCoord.x + v_texCoord.y)* 8.0 + vec3(0,2,4));
   
  
  
  //vec4 mixImageCol = webcamCol + roverCol; // mix(webcamCol, roverCol, roverCol.w);

  // Output to screen
  fragColor = webcamCol * vec4(col, 1.0);
  // texture ((vec4(v_texCoord, 0.0, 1.0) + vec4(col, 1.0)) * 0.5);
}
`
export default Frag;