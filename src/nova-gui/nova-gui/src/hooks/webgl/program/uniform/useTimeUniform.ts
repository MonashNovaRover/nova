import GLProgramState from "../GLProgramState.ts";
import {applyUniform, vec} from "./useUniform.ts";
import useProgramRenderEffect from "../useProgramRenderEffect.ts";
import {DependencyList, useEffect} from "react";

export default function useTimeUniform(programState: GLProgramState, name: string = "time",
                                       factory?: (milliseconds: DOMHighResTimeStamp, deltaMilliseconds: number) => vec,
                                       deps: DependencyList = []) {
  useProgramRenderEffect(programState, (gl, program, info) => {
    gl.useProgram(program);

    const uniform = factory?.(info.milliseconds, info.deltaMilliseconds) ?? [info.milliseconds / 1000];

    // Trigger a re-render for the next frame.
    // TODO: make a better mechanism for this
    programState.queue.push();

    applyUniform(gl, program, name, uniform);
  }, [name, ...deps]);

  useEffect(() => {
    // Start the cycle of endless re-renders
    programState.queue.push();
  }, [programState.queue]);
}