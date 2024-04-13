import GLProgramState from "../GLProgramState.ts";
import useProgramEffect from "../useProgramEffect.ts";
import React, {DependencyList} from "react";

export type vec1 = [number];
export type vec2 = [number, number];
export type vec3 = [number, number, number];
export type vec4 = [number, number, number, number];

export type vec = [number] | [number, number] | [number, number, number] | [number, number, number, number];

export default function useUniform(program: GLProgramState, name: string, factory: (() => vec),
                                   deps: DependencyList): void;
export default function useUniform(program: GLProgramState, name: string, constantUniform: vec): void;
export default function useUniform(program: GLProgramState, name: string, defaultFactory: () => vec): void;

/**
 * Applies uniform vector or float values to a given program. The given name of the uniform should match those of the
 * uniforms defined in the program.
 * @param program The result of `useProgram`
 * @param name The name of the uniform in the program
 * @param factoryOrConstant The uniform float or vector value, or a function to generate it when a dependency changes.
 * @param deps The dependency array, that should cause the uniform to be reset when changed.
 */
export default function useUniform(program: GLProgramState, name: string, factoryOrConstant: (() => vec) | vec,
                    deps: React.DependencyList = []) {
  useProgramEffect(program, (gl, program) => {
    gl.useProgram(program)

    const uniform = Array.isArray(factoryOrConstant) ? factoryOrConstant : factoryOrConstant();

    const location = gl.getUniformLocation(program, name);

    if (location === null)
      return;

    if      (uniform.length === 1) gl.uniform1f(location, ...uniform);
    else if (uniform.length === 2) gl.uniform2f(location, ...uniform);
    else if (uniform.length === 3) gl.uniform3f(location, ...uniform);
    else if (uniform.length === 4) gl.uniform4f(location, ...uniform);
  }, [name, ...deps]);


}