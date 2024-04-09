import GLProgramState from "../GLProgramState.ts";
import {DependencyList, useRef} from "react";
import useProgramEffect from "../useProgramEffect.ts";

export type vecArray = [number][] | [number, number][] | [number, number, number][] | [number, number, number, number][];

export default function useAttribute(program: GLProgramState, name: string, constantAttribute: vecArray): void;
export default function useAttribute(program: GLProgramState, name: string, factory: () => vecArray, deps: DependencyList): void;
export default function useAttribute(program: GLProgramState, name: string,
                                     factoryOrAttribute: (() => vecArray) | vecArray, deps: DependencyList = []) {
  const buffer = useRef<WebGLBuffer>();

  useProgramEffect(program, (context, program) => {
    // Create the buffer if necessary
    if (buffer.current === undefined) {
      buffer.current = context.createBuffer() ?? undefined;

      // Ensure the creation of the buffer was successful before trying to use it.
      if (buffer.current === undefined)
        return;
    }

    const attribute = Array.isArray(factoryOrAttribute) ? factoryOrAttribute : factoryOrAttribute();

    if (attribute.length === 0)
      return;

    const numComponents = attribute[0].length;

    const attributeData= new Float32Array(attribute.flatMap(v => v));

    // Select buffer as the buffer to apply buffer operations to from here out.
    context.bindBuffer(context.ARRAY_BUFFER, buffer.current);


    // Now pass the list of positions into WebGL to build the shape. We do this by creating a Float32Array from the
    // JavaScript array, then use it to fill the current buffer.
    context.bufferData(context.ARRAY_BUFFER, attributeData, context.STATIC_DRAW);

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
      numComponents,
      type,
      normalize,
      stride,
      offset,
    );
    context.enableVertexAttribArray(attributeLocation);
  }, [name, ...deps])
}