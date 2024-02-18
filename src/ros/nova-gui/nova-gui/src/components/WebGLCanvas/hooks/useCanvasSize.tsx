import React, {useEffect, useState} from "react";

export default function useCanvasSize(gl: WebGL2RenderingContext | undefined, canvasRef: React.RefObject<HTMLCanvasElement>): { width: number, height: number } {
  const [width, setWidth] = useState(4);
  const [height, setHeight] = useState(3);



  React.useLayoutEffect(() => {
    const pixelRatio = window.devicePixelRatio

    const canvas = canvasRef.current;
    if (canvas) {
      const resize = () => {
        const rect = canvas.parentElement?.getBoundingClientRect() ?? canvas.getBoundingClientRect();
        setWidth(pixelRatio * rect.width);
        setHeight(pixelRatio * rect.height);
      };
      const resizeObserver = new ResizeObserver(resize);
      resizeObserver.observe(canvas);
      const observer = new MutationObserver(resize);
      observer.observe(canvas, {attributes: true, attributeFilter: ["style"]});
    }
  }, []);

  useEffect(() => {
    if (!gl)
      return;


    gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);

  }, [gl, width, height]);

  return {
    width,
    height,
  };
}