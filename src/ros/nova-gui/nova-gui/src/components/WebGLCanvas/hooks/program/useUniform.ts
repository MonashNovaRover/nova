import {vec} from "../useUniforms.tsx";
import useEffectQueueEffect from "../effectQueue/useEffectQueueEffect.ts";
import {ProgramWithGL} from "./useProgram.ts";

/**
* Applies uniform vector or float values to a given program. Names of the uniforms should match those of the uniforms
* defined in the program.
* @param programWithGL The object containing the rendering context used
* @param name The name of the uniform in the program
* @param uniform The uniform float or vector value.
*/
const useUniform = (programWithGL: ProgramWithGL | undefined, name: string, uniform?: vec) => {
  useEffectQueueEffect(() => {
    if (!programWithGL || !uniform)
      return;

    const location = programWithGL.gl.getUniformLocation(programWithGL.program, name);

    if (location === null)
      return;

    if      (uniform.length === 1) programWithGL.gl.uniform1f(location, ...uniform);
    else if (uniform.length === 2) programWithGL.gl.uniform2f(location, ...uniform);
    else if (uniform.length === 3) programWithGL.gl.uniform3f(location, ...uniform);
    else if (uniform.length === 4) programWithGL.gl.uniform4f(location, ...uniform);

  }, [uniform, name], programWithGL?.queue);
}

export default useUniform;