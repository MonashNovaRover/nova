import {useLayoutEffect, useState} from "react";
import GLState from "./GLState.ts";

/**
 * Automatically calculates the size of a canvas used for webgl to match the size, in screen pixels, of some element
 * (the canvas itself by default).
 * @param gl The object contain the reference to the canvas, and the rendering context for the canvas.
 * @param sizeTarget The element to try match the pixel size of. Uses canvasRef when not specified.
 */
export default function useCanvasSize(gl : GLState, sizeTarget?: Element | null): [number, number] {
  const [width, setWidth] = useState(4);
  const [height, setHeight] = useState(3);

  useLayoutEffect(() => {
    const canvas = gl.canvasRef.current;
    if (canvas === null)
      return;

    const absoluteSizeTarget = sizeTarget ?? canvas.parentElement ?? canvas;

    const boxObserver = new ResizeObserver((entries) => {
      const entry = entries.find((entry) => entry.target === absoluteSizeTarget);

      if (!entry)
        return;

      setWidth(entry.devicePixelContentBoxSize[0].inlineSize);
      setHeight(entry.devicePixelContentBoxSize[0].blockSize);

      gl.context?.viewport(0,0,
        entry.devicePixelContentBoxSize[0].inlineSize,
        entry.devicePixelContentBoxSize[0].blockSize);

      const newWidth = entry.devicePixelContentBoxSize[0].inlineSize / window.devicePixelRatio;
      const newHeight = entry.devicePixelContentBoxSize[0].blockSize / window.devicePixelRatio;
      gl.canvasRef.current?.style.setProperty("block-size", newHeight.toString() + "px");
      gl.canvasRef.current?.style.setProperty("inline-size", newWidth.toString() + "px");

      /* … render to canvas … */
      gl.queue.push();
    });
    boxObserver.observe(absoluteSizeTarget, { box: "device-pixel-content-box" });

    return () => {
      boxObserver.disconnect();
    };
  }, [gl, sizeTarget]);

  return [
    width,
    height,
  ];
}