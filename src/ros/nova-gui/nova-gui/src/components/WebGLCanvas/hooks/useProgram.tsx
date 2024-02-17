import {useLayoutEffect, useState} from "react";
import initShaderProgram from "../webgl-utils/initShaderProgram.ts";

export function useProgram( gl: WebGL2RenderingContext | undefined, vert: string, frag: string ) {

  const [program, setProgram] = useState<WebGLProgram | undefined>();

  useLayoutEffect(() => {
    if (!gl)
      return;

    const newProgram = initShaderProgram(gl, vert, frag);

    if (!newProgram)
      return;

    if (program)
      console.log("Recompiled shader program.")

    gl.useProgram(newProgram);
    setProgram(newProgram);
  }, [gl, vert, frag]);

  return program;
}