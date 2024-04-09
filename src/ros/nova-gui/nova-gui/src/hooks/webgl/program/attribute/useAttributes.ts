import {useRef} from "react";
import GLProgramState from "../GLProgramState.ts";
import useProgramEffect from "../useProgramEffect.ts";

export interface IVertexAttribute {
  numComponents: number,
  data: number[]
}

export type GLAttributes = {[key: string] : IVertexAttribute};

const useAttributes = (program: GLProgramState, attributes?: GLAttributes) => {
  const buffersRef = useRef<Record<string, WebGLBuffer | undefined>>({});

  useProgramEffect(program, (gl, program) => {
    if (attributes === undefined)
      return;

    const newBuffersArray = Object.entries(attributes).map(([key, attribute]) => {
      // Create a buffer for the positions.
      const buffer = buffersRef.current[key] ?? gl.createBuffer() ?? undefined;

      if (!buffer)
        return [key, undefined];

      // Select the positionBuffer as the one to apply buffer operations to from here out.
      gl.bindBuffer(gl.ARRAY_BUFFER, buffer);

      // Now pass the list of positions into WebGL to build the shape. We do this by creating a Float32Array from the
      // JavaScript array, then use it to fill the current buffer.
      gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(attribute.data), gl.STATIC_DRAW);

      // setPositionAttribute
      const type = gl.FLOAT; // the data in the buffer is 32bit floats
      const normalize = false; // don't normalize
      const stride = 0; // how many bytes to get from one set of values to the next
      // 0 = use type and numComponents above
      const offset = 0; // how many bytes inside the buffer to start from

      gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
      const attributePosition = gl.getAttribLocation(program, key);

      gl.vertexAttribPointer(
        attributePosition,
        attribute.numComponents,
        type,
        normalize,
        stride,
        offset,
      );
      gl.enableVertexAttribArray(attributePosition);

      return [key, buffer];
    });

    buffersRef.current = Object.fromEntries(newBuffersArray);
    // console.log("Attributes updated");
  }, [attributes]);
}

export default useAttributes;