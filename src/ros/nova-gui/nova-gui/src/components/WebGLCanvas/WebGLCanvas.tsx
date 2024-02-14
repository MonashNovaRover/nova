import React, {memo, useEffect, useRef, useState} from "react";
import useGL from "./useGL.tsx";
import {useProgram} from "./useProgram.tsx";



export type vec2 = [number, number];
export type vec4 = [number, number, number, number];

export interface IWebGLCanvasProps {
  vert: string,
  frag: string,
  uniformFloats?: {[Name: string]: number},

  // canvas props
  className?: string,
  width?: number,
  height?: number,

  vertexAttributes: {[key: string] : number[][]}

  positions?: number[]
}



const UnmemoedWebGLCanvas: React.FC<IWebGLCanvasProps> = ({
  vert, frag, uniformFloats, positions, vertexAttributes, ...canvasProps
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const gl = useGL(canvasRef);
  const program = useProgram(gl, vert, frag);

  useEffect(() => {
    if (!gl || !positions || !program)
      return;

    // Create a buffer for the positions.
    const positionBuffer = gl.createBuffer();

    // Select the positionBuffer as the one to apply buffer operations to from here out.
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);

    // Now pass the list of positions into WebGL to build the shape. We do this by creating a Float32Array from the
    // JavaScript array, then use it to fill the current buffer.
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

    // setPositionAttribute
    const numComponents = 2; // pull out 2 values per iteration
    const type = gl.FLOAT; // the data in the buffer is 32bit floats
    const normalize = false; // don't normalize
    const stride = 0; // how many bytes to get from one set of values to the next
    // 0 = use type and numComponents above
    const offset = 0; // how many bytes inside the buffer to start from

    const vertexAttributePosition = gl.getAttribLocation(program, "a_position");

    gl.vertexAttribPointer(
      vertexAttributePosition,
      numComponents,
      type,
      normalize,
      stride,
      offset,
    );
    gl.enableVertexAttribArray(vertexAttributePosition);


    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

    return () => {
      gl.deleteBuffer(positionBuffer);
    }

  }, [gl, positions, program]);

  useEffect(() => {
    if (!gl)
      return;

    // Compile shader program
    // const shaderProgram = initShaderProgram(gl, vert, frag);

    // Set clear color to black, fully opaque
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    // Clear the color buffer with specified clear color
    gl.clear(gl.COLOR_BUFFER_BIT);

  }, [vert, frag, gl]);




  useEffect(() => {
    // Update program uniforms


  }, [uniformFloats]);

  return (
    <canvas {...canvasProps} ref={canvasRef}></canvas>
  );
}

const WebGLCanvas = memo(UnmemoedWebGLCanvas);
export default WebGLCanvas;