import React, {memo, useEffect, useRef} from "react";
import useGL from "./useGL.tsx";
import {useProgram} from "./useProgram.tsx";
import useVertexAttributes, {IVertexAttribute} from "./useVertexAttributes.tsx";
import useUniforms, {vec, vec4} from "./useUniforms.tsx";
import useSamplers from "./useSamplers.tsx";

export interface IWebGLCanvasProps {
  // canvas props
  className?: string,
  width?: number,
  height?: number,

  // The source code for a vertex shader to use. Shader programs are auto-compiled by the component on change.
  vert: string,
  // The source code for a fragment shader to use. Shader programs are auto-compiled by the component on change.
  frag: string,

  // Defines the background color to be rendered by the canvas
  clearColor?: vec4

  // When true, changes the width and height of the canvas automatically to be the width and height on the screen.
  // TODO: implement
  autoSize?: boolean

  // Defines values for vertex attributes, where the key should match the name of the attribute in the vertex shader.
  // This might look like: `in (vec[234]|float) <name>` in the vertex shader. e.g. `in vec4 position`.
  vertexAttributes?: {[key: string] : IVertexAttribute}
  // The number of vertices to render when calling `gl.drawArrays`
  vertexCount?: number,

  // Defines float for float vector values for uniforms, where the key should match the name of the uniform in the
  // vertex or fragment shader. This might look like: `uniform (vec[234]|float) <name>` in the shader.
  uniforms?: {[key: string] : vec}

  // Defines samplers for the fragment shader (textures). These are passed as a `HTMLImageElement` or
  // `HTMLVideoElement`, which is automatically converted into a texture.
  samplers?: {[key: string] : HTMLVideoElement | HTMLImageElement}
}


const UnmemoedWebGLCanvas: React.FC<IWebGLCanvasProps> = ({
  vert, frag, vertexCount, vertexAttributes, clearColor, uniforms, samplers, ...canvasProps
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

  // TODO: fix samplers
  // useSamplers(gl, program, samplers)

  // Effect to redraw the canvas
  useEffect(() => {
    if (!gl || !program)
      return;

    // Redraw
    gl.clear(gl.COLOR_BUFFER_BIT);

    // TODO: allow the mode to be specified, rather than being hard coded as `gl.TRIANGLE_STRIP`
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, vertexCount ?? 4);
  }, [gl, program, uniforms, vertexAttributes, vertexCount, samplers]);

  return <canvas {...canvasProps}  ref={canvasRef}></canvas>;
}

const WebGLCanvas = memo(UnmemoedWebGLCanvas);
export default WebGLCanvas;