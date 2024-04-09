import {useState} from "react";
import loadVideoTexture, {updateVideoTexture} from "../webgl-utils/loadVideoTexture.ts";
import loadTexture, {updateImageTexture} from "../webgl-utils/loadTexture.ts";
import useProgramEffect from "./program/useProgramEffect.ts";
import GLProgramState from "./program/GLProgramState.ts";

/**
 * Applies a video or an image element as a sampler for a webgl program.
 * @param program The object returned from the useProgram hook.
 * @param sampler The video or image to use as the source for the texture.
 * @param textureUnit the index of the texture unit to use. Must be unique.
 * @param name A string specifying the name of the uniform variable whose location is to be returned
 */
export default function useSampler(program: GLProgramState, textureUnit: number, name: string, sampler: HTMLImageElement | HTMLVideoElement | null | undefined) {
  const [texture, setTexture] = useState<WebGLTexture | null>(null)
  const [samplerFrameID, setSamplerFrameID] = useState<number>(0);

  useProgramEffect(program, (gl, program) => {
    if (sampler === null || sampler === undefined)
      return;

    // Flip image pixels into the bottom-to-top order that WebGL expects.
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    const location = gl.getUniformLocation(program, name)

    const currentTexture = texture ?? (sampler instanceof HTMLVideoElement
      ? loadVideoTexture(gl, sampler)
      : loadTexture(gl, sampler));

    // Return if we failed to create the texture for the given sampler.
    if (!currentTexture)
      return;

    // Bind the texture to texture unit 0
    gl.activeTexture(gl.TEXTURE0 + textureUnit);
    gl.bindTexture(gl.TEXTURE_2D, currentTexture);

    // Tell the shader we bound the texture to texture unit 0
    gl.uniform1i(location, textureUnit);

    if (texture) {
      // If the texture already existed, and wasn't created this iteration, update the texture.
      if (sampler instanceof HTMLVideoElement)
        updateVideoTexture(gl, sampler, texture);
      else
        updateImageTexture(gl, sampler, texture);
    }
    else {
      // Otherwise, persist the newly generated texture
      setTexture(currentTexture);
    }

    if (sampler instanceof HTMLVideoElement) {
      // Create a callback to run each time a frame is updated. The id of the requestVideoFrameCallback is stored in
      // frameIDs, so that it can be destroyed by the cleanup function.
      const callback = () => {
        updateVideoTexture(gl, sampler, currentTexture);
        setSamplerFrameID(sampler.requestVideoFrameCallback(callback));
      }
      setSamplerFrameID(sampler.requestVideoFrameCallback(callback));
    }
    else {
      updateImageTexture(gl, sampler, currentTexture);
    }

    return () => {
      if (gl === undefined || sampler === undefined)
        return;

      // Cancel all video frame callbacks
      if (sampler instanceof HTMLVideoElement)
        sampler.cancelVideoFrameCallback(samplerFrameID);
    }
  }, [sampler, name, textureUnit])

  return samplerFrameID;
}






