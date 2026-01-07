



//
// Initialize a shader program, so WebGL knows how to draw our data
//
export default function initShaderProgram(gl: WebGL2RenderingContext | undefined, vsSource: string, fsSource: string) {
  if (gl === undefined)
    return;

  const vertexShader = loadVertexShader(gl, vsSource);
  const fragmentShader = loadFragmentShader(gl, fsSource);

  // Create the shader program
  const shaderProgram = gl.createProgram();

  if (shaderProgram === null || vertexShader === undefined || fragmentShader === undefined)
    return undefined;

  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  // If creating the shader program failed, alert
  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert(
      `Unable to initialize the shader program:\n\t ${gl.getProgramInfoLog(
        shaderProgram,
      )}. `,
    );
    return undefined;
  }

  return shaderProgram;
}


export const loadVertexShader = (gl : WebGL2RenderingContext, source : string) =>
  loadShader(gl, gl.VERTEX_SHADER, source);

export const loadFragmentShader = (gl : WebGL2RenderingContext, source : string) =>
  loadShader(gl, gl.FRAGMENT_SHADER, source);

//
// creates a shader of the given type, uploads the source and
// compiles it.
//
export function loadShader(gl: WebGL2RenderingContext, type: number, source: string) {
  const shader = gl.createShader(type);

  // Exit early when null
  if (shader === null)
    return undefined;

  // Send the source to the shader object

  gl.shaderSource(shader, source.trimStart());

  // Compile the shader program

  gl.compileShader(shader);

  // See if it compiled successfully

  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    const errorMsg = gl.getShaderInfoLog(shader);
    const typeString = type === gl.VERTEX_SHADER ? "vertex"
      : type === gl.FRAGMENT_SHADER ? "fragment" : `unknown (${type.toString()})`;

    console.error(
      `An error occurred compiling the shaders (type ${typeString}):`,
      '\n', "Error Message: ", errorMsg,
      '\n', "Source: ", source,
      '\n', "VERSION: ", gl.getParameter(gl.VERSION),
      '\n', "SHADING_LANGUAGE_VERSION: ", gl.getParameter(gl.SHADING_LANGUAGE_VERSION)
    );
    console.error(errorMsg);

    gl.deleteShader(shader);
    return undefined;
  }

  return shader;
}