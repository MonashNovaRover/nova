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
import {Input, Tooltip} from "@nextui-org/react";
import {useGenericStore} from "../../hooks/useGenericStore.ts";

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
  const [fov, setFov] = useState(90);
  const [widthHeight, setWidthHeight] = useState([0, 0]);

  const [inputDistance, setInputDistance] = useGenericStore<string>("theta360InputDistance");
  const [inputThetaHigh, setInputThetaHigh] = useGenericStore<string>("theta360InputThetaHigh");
  const [inputThetaLow, setInputThetaLow] = useGenericStore<string>("theta360InputThetaLow");
  const [landmarkHeight, setLandmarkHeight] = useState<number>(0);

  // Allow for panning with the mouse
  const onMouseMove = useCallback((event: React.MouseEvent<HTMLCanvasElement>) => {
    if (!gl.canvasRef.current)
      return;

    const bounds = gl.canvasRef.current.getBoundingClientRect();

    const pixelsX = event.clientX - bounds.left;
    const pixelsY = event.clientY - bounds.top;

    const x = pixelsX / bounds.width;
    const y = pixelsY / bounds.height;

    setMousePoint([x, y]);
    setWidthHeight([bounds.width, bounds.height]);

    if (event.buttons !== 1)
      return;
    
    const maxResolutionComp = Math.max(bounds.width, bounds.height);

    setMousePos(([x, y]) => [
      x + fov * DEG_TO_RAD * event.movementX / maxResolutionComp,
      y + fov * DEG_TO_RAD * event.movementY / maxResolutionComp,
    ]);
  }, [fov, gl.canvasRef]);

  const [mousePoint, setMousePoint] = useState<[number, number]>([0, 0]);
  // Function that converts y values from 0 to 1 into an angle relative to the midpoint of the image
  const yToTheta = useCallback((v: number) => 360* widthHeight[1]/widthHeight[0] * (-v + 0.5) , [widthHeight]);

  // Calculate height of landmark
  const getHeight = useCallback( () => {
    const radThetaHigh = Number(inputThetaHigh)*Math.PI/180;
    const radThetaLow = Number(inputThetaLow)*Math.PI/180;
    (setLandmarkHeight( Number(inputDistance)*( Math.tan(radThetaHigh) - Math.tan(radThetaLow) ) ) );
  }, [setLandmarkHeight, inputDistance, inputThetaHigh, inputThetaLow]);

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


  // Called for the screenshot button, to fetch data to put in the file to save
  const getCanvasScreenshot = useCallback(() => {
    if (!gl.canvasRef.current)
      return [];

    const canvas = gl.canvasRef.current;
    gl.render(true);
    const dataURL = canvas.toDataURL("image/png");

    return convertToBlob(dataURL);
  }, [gl])

  // input fields for height calculation
  const distField = (
    <div className="flex flex-row p-1 justify-center">
      <Input
        className="w-3/4"
        label="Distance"
        placeholder="0.0m"
        value={inputDistance}
        onChange={(e) => {
          const v = e.target.value;
          setInputDistance(v);
          getHeight();
        }}
      />
    </div>
  )
  const lowThetaField = (
    <div className="flex flex-row p-1 justify-center">
      <Input
        className="w-3/4"
        label="Lower Angle"
        placeholder="0.0deg"
        value={inputThetaLow}
        onChange={(e) => {
          const v = e.target.value;
          setInputThetaLow(v);
          getHeight();
        }}
      />
    </div>
  )
  const highThetaField = (
    <div className="flex flex-row p-1 justify-center">
      <Input
        className="w-3/4"
        label="Upper Angle"
        placeholder="0.0deg"
        value={inputThetaHigh}
        onChange={(e) => {
          const v = e.target.value;
          setInputThetaHigh(v);
          getHeight();
        }}
      />
    </div>
  )

  return (
    <div className="flex flex-col gap-2.5 flex-grow">
      ({mousePoint[0].toFixed(3)}, {mousePoint[1].toFixed(3)}) -{'>'} {yToTheta(mousePoint[1]).toFixed(2)} deg
    <AutosizedGLCanvas
      gl={gl}
      className="rounded p-3 flex-grow"
      onMouseMove={onMouseMove}
      onWheel={onWheel}
      onMouseEnter={disableScroll}
      onMouseLeave={enableScroll}
    >
      <div className="flex flex-row gap-3">
        {props.children}
        <Tooltip content="Take WebGL Screenshot">
          <ExtendedDownloadButton
            fileContent={getCanvasScreenshot}
            filename={`360cam-perspective.png`}
            fileType={`image/png`}
            isIconOnly
          >
            <Image></Image>
          </ExtendedDownloadButton>
        </Tooltip>
      </div>
    </AutosizedGLCanvas>
      <div className="flex flex-row gap-9 items-center">
        {distField}
        {lowThetaField}
        {highThetaField}
        Height of landmark: {landmarkHeight.toPrecision(2)}m
      </div>
    </div>
  )
}

export default Perspective360CamCanvas;

