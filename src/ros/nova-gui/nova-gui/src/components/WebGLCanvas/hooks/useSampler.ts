import {useEffect, useState} from "react";
import {ProgramWithGL} from "./useProgram.tsx";
import loadVideoTexture, {updateVideoTexture} from "../webgl-utils/loadVideoTexture.ts";
import loadTexture, {updateImageTexture} from "../webgl-utils/loadTexture.ts";

/**
 * Applies a video or an image element as a sampler for a webgl program.
 * @param program The object returned from the useProgram hook.
 * @param sampler The video or image to use as the source for the texture.
 * @param textureUnit the index of the texture unit to use. Must be unique.
 * @param name A string specifying the name of the uniform variable whose location is to be returned
 */
export default function useSampler(program: ProgramWithGL | undefined, sampler: HTMLImageElement | HTMLVideoElement | null | undefined, textureUnit: number, name: string) {
  const [texture, setTexture] = useState<WebGLTexture | null>(null)
  const [samplerFrameID, setSamplerFrameID] = useState<number>(0);

  useEffect(() => {
    if (!program?.gl || !program?.program || sampler === null || sampler === undefined)
      return;

    // Flip image pixels into the bottom-to-top order that WebGL expects.
    program.gl.pixelStorei(program.gl.UNPACK_FLIP_Y_WEBGL, true);

    const location = program.gl.getUniformLocation(program.program, name)

    const currentTexture = texture ?? (sampler instanceof HTMLVideoElement
      ? loadVideoTexture(program.gl, sampler)
      : loadTexture(program.gl, sampler));

    // Return if we failed to create the texture for the given sampler.
    if (!currentTexture)
      return;

    // Bind the texture to texture unit 0
    program.gl.activeTexture(program.gl.TEXTURE0 + textureUnit);
    program.gl.bindTexture(program.gl.TEXTURE_2D, currentTexture);

    // Tell the shader we bound the texture to texture unit 0
    program.gl.uniform1i(location, textureUnit);

    if (texture) {
      // If the texture already existed, and wasn't created this iteration, update the texture.
      if (sampler instanceof HTMLVideoElement)
        updateVideoTexture(program.gl, sampler, texture);
      else
        updateImageTexture(program.gl, sampler, texture);
    }
    else {
      // Otherwise, persist the newly generated texture
      setTexture(currentTexture);
    }

    if (sampler instanceof HTMLVideoElement) {
      // Create a callback to run each time a frame is updated. The id of the requestVideoFrameCallback is stored in
      // frameIDs, so that it can be destroyed by the cleanup function.
      const callback = () => {
        updateVideoTexture(program.gl, sampler, currentTexture);
        setSamplerFrameID(sampler.requestVideoFrameCallback(callback));
      }
      setSamplerFrameID(sampler.requestVideoFrameCallback(callback));
    }
    else {
      updateImageTexture(program.gl, sampler, currentTexture);
    }

    return () => {
      if (program.gl === undefined || sampler === undefined)
        return;

      // Cancel all video frame callbacks
      if (sampler instanceof HTMLVideoElement)
        sampler.cancelVideoFrameCallback(samplerFrameID);
    }
  }, [program, sampler, name, textureUnit])

  return samplerFrameID;
}






