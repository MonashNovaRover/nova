import GLProgramState from "../GLProgramState.ts";
import useAttribute from "./useAttribute.ts";
import {vec2} from "../uniform/useUniform.ts";

export const screenQuadAttribute: vec2[] = [[1, 1], [-1, 1], [1, -1], [-1, -1]]

/**
 * Shorthand for the frequently used useAttribute quad from -1 to 0 on the x and y-axis, used with the default
 * GLProgramDrawMode.TRIANGLE_STRIP draw mode; [[1, 1], [-1, 1], [1, -1], [-1, -1]]
 * @param program The program to apply the attribute to
 * @param name The name of the vertex attribute in the program. "aPosition" by default
 */
export default function useScreenQuadAttribute(program: GLProgramState, name: string = "aPosition") {
  useAttribute(program, name, screenQuadAttribute, []);
}