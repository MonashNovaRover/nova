import {useEffect, useRef} from "react";
import GLProgramState from "../GLProgramState.ts";
import GLSamplerState, {GLSamplerStateOptions} from "./GLSamplerState.ts";
import GLTexture2DTargetType from "./GLTexture2DTargetType.ts";
import HTMLTextureFormat from "./HTMLTextureFormat.ts";
import GLTextureWrapMode from "./GLTextureWrapMode.ts";
import {useMappedGLint} from "../MappedGLint.ts";

function useSampler_aux(programState: GLProgramState, textureUnit: number, name: string,
                        sampler: HTMLImageElement | HTMLVideoElement | null | undefined,
                        options?: Partial<GLSamplerStateOptions>): GLSamplerState {
  const samplerRef = useRef<GLSamplerState>();

  if (samplerRef.current === undefined) {
    const filledOptions = {
      target: GLTexture2DTargetType.TEXTURE_2D,
      format: HTMLTextureFormat.RGBA,
      wrapT: GLTextureWrapMode.REPEAT,
      wrapS: GLTextureWrapMode.REPEAT,
      ...options
    }

    samplerRef.current = new GLSamplerState(programState, textureUnit, name, sampler, filledOptions);
  }

  return samplerRef.current;
}

/**
 * Applies a video or an image element as a sampler for a webgl program.
 * @param programState The object returned from the useProgram hook.
 * @param sampler The video or image to use as the source for the texture.
 * @param textureUnit the index of the texture unit to use. Must be unique.
 * @param name A string specifying the name of the uniform variable whose location is to be returned
 * @param options Additional options object allowing other aspects of the sampler, such as wrap behaviour, to be
 * specified
 */
export default function useSampler(programState: GLProgramState, textureUnit: number, name: string,
                              sampler: HTMLImageElement | HTMLVideoElement | null | undefined,
                              options?: Partial<GLSamplerStateOptions>) {
  const samplerState = useSampler_aux(programState, textureUnit, name, sampler, options)

  useEffect(() => {
    samplerState.sampler = sampler;
  }, [sampler, samplerState]);

  useMappedGLint(samplerState.wrapS, options?.wrapS);
  useMappedGLint(samplerState.wrapT, options?.wrapT);
  useMappedGLint(samplerState.format, options?.format);
  useMappedGLint(samplerState.target, options?.target);

  return samplerState;
}




