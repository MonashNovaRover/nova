import GLProgramState from "./GLProgramState.ts";
import {IVertexAttribute} from "../useAttributes.tsx";
import {useRef} from "react";
import useProgramEffect from "./useProgramEffect.ts";

export default function useAttribute(program: GLProgramState, name: string, attribute: IVertexAttribute) {

  const buffer = useRef<WebGLBuffer>();

  useProgramEffect(program, (context, program) => {
    // Create the buffer if necessary
    if (buffer.current === undefined) {
      buffer.current = context.createBuffer() ?? undefined;

      // Ensure the creation of the buffer was successful before trying to use it.
      if (buffer.current === undefined)
        return;
    }

    // Select buffer as the buffer to apply buffer operations to from here out.
    context.bindBuffer(context.ARRAY_BUFFER, buffer.current);

    // Now pass the list of positions into WebGL to build the shape. We do this by creating a Float32Array from the
    // JavaScript array, then use it to fill the current buffer.
    context.bufferData(context.ARRAY_BUFFER, new Float32Array(attribute.data), context.STATIC_DRAW);

    // setPositionAttribute
    const type = context.FLOAT; // the data in the buffer is 32bit floats
    const normalize = false; // don't normalize
    const stride = 0; // how many bytes to get from one set of values to the next
    // 0 = use type and numComponents above
    const offset = 0; // how many bytes inside the buffer to start from

    context.bindBuffer(context.ARRAY_BUFFER, buffer.current);
    const attributeLocation = context.getAttribLocation(program, name);

    context.vertexAttribPointer(
      attributeLocation,
      attribute.numComponents,
      type,
      normalize,
      stride,
      offset,
    );
    context.enableVertexAttribArray(attributeLocation);

    console.log("done!");
  }, [attribute, name])



}