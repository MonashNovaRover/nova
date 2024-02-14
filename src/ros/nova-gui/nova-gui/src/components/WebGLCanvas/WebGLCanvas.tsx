import React, {memo, useEffect, useRef} from "react";
import useGL from "./useGL.tsx";
import {useProgram} from "./useProgram.tsx";
import useVertexAttributes, {IVertexAttribute} from "./useVertexAttributes.tsx";



export type vec2 = [number, number];
export type vec4 = [number, number, number, number];



export interface IWebGLCanvasProps {
  vert: string,
  frag: string,
  // uniformFloats?: {[Name: string]: I},

  // canvas props
  className?: string,
  width?: number,
  height?: number,
  vertexCount?: number,

  vertexAttributes?: {[key: string] : IVertexAttribute}

  clearColor?: vec4
}



const UnmemoedWebGLCanvas: React.FC<IWebGLCanvasProps> = ({
  vert, frag, vertexCount, vertexAttributes, clearColor, ...canvasProps
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const gl = useGL(canvasRef);
  const program = useProgram(gl, vert, frag);

  useEffect(() => {
    gl?.clearColor(...(clearColor ?? [0, 0, 0, 0]));
  }, [gl, clearColor]);

  // Apply vertex attributes. (might include a position buffer of some kind?)
  useVertexAttributes(gl, program, vertexAttributes, vertexCount ?? 4);

  useEffect(() => {
    if (!gl)
      return;

    console.log("new GL created")

  }, [gl]);



  return <canvas {...canvasProps} ref={canvasRef}></canvas>;
}

const WebGLCanvas = memo(UnmemoedWebGLCanvas);
export default WebGLCanvas;