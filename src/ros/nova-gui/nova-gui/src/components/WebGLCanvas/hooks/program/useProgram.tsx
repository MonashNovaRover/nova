import {useEffect, useRef} from "react";
import {CanvasWithGL} from "../gl/useGL.tsx";
import GLProgramState from "./GLProgramState.ts";


function useProgram_aux(gl: CanvasWithGL, vert: string, frag: string, numberOfVertices: number): GLProgramState {
  const programRef = useRef<GLProgramState>();

  if (programRef.current === undefined)
    programRef.current = new GLProgramState(gl, vert, frag, numberOfVertices);

  return programRef.current;
}

/**
 * Compiles and uses a webgl program given the vertex and fragment shader source code.
 * @param gl The rendering context to use
 * @param vert The vertex shader source code
 * @param frag The fragment shader source code
 * @param numberOfVertices The number of vertices to render when calling draw arrays
 */
export function useProgram(gl: CanvasWithGL, vert: string, frag: string, numberOfVertices: number = 4) {
  const program = useProgram_aux(gl, vert, frag, numberOfVertices);

  useEffect(() => {
    program.numberOfVertices = numberOfVertices;
  }, [numberOfVertices, program]);

  useEffect(() => {
    program.setShaders(gl, vert, frag);
  }, [vert, frag, program, gl]);

  return program;
}