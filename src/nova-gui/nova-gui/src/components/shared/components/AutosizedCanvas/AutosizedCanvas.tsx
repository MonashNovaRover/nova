import React, {HTMLAttributes, useLayoutEffect} from "react";

function getObservedSize(entry: ResizeObserverEntry) {
  const deviceBox = entry.devicePixelContentBoxSize;
  if (deviceBox && deviceBox.length > 0) {
    const {inlineSize, blockSize} = deviceBox[0];
    return {
      deviceWidth: inlineSize,
      deviceHeight: blockSize,
      cssWidth: inlineSize / (window.devicePixelRatio || 1),
      cssHeight: blockSize / (window.devicePixelRatio || 1),
    };
  }

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

export interface AutosizedCanvasProps
  extends HTMLAttributes<HTMLCanvasElement> {
  canvasRef: React.RefObject<HTMLCanvasElement>;
  sizeTarget?: Element | null;
}

const AutosizedCanvas: React.FC<AutosizedCanvasProps> = (
  props
) => {
  const {
    canvasRef,
    sizeTarget,
    ...canvasProps
  } = props;

  useLayoutEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const absoluteSizeTarget =
      sizeTarget ?? canvas.parentElement ?? canvas;
    if (!absoluteSizeTarget) return;

    const boxObserver = new ResizeObserver((entries) => {
      const entry = entries.find(
        (e) => e.target === absoluteSizeTarget
      );
      if (!entry) return;

      const {
        deviceWidth,
        deviceHeight,
        cssWidth,
        cssHeight,
      } = getObservedSize(entry);

      canvas.style.setProperty(
        "inline-size",
        `${cssWidth}px`
      );
      canvas.style.setProperty(
        "block-size",
        `${cssHeight}px`
      );
      canvas.width = deviceWidth;
      canvas.height = deviceHeight;
    });

    try {
      boxObserver.observe(absoluteSizeTarget, {
        box: "device-pixel-content-box",
      });
    } catch {
      boxObserver.observe(absoluteSizeTarget);
    }

    return () => {
      boxObserver.disconnect();
    };
  }, [canvasRef, sizeTarget]);

  return <canvas ref={canvasRef} {...canvasProps} />;
};

export default AutosizedCanvas;
