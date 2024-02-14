import React, {useEffect, useState} from "react";


export default function useCanvasSize(gl: WebGL2RenderingContext | undefined, canvasRef: React.RefObject<HTMLCanvasElement>, pixelRatio: number ): { width: number, height: number } {
  const [width, setWidth] = useState(0);
  const [height, setHeight] = useState(0);

  React.useLayoutEffect(() => {
    const canvas = canvasRef.current;
    if (canvas) {
      const resize = () => {
        const rect = canvas.getBoundingClientRect();
        setWidth(pixelRatio * rect.width);
        setHeight(pixelRatio * rect.height);
      };
      const resizeObserver = new ResizeObserver(resize);
      resizeObserver.observe(canvas);
      const observer = new MutationObserver(resize);
      observer.observe(canvas, {attributes: true, attributeFilter: ["style"]});
    }
  });

  useEffect(() => {
    if (gl) {
      gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    }
  }, [gl, width, height]);

  return {
    width,
    height,
  };
}