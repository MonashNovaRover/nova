import {useCallback, useLayoutEffect, useRef} from "react";
import useAnimationFrame from "./useAnimationFrame.ts";
import GLState from "./GLState.ts";
import GLStateRenderInfo from "./GLStateRenderInfo.ts";

const useGL_aux = (): GLState => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const gl = useRef<GLState | undefined>(undefined);
  if (gl.current === undefined)
    gl.current = new GLState(canvasRef);

  return gl.current!;
}

/**
 * A custom hook allowing the WebGL2RenderingContext to be extracted from a canvas ref.
 * @param webContextAttributes any attributes to use when creating the canvas ref
 */
const useGL = (webContextAttributes?: WebGLContextAttributes )
  : GLState =>
{
  const gl = useGL_aux();

  useLayoutEffect(() => {
    if (!gl.canvasRef.current)
      throw new Error("The gl.canvasRef from useGL was not given to a canvas element!")

    // Just for debugging. We want to avoid regenerating the context, as it could hide bugs.
    if (gl.context === undefined)
      console.warn("A WebGL2RenderingContext was regenerated in useGL.");

    // Add event listeners to the canvas element

    gl.context = gl.canvasRef.current.getContext("webgl2", {
      ...webContextAttributes,
    }) ?? undefined;

    if (gl.context === undefined) {
      console.warn("Failed to set up a rendering context in useGL!");
      return;
    }
  }, [gl, webContextAttributes]);

  const renderCallback = useCallback((milliseconds: DOMHighResTimeStamp, deltaMilliseconds: number) => {
    const renderInfo: GLStateRenderInfo = {
      milliseconds: milliseconds,
      deltaMilliseconds: deltaMilliseconds
    };

    gl.render(false, renderInfo);
  }, [gl]);
  useAnimationFrame(renderCallback);

  // The returned object will only exist if the WebGL2RenderingContext has already been created.
  return gl;
}

export default useGL;