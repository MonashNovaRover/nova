import React, {useEffect, useLayoutEffect, useRef} from "react";
import useEffectQueue from "../effect-queue/useEffectQueue.ts";
import EffectQueue from "../effect-queue/EffectQueue.ts";
import useRenderQueue from "../render-queue/useRenderQueue.ts";
import RenderQueue from "../render-queue/RenderQueue.ts";

export interface CanvasWithGL {
  canvasRef: React.RefObject<HTMLCanvasElement>,
  context?: WebGL2RenderingContext,
  queue: EffectQueue<[WebGL2RenderingContext]>,
  renderQueue: RenderQueue,

  /**
   * Re-renders all programs if there are any changes to inputs of the programs,
   * unless otherwise specified with force = true
   * @param force Set to true if you want to force everything to be drawn, even if there are no changes to the inputs.
   */
  render: (force?: boolean) => void,
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
      render: () => {},
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

    if (gl.context === undefined) {
      console.warn("Failed to set up a rendering context in useGL!");
      return;
    }

    gl.renderQueue.setup(gl.context);
    gl.render = (force?: boolean) => {
      if (!gl.context)
        return;

      if (!gl.queue.clear(gl.context) || force) {
        gl.renderQueue.render(gl.context);
      }
    }
  }, [gl, webContextAttributes]);

  // This performs rendering
  useEffect(() => {
    let frameID = 0;

    // Make a loop using requestAnimationFrame
    const callback = () => {
      gl.render();
      frameID = requestAnimationFrame(callback);
    }
    callback();

    return () => cancelAnimationFrame(frameID);
  }, [gl]);

  // The returned object will only exist if the WebGL2RenderingContext has already been created.
  return gl;
}

export default useGL;