import React, {memo, MouseEventHandler, useEffect, WheelEventHandler} from "react";
import {CanvasWithGL} from "./hooks/gl/useGL.tsx";
import {useProgram} from "./hooks/program/useProgram.tsx";
import useAttributes, {GLAttributes} from "./hooks/useAttributes.tsx";
import useUniforms, {GLUniforms, vec2, vec4} from "./hooks/useUniforms.tsx";
import useSamplers, {GLSamplers} from "./hooks/useSamplers.tsx";

/**
 * Properties for the WebGL canvas. Use these to configure shader programs
 */
export interface IWebGLCanvasProps {
  gl: CanvasWithGL


  // HTMLCanvasElement props
  className?: string,

  width?: number,
  height?: number,
  resolution?: vec2,

  onMouseOver?: MouseEventHandler<HTMLCanvasElement>,
  onMouseMove?: MouseEventHandler<HTMLCanvasElement>,
  onMouseEnter?: MouseEventHandler<HTMLCanvasElement>,
  onMouseLeave?: MouseEventHandler<HTMLCanvasElement>,
  onMouseDown?: MouseEventHandler<HTMLCanvasElement>,
  onClick?: () => void,
  onWheel?: WheelEventHandler<HTMLCanvasElement>,

  // When true, changes the width and height of the canvas automatically to be the width and height on the screen.
  // TODO: implement

  // --- GL parameters --- //

  // The vertex shader source code in a string. Shader programs are auto-compiled by the component on change.
  vert: string,
  // The fragment shader source code in a string. Shader programs are auto-compiled by the component on change.
  frag: string,

  // Defines the background color drawn first on each render, before `gl.drawArrays`
  clearColor?: vec4,

  // Defines values for vertex attributes, where the key should match the name of the attribute in the vertex shader.
  // This might look like: `in (vec[234]|float) <name>` in the vertex shader. e.g. `in vec4 position`.
  attributes?: GLAttributes,

  // The number of vertices to render when calling `gl.drawArrays`
  vertexCount?: number,

  // Defines float for float vector values for uniforms, where the key should match the name of the uniform in the
  // vertex or fragment shader. This might look like: `uniform (vec[234]|float) <name>` in the shader.
  uniforms?: GLUniforms,

  // Defines samplers for the fragment shader (textures). These are passed as a `HTMLImageElement` or
  // `HTMLVideoElement`, which is automatically converted into a texture.
  samplers?: GLSamplers,
}

/**
 * This is the WebGLCanvas without the `memo()` wrapper.
 *
 * A component that allows for a vertex and fragment shader to be rendered to a `HTMLCanvasElement` with the given
 * shader source code, vertex count, attributes, uniforms and samplers. Alternatively, the WebGLCanvas canvas is defined
 * using various custom hooks, (`useGL`, `useProgram`, `useAttributes`, `useUniforms`, and `useSamplers`) which can be
 * used to create more specific WebGL powered components.
 * @constructor
 */
const UnmemoedWebGLCanvas: React.FC<IWebGLCanvasProps> = ({
                                                            gl,
                                                            vert, frag,
  vertexCount,
  attributes,
  clearColor,
  uniforms,
  samplers,
  width,
  height,
  resolution,
  ...canvasProps
}) => {
  //const canvasRef = useRef<HTMLCanvasElement>(null);
  //const gl = useGL(canvasRef);

  const program = useProgram(gl, vert, frag);

  // Effect to update the clear color
  useEffect(() => {
    gl.gl?.clearColor(...(clearColor ?? [0, 0, 0, 0]));
  }, [gl.gl, clearColor]);


  // Apply uniforms
  useUniforms(gl, program, uniforms);

  // Apply vertex attributes. (might include a position buffer of some kind?)
  useAttributes(program, attributes);

  // Applies samplers. Using frameID as a dependency allows for re-renders on video frame updates.
  const frameID = useSamplers(gl.gl, program, samplers)

  // Effect to redraw the canvas
  useEffect(() => {
    if (!gl.gl || !program)
      return;

    // Redraw
    gl.gl.clear(gl.gl.COLOR_BUFFER_BIT);
    // TODO: allow the mode to be specified, rather than being hard coded as `gl.TRIANGLE_STRIP`
    gl.gl.drawArrays(gl.gl.TRIANGLE_STRIP, 0, vertexCount ?? 4);
  }, [gl.gl, program, uniforms, attributes, samplers, vertexCount, frameID]);

  return <canvas
    {...canvasProps}
    width={resolution?.[0] ?? width}
    height={resolution?.[1] ?? height}
    ref={gl.canvasRef}
  />;
}

/**
 * A component that allows for a vertex and fragment shader to be rendered to a `HTMLCanvasElement` with the given
 * shader source code, vertex count, attributes, uniforms and samplers. Alternatively, the WebGLCanvas canvas is defined
 * using various custom hooks, (`useGL`, `useProgram`, `useAttributes`, `useUniforms`, and `useSamplers`) which can be
 * used to create more specific WebGL powered components.
 * @constructor
 */
const WebGLCanvas = memo(UnmemoedWebGLCanvas);
export default WebGLCanvas;