import React, {useLayoutEffect, useMemo, useRef} from "react";
import useEffectQueue, {EffectQueue} from "./effectQueue/useEffectQueue.ts";

export interface CanvasWithGL {
  canvasRef: React.RefObject<HTMLCanvasElement>,
  context?: WebGL2RenderingContext,
  queue: EffectQueue,
}

/**
 * A custom hook allowing the WebGL2RenderingContext to be extracted from a canvas ref.
 * @param webglAttributes any attributes to use when creating the canvas ref
 */
const useGL = (webglAttributes?: WebGLContextAttributes )
  : CanvasWithGL =>
{

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const queue = useEffectQueue<[WebGL2RenderingContext]>();

  const context = useRef<CanvasWithGL>();
  if (context.current === undefined) {
    context.current = ({
      canvasRef: canvasRef,
      context: undefined,
      queue: queue,
    } as CanvasWithGL);
  }





  useLayoutEffect(() => {
    if (gl.current !== undefined && gl.current !== null)
      console.warn("A WebGL2RenderingContext was regenerated in useGL.");

    gl.current = canvasRef.current?.getContext?.("webgl2", {
      ...webglAttributes,
    }) ?? undefined;
  }, [webglAttributes]);

  // The returned object will only exist if the WebGL2RenderingContext has already been created.
  return useMemo(() => {
    return {
      canvasRef: canvasRef,
      gl: gl.current,
      queue: queue,
    } as CanvasWithGL;
  }, [queue]) ?? false;
}

export default useGL;