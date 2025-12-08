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

    const absoluteSizeTarget = sampler ?? gl.canvasRef.current.parentElement ?? gl.canvasRef.current;

    const boxObserver = new ResizeObserver((entries) => {
      const entry = entries.find((entry) => entry.target === absoluteSizeTarget);

      if (!entry)
        return;

      const width = entry.devicePixelContentBoxSize[0].inlineSize;
      const height = entry.devicePixelContentBoxSize[0].blockSize;

      frameID = program.queue.update(frameID, (context, program) => {
        const location = context.getUniformLocation(program, name);

        if (location === null)
          return;

        if (!sampler) {
          context.uniform2f(location, width, height);
        }
        else if (sampler instanceof HTMLVideoElement) {
          context.uniform2f(location, sampler.videoWidth, sampler.videoHeight);
        }
        else {
          context.uniform2f(location, sampler.naturalWidth, sampler.naturalHeight);
        }
      });
    });
    boxObserver.observe(absoluteSizeTarget, { box: "device-pixel-content-box" });

    return () => {
      boxObserver.disconnect();

      program.queue.cancel(frameID);
    };
  }, [gl, program, name, sampler]);
}