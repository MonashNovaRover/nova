import React, {useEffect} from "react";
import GLProgramState from "./GLProgramState.ts";

export default function useProgramEffect(program: GLProgramState,
                                         effect: (context: WebGL2RenderingContext, program: WebGLProgram) => void,
                                         deps: React.DependencyList)
{
  useEffect(() => {
    const frameID = program.queue.push(effect);
    return () => program.queue.cancel(frameID);
  }, [...deps]);
}