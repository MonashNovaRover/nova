import GLProgramState from "../GLProgramState.ts";
import {applyAttribute, defaultUseAttributeOptions, UseAttributeOptions, vecArray} from "./useAttribute.ts";
import {DependencyList, useRef} from "react";
import useProgramRenderEffect from "../useProgramRenderEffect.ts";

export default function useTimeAttribute(programState: GLProgramState, name: string,
  factory: (milliseconds: DOMHighResTimeStamp, deltaMilliseconds: number) => vecArray, deps: DependencyList = [],
  options?: Partial<UseAttributeOptions>) {

  const buffer = useRef<WebGLBuffer>(undefined);

  const filledOptions: UseAttributeOptions = {
    ...defaultUseAttributeOptions,
    ...options
  };

  useProgramRenderEffect(programState, (context, program, info) => {
    // Create the buffer if necessary
    if (buffer.current === undefined) {
      buffer.current = context.createBuffer() ?? undefined;

      // Ensure the creation of the buffer was successful before trying to use it.
      if (buffer.current === undefined)
        return;
    }

    const attribute = factory(info.milliseconds, info.deltaMilliseconds);
    applyAttribute(context, program, name, attribute, buffer.current, filledOptions, false);
  }, [name, ...deps]);
}