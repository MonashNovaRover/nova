import GLProgramState from "../GLProgramState.ts";
import useProgramEffect from "../useProgramEffect.ts";
import React from "react";

export type vec1 = [number];
export type vec2 = [number, number];
export type vec3 = [number, number, number];
export type vec4 = [number, number, number, number];

export type vec = [number] | [number, number] | [number, number, number] | [number, number, number, number];

/**
 * Applies uniform vector or float values to a given program. Names of the uniforms should match those of the uniforms
 * defined in the program.
 * @param program The object containing the rendering context used
 * @param name The name of the uniform in the program
 * @param factory The uniform float or vector value.
 * @param deps The dependency array, that should cause the uniform to be reset when changed.
 */
const useUniform = (program: GLProgramState, name: string, factory: (() => vec) | vec,
                    deps: React.DependencyList = []) => {
  useProgramEffect(program, (gl, program) => {
    const uniform = Array.isArray(factory) ? factory : factory();

    const location = gl.getUniformLocation(program, name);

    if (location === null)
      return;

    if      (uniform.length === 1) gl.uniform1f(location, ...uniform);
    else if (uniform.length === 2) gl.uniform2f(location, ...uniform);
    else if (uniform.length === 3) gl.uniform3f(location, ...uniform);
    else if (uniform.length === 4) gl.uniform4f(location, ...uniform);
  }, [name, ...deps]);
}

export default useUniform;