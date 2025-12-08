import {useLayoutEffect} from "react";
import GLState from "./GLState.ts";

function getObservedSize(entry: ResizeObserverEntry) {

  // Prefer devicePixelContentBoxSize if available (Chromium).
  const deviceBox = entry.devicePixelContentBoxSize;
  if (deviceBox && deviceBox.length > 0) {
    const { inlineSize, blockSize } = deviceBox[0];
    return {
      deviceWidth: inlineSize,
      deviceHeight: blockSize,
      cssWidth: inlineSize / (window.devicePixelRatio || 1),
      cssHeight: blockSize / (window.devicePixelRatio || 1),
    };
  }

  // Next, try contentBoxSize (Firefox / some others).
  const contentBox = entry.contentBoxSize;
  if (contentBox) {
    const box = Array.isArray(contentBox) ? contentBox[0] : contentBox;
    const cssWidth = box.inlineSize;
    const cssHeight = box.blockSize;
    const dpr = window.devicePixelRatio || 1;
    return {
      deviceWidth: Math.round(cssWidth * dpr),
      deviceHeight: Math.round(cssHeight * dpr),
      cssWidth,
      cssHeight,
    };
  }

  // Fallback: contentRect (available everywhere).
  const cssWidth = entry.contentRect.width;
  const cssHeight = entry.contentRect.height;
  const dpr = window.devicePixelRatio || 1;
  return {
    deviceWidth: Math.round(cssWidth * dpr),
    deviceHeight: Math.round(cssHeight * dpr),
    cssWidth,
    cssHeight,
  };
}

/**
 * Automatically calculates the size of a canvas used for webgl to match the size, in screen pixels, of some element
 * (the canvas itself by default).
 * @param gl The object containing the reference to the canvas, and the rendering context for the canvas.
 * @param sizeTarget The element to try to match the pixel size of. Uses canvasRef when not specified.
 */
export default function useCanvasSize(gl: GLState, sizeTarget?: Element | null): void {
  useLayoutEffect(() => {
    const canvas = gl.canvasRef.current;
    if (!canvas) return;

    const absoluteSizeTarget = sizeTarget ?? canvas.parentElement ?? canvas;
    if (!absoluteSizeTarget) return;

    let frameID = -1;

    const boxObserver = new ResizeObserver((entries) => {
      const entry = entries.find((e) => e.target === absoluteSizeTarget);
      if (!entry) return;

      const { deviceWidth, deviceHeight, cssWidth, cssHeight } = getObservedSize(entry);

      frameID = gl.queue.update(frameID, () => {
        if (!gl.context) return;

        // Resize the drawing buffer
        gl.context.viewport(0, 0, deviceWidth, deviceHeight);

        const canvasEl = gl.canvasRef.current;
        if (!canvasEl) return;

        // Pixel-perfect CSS size vs drawing buffer size
        canvasEl.style.setProperty("inline-size", `${cssWidth}px`);
        canvasEl.style.setProperty("block-size", `${cssHeight}px`);
        canvasEl.width = deviceWidth;
        canvasEl.height = deviceHeight;
      });
    });

    // Safely request device-pixel box where supported; fall back otherwise.
    try {
      // This will be ignored/throw on browsers that don't support it.
      boxObserver.observe(absoluteSizeTarget, { box: "device-pixel-content-box" });
    } catch {
      boxObserver.observe(absoluteSizeTarget);
    }

    return () => {
      boxObserver.disconnect();
      gl.queue.cancel(frameID);
    };
  }, [gl, sizeTarget]);
}
