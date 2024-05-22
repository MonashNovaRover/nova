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
import {Image} from "react-feather";
import ExtendedDownloadButton from "../shared/ExtendedDownload.tsx";
import {Tooltip} from "@nextui-org/react";

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

// https://stackoverflow.com/questions/64891555/convert-base64-image-to-jpeg
function convertToBlob(base64image: string): [ArrayBuffer] {
  if (base64image.length === 0)
    return [new ArrayBuffer(0)];

  // convert base64 to raw binary data held in a string
  const byteString = atob(base64image.split(',')[1]);

  // write the bytes of the string to an ArrayBuffer
  const ab = new ArrayBuffer(byteString.length);
  const dw = new DataView(ab);

  for (let i = 0; i < byteString.length; i++) {
    dw.setUint8(i, byteString.charCodeAt(i));
  }

  // write the ArrayBuffer to a blob, and you're done
  return [ab]
}


const Perspective360CamCanvas: React.FC<WebGL360CamProps> = (props) => {
  const gl = useGL();
  const [mousePos, setMousePos] = useState([0, 0]);

  // Allow for panning with the mouse
  const onMouseMove = useCallback((event: React.MouseEvent<HTMLCanvasElement>) => {
    if (event.buttons !== 1)
      return;

    const bounds = gl.canvasRef.current?.getBoundingClientRect() ?? {width: 1, height: 1};
    const maxResolutionComp = Math.max(bounds.width, bounds.height);

    setMousePos(([x, y]) => [
      x + event.movementX / maxResolutionComp,
      y + event.movementY / maxResolutionComp
    ]);
  }, [gl.canvasRef]);

  // Create program to project and render image
  const program = useProgram(gl, Vert, Frag);
  useScreenQuadAttribute(program);
  useResolutionUniform(gl, program);
  useSampler(program, 0, "camera", props.image, {
    format: HTMLTextureFormat.RGB,
    wrapS: GLWrapMode.MIRRORED_REPEAT,
    wrapT: GLWrapMode.MIRRORED_REPEAT
  });
  useUniform(program, "mousePos", () => mousePos as vec, [mousePos]);

  // Called for the screenshot button, to fetch data to put in the file to save
  const getCanvasScreenshot = useCallback(() => {
    if (!gl.canvasRef.current)
      return [];

    const canvas = gl.canvasRef.current;
    gl.render(true);
    const dataURL = canvas.toDataURL("image/png");

    return convertToBlob(dataURL);
  }, [gl])

  return (
    <div className="flex flex-col gap-3">
      <AutosizedGLCanvas
        gl={gl}
        className="aspect-[16/9] rounded p-3"
        onMouseMove={onMouseMove}
        onMouseEnter={disableScroll}
        onMouseLeave={enableScroll}
      >
        <div className="flex flex-row gap-3">
          {props.children}
          <Tooltip content="Take WebGL Screenshot">
            <ExtendedDownloadButton
              fileContent={getCanvasScreenshot}
              filename={`360cam-panorama.png`}
              fileType={`image/png`}
              isIconOnly
            >
              <Image></Image>
            </ExtendedDownloadButton>
          </Tooltip>
        </div>

      </AutosizedGLCanvas>
    </div>
  )
}

export default Perspective360CamCanvas;

