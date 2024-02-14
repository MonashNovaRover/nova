import React, {useState} from "react";

/**
 * A custom hook allowing the WebGL2RenderingContext to be extracted from a canvas ref.
 * @param canvasRef the canvas ref, a result of calling something like `useRef<HTMLCanvasElement>(null);`
 * @param webglAttributes any attributes to use when creating the canvas ref
 */
const useGL = ( canvasRef: React.RefObject<HTMLCanvasElement>, webglAttributes?: WebGLContextAttributes )
  : WebGL2RenderingContext | undefined =>
{
  const [gl, setGL] = useState<WebGL2RenderingContext | undefined>();

  React.useLayoutEffect(() => {
    const canvas = canvasRef.current;
    setGL(canvas?.getContext?.("webgl2", {
      ...webglAttributes,
    }) ?? undefined);

    console.log("A new WebGL 2 rendering context was created.");
  }, []);

  return gl;
}

export default useGL;