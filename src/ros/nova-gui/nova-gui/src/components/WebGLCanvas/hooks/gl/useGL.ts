import React, {useLayoutEffect, useRef} from "react";
import useEffectQueue from "../effectQueue/useEffectQueue.ts";
import EffectQueue from "../effectQueue/EffectQueue.ts";
import useRenderQueue from "./useRenderQueue.ts";
import RenderQueue from "./RenderQueue.ts";

export interface CanvasWithGL {
  canvasRef: React.RefObject<HTMLCanvasElement>,
  context?: WebGL2RenderingContext,
  queue: EffectQueue<[WebGL2RenderingContext]>,
  renderQueue: RenderQueue,
}

const useGL_aux = (): CanvasWithGL => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const renderQueue = useRenderQueue();
  const queue = useEffectQueue<[WebGL2RenderingContext]>();

  const gl = useRef<CanvasWithGL>();
  if (gl.current === undefined) {
    gl.current = ({
      canvasRef: canvasRef,
      context: undefined,
      queue: queue,
      renderQueue: renderQueue,
    } as CanvasWithGL);
  }

  return gl.current;
}

/**
 * A custom hook allowing the WebGL2RenderingContext to be extracted from a canvas ref.
 * @param webContextAttributes any attributes to use when creating the canvas ref
 */
const useGL = (webContextAttributes?: WebGLContextAttributes )
  : CanvasWithGL =>
{
  const gl = useGL_aux();

  useLayoutEffect(() => {
    if (!gl.canvasRef.current)
      throw new Error("The gl.canvasRef from useGL was not given to a canvas element!")

    // Just for debugging. We want to avoid regenerating the context, as it could hide bugs.
    if (gl.context === undefined)
      console.warn("A WebGL2RenderingContext was regenerated in useGL.");

    gl.context = gl.canvasRef.current.getContext("webgl2", {
      ...webContextAttributes,
    }) ?? undefined;

    if (gl.context !== undefined)
      gl.renderQueue.setup(gl.context);
    else
      console.warn("Failed to set up a rendering context in useGL!");
  }, [gl, webContextAttributes]);

  // The returned object will only exist if the WebGL2RenderingContext has already been created.
  return gl;
}

export default useGL;