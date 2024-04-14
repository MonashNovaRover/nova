import GLProgramState from "../GLProgramState.ts";
import {DependencyList, useRef} from "react";
import useProgramEffect from "../useProgramEffect.ts";
import useProgramRenderEffect from "../useProgramRenderEffect.ts";

export type vecArray = [number][] | [number, number][] | [number, number, number][] | [number, number, number, number][];

export interface UseAttributeOptions {
  normalize: boolean
  // how many bytes to get from one set of values to the next
  stride: number,
  // how many bytes inside the buffer to start from
  offset: number,
}

/**
 * Applies float vector or float vertex attribute values to a given program. The given name of the attribute should match
 * those of the attribute defined in the program.
 * @param program The result of `useProgram`
 * @param name The name of the uniform in the program
 * @param factoryOrAttribute The uniform float or vector value, or a function to generate it when a dependency changes.
 * @param deps The dependency array, that should cause the uniform to be reset when changed.
 * @param options Additional options to configure the attribute
 */
export default function useAttributed(program: GLProgramState, name: string,
                                      factoryOrAttribute: (() => vecArray) | vecArray, deps: DependencyList = [],
                                      options?: Partial<UseAttributeOptions>) {
  const buffer = useRef<WebGLBuffer>();

  const filledOptions = {
    normalize: false,
    stride: 0,
    offset: 0,
    ...options
  }

  const numComponentsRef = useRef<number>();

  useProgramEffect(program, (context, program) => {
    context.useProgram(program);

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
    numComponentsRef.current = numComponents;

    const attributeLocation = context.getAttribLocation(program, name);
    //context.disableVertexAttribArray(attributeLocation);
    const attributeData= new Float32Array(attribute.flatMap(v => v));

    // Select buffer as the buffer to apply buffer operations to from here out.
    context.bindBuffer(context.ARRAY_BUFFER, buffer.current);

    // Now pass the list of positions into WebGL to build the shape. We do this by creating a Float32Array from the
    // JavaScript array, then use it to fill the current buffer.
    // If this has any dependencies in the dependency array, then it can vary. Otherwise, it never changes. So, we can
    // set STATIC_DRAW if it never changes, and DYNAMIC_DRAW if it does.
    context.bufferData(context.ARRAY_BUFFER, attributeData,
      deps.length === 0 ? context.STATIC_DRAW : context.DYNAMIC_DRAW);

    context.vertexAttribPointer(
      attributeLocation,
      numComponents,
      context.FLOAT,
      filledOptions.normalize,
      filledOptions.stride,
      filledOptions.offset,
    );

    context.enableVertexAttribArray(attributeLocation);
  }, [name, ...deps])

  useProgramRenderEffect(program, (context, program) => {
    if (!buffer.current)
      return;

    context.bindBuffer(context.ARRAY_BUFFER, buffer.current);
    const attributeLocation = context.getAttribLocation(program, name);
    context.vertexAttribPointer(
      attributeLocation,
      numComponentsRef.current ?? 2,
      context.FLOAT,
      filledOptions.normalize,
      filledOptions.stride,
      filledOptions.offset,
    );
  }, [])
}