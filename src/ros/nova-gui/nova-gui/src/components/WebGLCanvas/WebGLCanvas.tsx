import React, {memo, useEffect, useRef} from "react";
import useGL from "./useGL.tsx";
import {useProgram} from "./useProgram.tsx";
import useVertexAttributes, {IVertexAttribute} from "./useVertexAttributes.tsx";
import useUniforms, {vec, vec4} from "./useUniforms.tsx";







export interface IWebGLCanvasProps {
  vert: string,
  frag: string,
  // uniformFloats?: {[Name: string]: I},

  // canvas props
  className?: string,
  width?: number,
  height?: number,
  vertexCount?: number,

  // When true, changes the width and height of the canvas automatically to be the width and height on the screen.
  autoSize?: boolean

  vertexAttributes?: {[key: string] : IVertexAttribute}

  clearColor?: vec4

  uniforms?: {[key: string] : vec}
}



const UnmemoedWebGLCanvas: React.FC<IWebGLCanvasProps> = ({
  vert, frag, vertexCount, vertexAttributes, clearColor, uniforms, ...canvasProps
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const gl = useGL(canvasRef);
  const program = useProgram(gl, vert, frag);

  // Effect to update the clear color
  useEffect(() => {
    gl?.clearColor(...(clearColor ?? [0, 0, 0, 0]));
  }, [gl, clearColor]);

  // Apply vertex attributes. (might include a position buffer of some kind?)
  useVertexAttributes(gl, program, vertexAttributes);

  // Apply uniforms
  useUniforms(gl, program, uniforms);

  // Effect to redraw the canvas
  useEffect(() => {
    if (!gl || !program)
      return;

    // Redraw
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, vertexCount ?? 4);
  }, [gl, program, uniforms, vertexAttributes, vertexCount]);

  return <canvas {...canvasProps}  ref={canvasRef}></canvas>;
}

const WebGLCanvas = memo(UnmemoedWebGLCanvas);
export default WebGLCanvas;