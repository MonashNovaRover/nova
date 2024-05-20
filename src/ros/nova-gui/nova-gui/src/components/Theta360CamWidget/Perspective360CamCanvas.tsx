import React, {useCallback, useState} from "react";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import useGL from "../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "./gl/perspective.vert";
import Frag from "./gl/perspective.frag";
import useScreenQuadAttribute from "../../hooks/webgl/program/attribute/useScreenQuadAttribute.ts";
import useSampler from "../../hooks/webgl/program/sampler/useSampler.ts";
import HTMLTextureFormat from "../../hooks/webgl/program/sampler/HTMLTextureFormat.ts";
import useResolutionUniform from "../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import useUniform, {vec} from "../../hooks/webgl/program/uniform/useUniform.ts";
import GLWrapMode from "../../hooks/webgl/program/sampler/GLWrapMode.ts";

const DEG_TO_RAD = 0.0174532925199;
export interface WebGL360CamProps {
  image?: HTMLImageElement,
  children?: React.ReactNode
}

const enableScroll = () => {
  document.removeEventListener('wheel', preventDefault, false)
}

const disableScroll = () => {
  document.addEventListener('wheel', preventDefault, {
    passive: false,
  })
}

const preventDefault = (e: Event) => {
  e = e || window.event
  if (e.preventDefault) {
    e.preventDefault()
  }
  e.returnValue = false
}

const Perspective360CamCanvas: React.FC<WebGL360CamProps> = (props) => {
  const gl = useGL();
  const [mousePos, setMousePos] = useState([0, 0]);
  const [fov, setFov] = useState(90);

  // Allow for panning with the mouse
  const onMouseMove = useCallback((event: React.MouseEvent<HTMLCanvasElement>) => {
    if (event.buttons !== 1)
      return;

    const bounds = gl.canvasRef.current?.getBoundingClientRect() ?? {width: 1, height: 1};
    const maxResolutionComp = Math.max(bounds.width, bounds.height);

    setMousePos(([x, y]) => [
      x + fov * DEG_TO_RAD * event.movementX / maxResolutionComp,
      y + fov * DEG_TO_RAD * event.movementY / maxResolutionComp
    ]);
  }, [fov, gl.canvasRef]);

  // Listen to scrolling to change FOV
  const onWheel = useCallback((e: React.WheelEvent<HTMLCanvasElement>) => {
    setFov((fov) => Math.max(Math.min((fov + e.deltaY / 50), 179), 0.01));
  }, []);

  // Create program to project and render image
  const program = useProgram(gl, Vert, Frag);
  useScreenQuadAttribute(program);
  useResolutionUniform(gl, program);
  useSampler(program, 0, "camera", props.image, {
    format: HTMLTextureFormat.RGB,
    wrapS: GLWrapMode.MIRRORED_REPEAT,
    wrapT: GLWrapMode.MIRRORED_REPEAT
  });
  useUniform(program, "fov", () => [fov], [fov]);
  useUniform(program, "mousePos", () => mousePos as vec, [mousePos]);

  return (
    <AutosizedGLCanvas
      gl={gl}
      className="aspect-[16/9] rounded p-3"
      onMouseMove={onMouseMove}
      onWheel={onWheel}
      onMouseEnter={disableScroll}
      onMouseLeave={enableScroll}
    >
      {props.children}
    </AutosizedGLCanvas>
  )
}

export default Perspective360CamCanvas;

