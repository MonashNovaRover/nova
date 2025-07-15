import GLState from "../../../../hooks/webgl/gl/GLState.ts";
import React, {HTMLAttributes, memo} from "react";
import useCanvasSize from "../../../../hooks/webgl/gl/useCanvasSize.ts";

export interface AutosizedGLCanvasPros extends HTMLAttributes<HTMLCanvasElement> {
  gl: GLState,
  sizeTarget?: Element | null,
  canvasClassName?: string,
  drawChildrenBelow?: boolean
}

/**
 * Creates a canvas you can use with `useGL`, which is pixel perfect, and acts like a `<div>` in regard to how it is
 * positioned and sized.
 *
 * You can also give it children, which are placed on top of the canvas.
 * @param props The component properties, which ***must*** include the result of `useGL` as `gl`
 * @constructor
 */
const RawAutosizedGLCanvas: React.FC<AutosizedGLCanvasPros> = (props) => {
  const {
    gl: gl,
    className:
      className,
    children: children,
    canvasClassName: canvasClassName,
    sizeTarget: sizeTarget,
    drawChildrenBelow: drawChildrenBelow,
    ...canvasProps
  } = props;
  useCanvasSize(gl, sizeTarget);

  const childrenContainer = children && (
    <div className="relative">
      {children}
    </div>
  );

  const canvasContainer = (
    <div className={"absolute top-0 left-0 right-0 bottom-0 overflow-hidden " + (drawChildrenBelow ? "z-10" : "")}>
      <canvas ref={gl.canvasRef} {...canvasProps}
              className={sizeTarget ? canvasClassName + " absolute left-0 top-0" : canvasClassName}
              />
    </div>
  );

  return (
    <div className={"relative overflow-hidden " + className + (drawChildrenBelow ? "flex flex-col-reverse" : "" )}>
      {canvasContainer}
      {childrenContainer}
    </div>
  )
}

/**
 * Creates a canvas you can use with `useGL`, which is pixel perfect, and acts like a `<div>` in regard to how it is
 * positioned and sized.
 *
 * You can also give it children, which are placed on top of the canvas.
 * @param props The component properties, which ***must*** include the result of `useGL` as `gl`
 * @constructor
 */
const AutosizedGLCanvas = memo(RawAutosizedGLCanvas);
export default AutosizedGLCanvas;

