import {useEffect, useState} from "react";
import loadTexture, {updateImageTexture} from "../webgl-utils/loadTexture.ts";
import loadVideoTexture, {updateVideoTexture} from "../webgl-utils/loadVideoTexture.ts";
import "rvfc-polyfill"

export type GLSampler = HTMLImageElement | HTMLVideoElement | null | undefined;
export type GLSamplers = {[key: string] : GLSampler}; // Map<string, GLSampler>;



const useSamplers = (gl?: WebGL2RenderingContext, program?: WebGLProgram, samplers?: GLSamplers) => {



  const [textures, setTextures] = useState<{[key: string] : WebGLTexture | undefined}>({});
  const [samplerFrameID, setSamplerFrameID] = useState<number>(0);

  useEffect(() => {
    if (gl === undefined || samplers === undefined || program === undefined)
      return;

    const frameIDs = new Array(samplers.length).map(() => 0);

    // Flip image pixels into the bottom-to-top order that WebGL expects.
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    // Create a texture for every sampler
    const newTex = Object.entries(samplers).map(([key, sampler], index) => {
      if (sampler === undefined || sampler === null)
        return [key, undefined]; // Skip undefined samplers

      const cachedTexture = textures[key];

      if (sampler instanceof HTMLImageElement) {
        // The sampler is an image
        if (cachedTexture) {
          updateImageTexture(gl, sampler, cachedTexture);

          const location = gl.getUniformLocation(program, key);

          if (!location) {
            // This sampler does not exist in the program
            console.warn(`Sampler with name '${key}' does not exist in the shader program!`);
            return [key, undefined];
          }
          console.log(`Sampler with name '${key}' exists in the shader program!`);

          gl.uniform1i(location, index);

          return [key, cachedTexture];
        }

        const location = gl.getUniformLocation(program, key);
        if (!location) {
          // This sampler does not exist in the program
          console.warn(`Sampler '${key}' does not exist in the shader program!`);
          return [key, undefined];
        }
        console.log(`Sampler with name '${key}' exists in the shader program!`);

        const texture = loadTexture(gl, sampler);

        if (!texture)
          return [key, undefined]; // Failed to load into texture; continue to next sampler

        gl.activeTexture(gl.TEXTURE0 + index);

        // Bind the texture to texture unit 0
        gl.bindTexture(gl.TEXTURE_2D, texture);

        // Tell the shader we bound the texture to texture unit 0
        gl.uniform1i(location, index);

        return [key, texture];
      }
      else if (sampler instanceof HTMLVideoElement) {
        // The sampler is a video
        const video = sampler;

        // Create a callback to run each time a frame is updated. The id of the requestVideoFrameCallback is stored in
        // frameIDs, so that it can be destroyed by the cleanup function.
        const callback = () => {
          updateVideoTexture(gl, video, texture);

          const newSamplerFrameID = frameIDs.reduce((x,y) => x+y, 0)
          setSamplerFrameID(newSamplerFrameID);
          frameIDs[index] = video.requestVideoFrameCallback(callback);
        }

        const startCallback = () => {
          frameIDs[index] = video.requestVideoFrameCallback(callback);
        }

        let texture = cachedTexture;

        if (!cachedTexture) {
          const location = gl.getUniformLocation(program, key);

          if (!location) {
            // This sampler does not exist in the program
            console.warn(`Sampler with name '${key}' does not exist in the shader program!`);
            return [key, undefined];
          }
          console.log(`Sampler with name '${key}' exists in the shader program!`);

          // The sampler is an image
          texture = cachedTexture ?? loadVideoTexture(gl, video);

          if (!texture)
            return [key, undefined]; // Failed to load into texture; continue to next sampler

          gl.activeTexture(gl.TEXTURE0 + index);

          // Bind the texture to texture unit 0
          gl.bindTexture(gl.TEXTURE_2D, texture);

          // Tell the shader we bound the texture to the given texture unit
          gl.uniform1i(location, index);


        }


        startCallback();
        return [key, texture];
      }
      return [key, undefined];
    });

    setTextures(Object.fromEntries(newTex));

    return () => {
      Object.entries(samplers).forEach(([, sampler], index) => {
        if (gl === undefined || samplers === undefined || program === undefined)
          return;

        // Cancel all video frame callbacks
        if (sampler instanceof HTMLVideoElement)
          sampler.cancelVideoFrameCallback(frameIDs[index]);
      });
    }
  }, [program, samplers]);


  return samplerFrameID;
}

export default useSamplers;