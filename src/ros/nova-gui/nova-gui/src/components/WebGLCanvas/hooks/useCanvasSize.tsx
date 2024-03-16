import {useEffect, useLayoutEffect, useState} from "react";
import {CanvasWithGL} from "./useGL.tsx";

/**
 * Automatically calculates the size of a canvas used for webgl to match the size, in screen pixels, of some element
 * (the canvas itself by default).
 * @param gl The webgl rendering context.
 * @param canvasRef The reference to the canvas
 * @param sizeTarget The element to try match the pixel size of. Uses canvasRef when not specified.
 */
export default function useCanvasSize({gl, canvasRef} : CanvasWithGL, sizeTarget?: Element): [number, number] {
  const [width, setWidth] = useState(4);
  const [height, setHeight] = useState(3);

  useLayoutEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas)
      return;

    const pixelRatio = window.devicePixelRatio
    const absoluteSizeTarget = sizeTarget ?? canvas.parentElement ?? canvas;

    const resize = () => {
      const rect = absoluteSizeTarget.getBoundingClientRect();
      setWidth(pixelRatio * rect.width);
      setHeight(pixelRatio * rect.height);
    };

    const resizeObserver = new ResizeObserver(resize);
    resizeObserver.observe(absoluteSizeTarget);
    const observer = new MutationObserver(resize);
    observer.observe(absoluteSizeTarget, {attributes: true, attributeFilter: ["style"]});

    return () => {
      resizeObserver.disconnect();
      observer.disconnect();
    };
  }, [canvasRef, sizeTarget]);

  useEffect(() => {
    if (!gl)
      return;

    gl.viewport(0, 0, width, height);
  }, [gl, width, height]);

  return [
    width,
    height,
  ];
}