import {useEffect, useState} from "react";
import {ProgramWithGL} from "./program/useProgram.tsx";

export interface IVertexAttribute {
  numComponents: number
  data: number[]
}

export type GLAttributes = {[key: string] : IVertexAttribute};

const useAttributes = (programWithGL?: ProgramWithGL, attributes?: GLAttributes) => {

  const [attributeBuffers, setAttributeBuffers] = useState<Record<string, WebGLBuffer | undefined>>({});

  useEffect(() => {
    if (programWithGL === undefined || attributes === undefined)
      return;

    const gl = programWithGL?.gl;
    const program = programWithGL?.program;

    const newBuffersArray = Object.entries(attributes).map(([key, attribute]) => {

      // Create a buffer for the positions.
      const buffer = attributeBuffers[key] ?? gl.createBuffer() ?? undefined;

      if (!buffer)
        return [key, undefined];

      // Select the positionBuffer as the one to apply buffer operations to from here out.
      gl.bindBuffer(gl.ARRAY_BUFFER, buffer);

      // Now pass the list of positions into WebGL to build the shape. We do this by creating a Float32Array from the
      // JavaScript array, then use it to fill the current buffer.
      gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(attribute.data), gl.STATIC_DRAW);

      // setPositionAttribute
      const numComponents = 2; // pull out 2 values per iteration
      const type = gl.FLOAT; // the data in the buffer is 32bit floats
      const normalize = false; // don't normalize
      const stride = 0; // how many bytes to get from one set of values to the next
      // 0 = use type and numComponents above
      const offset = 0; // how many bytes inside the buffer to start from

      gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
      const attributePosition = gl.getAttribLocation(program, key);

      gl.vertexAttribPointer(
        attributePosition,
        numComponents,
        type,
        normalize,
        stride,
        offset,
      );
      gl.enableVertexAttribArray(attributePosition);

      return [key, buffer];
    });

    setAttributeBuffers(Object.fromEntries(newBuffersArray));
    // console.log("Attributes updated");
  }, [programWithGL, attributes]);
}

export default useAttributes;