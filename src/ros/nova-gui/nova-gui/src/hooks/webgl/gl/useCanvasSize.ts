import {useLayoutEffect} from "react";
import GLState from "./GLState.ts";

/**
 * Automatically calculates the size of a canvas used for webgl to match the size, in screen pixels, of some element
 * (the canvas itself by default).
 * @param gl The object containing the reference to the canvas, and the rendering context for the canvas.
 * @param sizeTarget The element to try to match the pixel size of. Uses canvasRef when not specified.
 */
export default function useCanvasSize(gl : GLState, sizeTarget?: Element | null): void {
  useLayoutEffect(() => {
    const canvas = gl.canvasRef.current;
    if (canvas === null)
      return;

    const absoluteSizeTarget = sizeTarget ?? canvas.parentElement ?? canvas;

    let frameID = -1;

    const boxObserver = new ResizeObserver((entries) => {
      const entry = entries.find((entry) => entry.target === absoluteSizeTarget);

      if (!entry)
        return;

      /* … render to canvas … */
      frameID = gl.queue.update(frameID, () => {
        // Resize the drawing buffer
        gl.context?.viewport(0,0,
          entry.devicePixelContentBoxSize[0].inlineSize,
          entry.devicePixelContentBoxSize[0].blockSize);

        if (!gl.canvasRef.current)
          return;

        // Update canvas element styles to make it align pixel perfect with the previously set drawing buffer size
        gl.canvasRef.current?.style.setProperty("inline-size",
          `${entry.devicePixelContentBoxSize[0].inlineSize / window.devicePixelRatio}px`);
        gl.canvasRef.current?.style.setProperty("block-size",
          `${entry.devicePixelContentBoxSize[0].blockSize  / window.devicePixelRatio}px`);
        gl.canvasRef.current.width = entry.devicePixelContentBoxSize[0].inlineSize;
        gl.canvasRef.current.height = entry.devicePixelContentBoxSize[0].blockSize;
      });
    });
    boxObserver.observe(absoluteSizeTarget, { box: "device-pixel-content-box" });

    return () => {
      boxObserver.disconnect();

      gl.queue.cancel(frameID);
    };
  }, [gl, sizeTarget]);
}