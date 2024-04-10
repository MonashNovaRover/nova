import GLProgramState from "../GLProgramState.ts";
import {useLayoutEffect} from "react";
import GLState from "../../gl/GLState.ts";

/**
 * Sets a vec2 which is equal to the size of the webgl canvas in pixels, under the name "resolution"
 */
export default function useResolutionUniform(gl: GLState, program: GLProgramState) {

  useLayoutEffect(() => {
    if (!gl.canvasRef.current)
      return;

    let frameID = -1;

    const onResize = () => {
      program.queue.cancel(frameID);

      frameID = program.queue.push((context, program) => {
        const location = context.getUniformLocation(program, "resolution");

        if (location === null)
          return;

        context.uniform2f(location, context.drawingBufferWidth, context.drawingBufferHeight);
      });
    }

    const resizeObserver = new ResizeObserver(onResize);
    resizeObserver.observe(gl.canvasRef.current);
    const observer = new MutationObserver(onResize);
    observer.observe(gl.canvasRef.current, {attributes: true, attributeFilter: ["style", "width", "height"]});

    return () => {
      resizeObserver.disconnect();
      observer.disconnect();

      program.queue.cancel(frameID);
    };
  }, [gl, program]);
  
}