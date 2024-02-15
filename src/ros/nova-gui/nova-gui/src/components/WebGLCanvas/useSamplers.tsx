import {useEffect} from "react";
import loadTexture from "./loadTexture.ts";

const useUniforms = (gl?: WebGL2RenderingContext, program?: WebGLProgram, samplers?: {[key: string] : HTMLImageElement | HTMLVideoElement}) => {



  useEffect(() => {
    if (gl === undefined || samplers === undefined || program === undefined)
      return;

    // Flip image pixels into the bottom-to-top order that WebGL expects.
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    // Create a texture for every sampler
    Object.entries(samplers).forEach(([key, sampler]) => {

      console.log(`type of ${key} is ${sampler instanceof HTMLVideoElement ? "video" : "image"}`, sampler);

      if (sampler instanceof HTMLVideoElement) {
        const video = sampler;


      }
      else {
        const image = sampler;

        const texture = loadTexture(gl, image);

        if (!texture)
          return; // Continue to next sampler



        const location = gl.getUniformLocation(program, key);

        gl.activeTexture(gl.TEXTURE0);

        // Bind the texture to texture unit 0
        gl.bindTexture(gl.TEXTURE_2D, texture);

        // Tell the shader we bound the texture to texture unit 0
        gl.uniform1i(location, 0);
      }
    });
  }, [gl, program, samplers]);


}

export default useUniforms;