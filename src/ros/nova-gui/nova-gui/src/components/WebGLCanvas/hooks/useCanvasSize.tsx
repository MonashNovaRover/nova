import {useEffect, useLayoutEffect, useState} from "react";
import {CanvasWithGL} from "./useGL.tsx";
import {size} from "lodash";

export default function useCanvasSize({gl, canvasRef} : CanvasWithGL, sizeTarget?: Element): [number, number] {
  const [width, setWidth] = useState(4);
  const [height, setHeight] = useState(3);



  useLayoutEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas)
      return;

    const pixelRatio = window.devicePixelRatio
    const absoluteSizeTarget = sizeTarget ?? canvas;

    const resize = () => {
      const rect = canvas.parentElement?.getBoundingClientRect() ?? canvas.getBoundingClientRect();
      setWidth(pixelRatio * rect.width);
      setHeight(pixelRatio * rect.height);
    };

    const resizeObserver = new ResizeObserver(resize);
    resizeObserver.observe(absoluteSizeTarget);
    const observer = new MutationObserver(resize);
    observer.observe(absoluteSizeTarget, {attributes: true, attributeFilter: ["style"]});
  }, [sizeTarget]);

  useEffect(() => {
    if (!gl)
      return;

    console.log("Resized canvas")

    gl.viewport(0, 0, width, height);
  }, [gl, width, height]);

  return [
    width,
    height,
  ];
}