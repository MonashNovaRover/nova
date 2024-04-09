import GLProgramState from "../GLProgramState.ts";
import useProgramEffect from "../useProgramEffect.ts";

export type vec1 = [number];
export type vec2 = [number, number];
export type vec3 = [number, number, number];
export type vec4 = [number, number, number, number];

export type vec = [number] | [number, number] | [number, number, number] | [number, number, number, number];

export type GLUniforms = {[key: string] : vec};

/**
 * Applies uniform vector or float values to a given program. Names of the uniforms should match those of the uniforms
 * defined in the program.
 * @param program The program to apply uniforms to
 * @param uniforms The uniform float and vector values.
 */
const useUniforms = (program: GLProgramState, uniforms?: GLUniforms) => {
  useProgramEffect(program, (gl, program) => {
    if (gl === undefined || uniforms === undefined || program === undefined)
      return;

    const entries = Object.entries(uniforms);
    entries.push(["resolution", [gl.drawingBufferWidth, gl.drawingBufferHeight]]);

    entries.forEach(([key, uniform]) => {
      const location = gl.getUniformLocation(program, key);

      if (location === null)
        return;

      if      (uniform.length === 1) gl.uniform1f(location, ...uniform);
      else if (uniform.length === 2) gl.uniform2f(location, ...uniform);
      else if (uniform.length === 3) gl.uniform3f(location, ...uniform);
      else if (uniform.length === 4) gl.uniform4f(location, ...uniform);
    });
  }, [uniforms]);
}

export default useUniforms;