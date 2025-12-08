import GLProgramState from "../GLProgramState.ts";
import useProgramEffect from "../useProgramEffect.ts";
import {DependencyList} from "react";

export type vec1 = [number];
export type vec2 = [number, number];
export type vec3 = [number, number, number];
export type vec4 = [number, number, number, number];

export type vec = [number] | [number, number] | [number, number, number] | [number, number, number, number];

/**
 * Applies uniform vector or float values to a given program. The given name of the uniform should match those of the
 * uniforms defined in the program.
 * @param program The result of `useProgram`
 * @param name The name of the uniform in the program
 * @param factoryOrConstant The uniform float or vector value, or a function to generate it when a dependency changes.
 * @param deps The dependency array, that should cause the uniform to be reset when changed.
 */
export default function useUniform(program: GLProgramState, name: string,
                                   factoryOrConstant: (() => vec) | vec, deps: DependencyList = []) {
  useProgramEffect(program, (gl, program) => {
    gl.useProgram(program)

    const uniform = Array.isArray(factoryOrConstant) ? factoryOrConstant : factoryOrConstant();
    applyUniform(gl, program, name, uniform);
  }, [name, ...deps]);
}

export function applyUniform(context: WebGL2RenderingContext, program: WebGLProgram, name: string, uniform: vec) {
  const location = context.getUniformLocation(program, name);

  if (location === null) {
    console.warn(`Uniform "${name}" does not exist on the current program`);
    return;
  }

  if      (uniform.length === 1) context.uniform1f(location, ...uniform);
  else if (uniform.length === 2) context.uniform2f(location, ...uniform);
  else if (uniform.length === 3) context.uniform3f(location, ...uniform);
  else if (uniform.length === 4) context.uniform4f(location, ...uniform);
}
