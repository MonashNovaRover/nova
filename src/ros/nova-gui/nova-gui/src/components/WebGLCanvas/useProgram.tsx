import {useEffect, useLayoutEffect, useState} from "react";
import initShaderProgram from "./initShaderProgram.tsx";


export function useProgram( gl: WebGL2RenderingContext | undefined, vert: string, frag: string ) {

  // const { createProgram, removeProgram } = useShader({ gl, showDebugInfo });

  const [program, setProgram] = useState<WebGLProgram | undefined>();

  useLayoutEffect(() => {
    if (!gl)
      return;

    const program = initShaderProgram(gl, vert, frag);

    if (!program)
      return;

    setProgram(program);
  }, [gl, vert, frag]);

  useEffect(() => {
    if (!gl || !program)
      return;

    gl.useProgram(program);

    console.log("Setting shader program...");
  }, [gl, program]);

  return program;
}