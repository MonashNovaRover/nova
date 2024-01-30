import {memo, useEffect, useRef} from "react";

//
// Initialize a shader program, so WebGL knows how to draw our data
//
function initShaderProgram(gl: WebGLRenderingContext, vsSource: string, fsSource: string) {
  const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
  const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

  // Create the shader program

  const shaderProgram = gl.createProgram();

  if (shaderProgram === null || vertexShader === null || fragmentShader === null)
    return shaderProgram;

  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  // If creating the shader program failed, alert

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert(
      `Unable to initialize the shader program: ${gl.getProgramInfoLog(
        shaderProgram,
      )}`,
    );
    return null;
  }

  return shaderProgram;
}

//
// creates a shader of the given type, uploads the source and
// compiles it.
//
function loadShader(gl: WebGLRenderingContext, type: number, source: string) {
  const shader = gl.createShader(type);
  
  // Exit early when null
  if (shader === null) 
    return shader;

  // Send the source to the shader object

  gl.shaderSource(shader, source);

  // Compile the shader program

  gl.compileShader(shader);

  // See if it compiled successfully

  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    alert(
      `An error occurred compiling the shaders: ${gl.getShaderInfoLog(shader)}`,
    );
    gl.deleteShader(shader);
    return null;
  }

  return shader;
}


export interface IWebGLCanvasProps {
  vert: string,
  frag: string,
  uniformFloats: {[Name: string]: number}
}

const UnmemoedWebGLCanvas: React.FC<IWebGLCanvasProps> = ({
  vert, frag, uniformFloats
}) => {
  const canvas = useRef<HTMLCanvasElement>(null);
  const gl = canvas.current?.getContext("webgl") ?? null;

  const isSupported = gl !== null;

  useEffect(() => {
    if (!isSupported)
      return;

    // Compile shader program
    // const shaderProgram = initShaderProgram(gl, vert, frag);

    // Set clear color to black, fully opaque
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    // Clear the color buffer with specified clear color
    gl.clear(gl.COLOR_BUFFER_BIT);

  }, [vert, frag]);

  useEffect(() => {
    // Update program uniforms


  }, [uniformFloats]);

  return (<>{canvas.current}</>) /*: (
    <div>
      Unable to initialize WebGL. <br/>Your browser or machine may not support it.
    </div>
  )*/

}

const WebGLCanvas = memo(UnmemoedWebGLCanvas);
export default WebGLCanvas;