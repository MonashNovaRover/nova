import {useEffect, useRef} from "react";
import GLProgramState from "../GLProgramState.ts";
import GLSamplerState, {GLSamplerStateOptions} from "./GLSamplerState.ts";
import GLTexture2DTarget from "./GLTexture2DTarget.ts";
import HTMLTextureFormat from "./HTMLTextureFormat.ts";
import GLWrapMode from "./GLWrapMode.ts";
import {useMappedGLint} from "../MappedGLint.ts";

function useSampler_aux(programState: GLProgramState, textureUnit: number, name: string,
                        sampler: HTMLImageElement | HTMLVideoElement | null | undefined,
                        options?: Partial<GLSamplerStateOptions>): GLSamplerState {
  const filledOptions = {
    target: GLTexture2DTarget.TEXTURE_2D,
    format: HTMLTextureFormat.RGBA,
    wrapT: GLWrapMode.REPEAT,
    wrapS: GLWrapMode.REPEAT,
    ...options
  }

  const samplerRef = useRef<GLSamplerState | undefined>(undefined);

  if (samplerRef.current === undefined)
    samplerRef.current = new GLSamplerState(programState, textureUnit, name, sampler, filledOptions);

  useEffect(() => {
    samplerRef.current!.target.value = filledOptions.target;
    samplerRef.current!.format.value = filledOptions.format;
    samplerRef.current!.wrapT.value = filledOptions.wrapT;
    samplerRef.current!.wrapS.value = filledOptions.wrapS;
  }, [filledOptions.target, filledOptions.format, filledOptions.wrapT, filledOptions.wrapS]);

  return samplerRef.current!;
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




