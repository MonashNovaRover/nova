import React, {useLayoutEffect, useRef} from "react";

export interface CanvasWithGL {
  canvasRef: React.RefObject<HTMLCanvasElement>,
  gl?: WebGL2RenderingContext
}

/**
 * A custom hook allowing the WebGL2RenderingContext to be extracted from a canvas ref.
 * @param webglAttributes any attributes to use when creating the canvas ref
 */
const useGL = (webglAttributes?: WebGLContextAttributes )
  : CanvasWithGL =>
{
  const gl = useRef<WebGL2RenderingContext | undefined>(undefined);
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useLayoutEffect(() => {
    gl.current = (canvasRef.current?.getContext?.("webgl2", {
      ...webglAttributes,
    }) ?? undefined);
  }, [canvasRef, webglAttributes]);

  return {
    canvasRef: canvasRef,
    gl: gl.current,
  };
}

export default useGL;