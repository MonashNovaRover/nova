import {useLayoutEffect, useState} from "react";
import initShaderProgram from "../webgl-utils/initShaderProgram.ts";
import {CanvasWithGL} from "./useGL.tsx";

/**
 * Compiles and uses a webgl program given the vertex and fragment shader source code.
 * @param gl The rendering context to use
 * @param vert The vertex shader source code
 * @param frag The fragment shader source code
 */
export function useProgram(gl: CanvasWithGL, vert: string, frag: string) {
  const [program, setProgram] = useState<WebGLProgram | undefined>();

  useLayoutEffect(() => {
    if (!gl.gl)
      return;

    const newProgram = initShaderProgram(gl.gl, vert, frag);

    if (!newProgram)
      return;

    if (program)
      console.log("Recompiled shader program.")

    gl.gl.useProgram(newProgram);
    setProgram(newProgram);
  }, [gl, vert, frag]);

  return program;
}