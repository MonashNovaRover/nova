import {useEffect, useRef} from "react";
import GLProgramState, {GLProgramStateOptions} from "./GLProgramState.ts";
import GLProgramDrawMode from "./GLProgramDrawMode.ts";
import GLState from "../gl/GLState.ts";

function useProgram_aux(gl: GLState, vert: string, frag: string, options: GLProgramStateOptions): GLProgramState {
  const programRef = useRef<GLProgramState>(new GLProgramState(gl, vert, frag, options));

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
export default function useProgram(gl: GLState, vert: string, frag: string, options?: Partial<GLProgramStateOptions>) {
  const filledOptions = {
    drawMode: GLProgramDrawMode.TRIANGLE_STRIP,
    vertexFirst: 0,
    vertexCount: 4,
    ...options,
  }

  const program = useProgram_aux(gl, vert, frag, filledOptions);

  useEffect(() => {
    program.vertexCount = filledOptions.vertexCount;
    program.queue.push(); // Trigger a re-render
  }, [filledOptions.vertexCount, program]);

  useEffect(() => {
    program.vertexFirst = filledOptions.vertexFirst;
    program.queue.push(); // Trigger a re-render
  }, [filledOptions.vertexFirst, program]);

  useEffect(() => {
    program.drawMode = filledOptions.drawMode;
  }, [filledOptions.drawMode, program]);

  useEffect(() => {
    program.setShaders(gl, vert, frag);
  }, [vert, frag, program, gl]);

  return program;
}