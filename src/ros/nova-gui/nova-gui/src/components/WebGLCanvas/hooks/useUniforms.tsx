import {useEffect} from "react";

export type vec1 = [number];
export type vec2 = [number, number];
export type vec3 = [number, number, number];
export type vec4 = [number, number, number, number];

export type vec = [number] | [number, number] | [number, number, number] | [number, number, number, number];

export type GLUniforms = {[key: string] : vec};

const useUniforms = (gl?: WebGLRenderingContext, program?: WebGLProgram, uniforms?: GLUniforms) => {
  useEffect(() => {
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

    // console.log("Uniforms updated");
  }, [gl, program, uniforms]);
}

export default useUniforms;