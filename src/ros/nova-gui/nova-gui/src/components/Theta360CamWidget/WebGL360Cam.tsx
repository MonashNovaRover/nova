import React from "react";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import useGL from "../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "./gl/perspective.vert";
import Frag from "./gl/perspective.frag";
import useScreenQuadAttribute from "../../hooks/webgl/program/attribute/useScreenQuadAttribute.ts";
import useSampler from "../../hooks/webgl/program/sampler/useSampler.ts";
import HTMLTextureFormat from "../../hooks/webgl/program/sampler/HTMLTextureFormat.ts";
import useResolutionUniform from "../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import useUniform from "../../hooks/webgl/program/uniform/useUniform.ts";


export interface WebGL360CamProps {
  image?: HTMLImageElement,
  children?: React.ReactNode
}

const WebGL360Cam: React.FC<WebGL360CamProps> = (props) => {
  const gl = useGL();

  const program = useProgram(gl, Vert, Frag);
  useScreenQuadAttribute(program);
  useResolutionUniform(gl, program);
  useSampler(program, 0, "camera", props.image, {
    format: HTMLTextureFormat.RGB
  });
  useUniform(program, "fov", () => [80], []);
  useUniform(program, "mousePos", () => [0, 0], []);

  return (
    <AutosizedGLCanvas gl={gl} className="aspect-[16/9] rounded p-3">
      {props.children}
    </AutosizedGLCanvas>
  )
}

export default WebGL360Cam;

