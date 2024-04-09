import {useEffect, useMemo, useRef} from "react";
import initShaderProgram from "../../webgl-utils/initShaderProgram.ts";
import {CanvasWithGL} from "../useGL.tsx";
import useEffectQueue, {EffectQueue} from "../effectQueue/useEffectQueue.ts";

export interface ProgramWithGL {
  program: WebGLProgram,
  gl: WebGL2RenderingContext,
  queue: EffectQueue,
}

/**
 * Compiles and uses a webgl program given the vertex and fragment shader source code.
 * @param gl The rendering context to use
 * @param vert The vertex shader source code
 * @param frag The fragment shader source code
 */
export function useProgram(gl: CanvasWithGL, vert: string, frag: string) {
  // const [program, setProgram] = useState<WebGLProgram | undefined>();
  const programRef = useRef<WebGLProgram | undefined>();

  useEffect(() => {
    if (!gl.gl)
      return;

    const newProgram = initShaderProgram(gl.gl, vert, frag);

    if (!newProgram)
      return;

    if (programRef.current !== undefined)
      console.log("Recompiled shader program.");

    gl.gl.useProgram(newProgram);
    programRef.current = newProgram;
  }, [gl, vert, frag]);

  const queue = useEffectQueue();

  return useMemo(() => {
    if (!gl.gl || !programRef.current)
      return;

    return {
      program: programRef.current,
      gl: gl.gl,
      queue: queue,
    } as ProgramWithGL
  }, [gl.gl, queue]);
}