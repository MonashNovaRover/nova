import React, {useLayoutEffect, useRef} from "react";

/**
 * A custom hook allowing the WebGL2RenderingContext to be extracted from a canvas ref.
 * @param canvasRef the canvas ref, a result of calling something like `useRef<HTMLCanvasElement>(null);`
 * @param webglAttributes any attributes to use when creating the canvas ref
 */
const useGL = ( canvasRef: React.RefObject<HTMLCanvasElement>, webglAttributes?: WebGLContextAttributes )
  : WebGL2RenderingContext | undefined =>
{
  const gl = useRef<WebGL2RenderingContext | undefined>(undefined);

  useLayoutEffect(() => {
    gl.current = (canvasRef.current?.getContext?.("webgl2", {
      ...webglAttributes,
    }) ?? undefined);
  }, [canvasRef, webglAttributes]);

  return gl.current;
}

export default useGL;