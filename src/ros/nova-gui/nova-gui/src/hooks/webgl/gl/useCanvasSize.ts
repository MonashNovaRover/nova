import {useLayoutEffect, useState} from "react";
import {GLState} from "./useGL.ts";

/**
 * Automatically calculates the size of a canvas used for webgl to match the size, in screen pixels, of some element
 * (the canvas itself by default).
 * @param gl The object contain the reference to the canvas, and the rendering context for the canvas.
 * @param sizeTarget The element to try match the pixel size of. Uses canvasRef when not specified.
 */
export default function useCanvasSize(gl : GLState, sizeTarget?: Element): [number, number] {
  const [width, setWidth] = useState(4);
  const [height, setHeight] = useState(3);

  useLayoutEffect(() => {
    const canvas = gl.canvasRef.current;
    if (canvas === null)
      return;

    const pixelRatio = window.devicePixelRatio
    const absoluteSizeTarget = sizeTarget ?? canvas.parentElement ?? canvas;

    const resize = () => {
      const rect = absoluteSizeTarget.getBoundingClientRect();
      const newWidth = Math.ceil(pixelRatio * rect.width);
      const newHeight = Math.ceil(pixelRatio * rect.height);

      setWidth(newWidth);
      setHeight(newHeight);

      gl.context?.viewport(0,0, newWidth, newHeight);
    };

    const resizeObserver = new ResizeObserver(resize);
    resizeObserver.observe(absoluteSizeTarget);
    const observer = new MutationObserver(resize);
    observer.observe(absoluteSizeTarget, {attributes: true, attributeFilter: ["style"]});

    return () => {
      resizeObserver.disconnect();
      observer.disconnect();
    };
  }, [gl, sizeTarget]);

  return [
    width,
    height,
  ];
}