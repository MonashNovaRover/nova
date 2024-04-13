import GLProgramState from "../GLProgramState.ts";
import {useLayoutEffect} from "react";
import GLState from "../../gl/GLState.ts";

/**
 * Sets a vec2 which is equal to the size of the webgl canvas in pixels, under the name "resolution"
 */
export default function useResolutionUniform(gl: GLState, program: GLProgramState, name: string = "resolution", sampler?: HTMLImageElement | HTMLVideoElement | null) {

  useLayoutEffect(() => {
    if (!gl.canvasRef.current)
      return;

    let frameID = -1;

    const onResize = () => {
      program.queue.cancel(frameID);

      frameID = program.queue.push((context, program) => {
        const location = context.getUniformLocation(program, name);

        if (location === null)
          return;

        if (!sampler) {
          context.uniform2f(location, context.drawingBufferWidth, context.drawingBufferHeight);
        }
        else if (sampler instanceof HTMLVideoElement) {
          context.uniform2f(location, sampler.videoWidth, sampler.videoHeight);
        }
        else {
          context.uniform2f(location, sampler.naturalWidth, sampler.naturalHeight);
        }
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
  }, [gl, program, name, sampler]);
  
}