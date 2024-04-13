import {useEffect, useRef} from "react";
import GLProgramState, {GLProgramStateOptions} from "./GLProgramState.ts";
import GLProgramDrawMode from "./GLProgramDrawMode.ts";
import GLState from "../gl/GLState.ts";

function useProgram_aux(gl: GLState, vert: string, frag: string, options: GLProgramStateOptions): GLProgramState {
  const programRef = useRef<GLProgramState>();

  if (programRef.current === undefined)
    programRef.current = new GLProgramState(gl, vert, frag, options);

  return programRef.current;
}

/**
 * Compiles and uses a webgl program given the vertex and fragment shader source code.
 * @param gl The rendering context to use
 * @param vert The vertex shader source code
 * @param frag The fragment shader source code
 * @param options Additional settings for the draw mode
 */
export function useProgram(gl: GLState, vert: string, frag: string, options?: Partial<GLProgramStateOptions>) {
  const filledOptions = {
    numberOfVertices: 4,
    drawMode: GLProgramDrawMode.TRIANGLE_STRIP,
    ...options,
  }

  const program = useProgram_aux(gl, vert, frag, filledOptions);

  useEffect(() => {
    program.numberOfVertices = filledOptions.numberOfVertices;
  }, [filledOptions.numberOfVertices, program]);

  useEffect(() => {
    program.drawMode = filledOptions.drawMode;
  }, [filledOptions.drawMode, program]);

  useEffect(() => {
    program.setShaders(gl, vert, frag);
  }, [vert, frag, program, gl]);

  return program;
}