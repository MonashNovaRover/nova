import React, {useCallback, useState} from "react";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import useGL from "../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "./gl/panorama.vert";
import Frag from "./gl/panorama.frag";
import CompassVert from "./gl/compass.vert";
import CompassFrag from "./gl/compass.frag";
import useScreenQuadAttribute from "../../hooks/webgl/program/attribute/useScreenQuadAttribute.ts";
import useSampler from "../../hooks/webgl/program/sampler/useSampler.ts";
import HTMLTextureFormat from "../../hooks/webgl/program/sampler/HTMLTextureFormat.ts";
import useResolutionUniform from "../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import useUniform, {vec} from "../../hooks/webgl/program/uniform/useUniform.ts";
import GLWrapMode from "../../hooks/webgl/program/sampler/GLWrapMode.ts";
import {Image, X} from "react-feather";
import ExtendedDownloadButton from "../shared/ExtendedDownload.tsx";
import {Input, Slider, Tooltip} from "@nextui-org/react";
import useImageTexture from "../../hooks/webgl/program/sampler/useImageTexture.ts";
import Compass from "../../assets/compass.png";
import {isArray} from "lodash";
import {useGenericStore} from "../../hooks/useGenericStore.ts";


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
  const [compassAngle, setCompassAngleRaw] = useGenericStore<number>("theta360CompassHeading");
  const [widthHeight, setWidthHeight] = useState([0, 0]);

  const compassImage = useImageTexture(Compass);
  const [textIsValid, setTextIsValid] = useState<boolean>(true);
  const [textCompassAngle, setTextCompassAngleRaw] = useState<string>(compassAngle.toString());

  // Used when changing via input text box
  const setTextCompassAngle = useCallback((text: string) => {
    setTextCompassAngleRaw(text)

    // Case for when empty input provided
    if (text.length === 0) {
      setTextIsValid(true);
      return;
    }

    // Parse the input as a float
    const number = parseFloat(text);

    // Prevent non-float inputs
    if (isNaN(number)) {
      setTextIsValid(false);
      return;
    }

    // Apply changes
    setCompassAngleRaw(number);
    setTextIsValid(true);
  }, []);

  // Used when changing via slider
  const setCompassAngle = useCallback((rawValue: number | number[]) => {
    const value = isArray(rawValue) ? rawValue[0] : rawValue;

    setCompassAngleRaw(value);
    setTextCompassAngleRaw(value.toFixed(2));
    setTextIsValid(true);
  }, [])

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

    //const bounds = gl.canvasRef.current?.getBoundingClientRect() ?? {width: 1, height: 1};
    const maxResolutionComp = Math.max(bounds.width, bounds.height);

    setMousePos(([x, y]) => [
      x + 2 * Math.PI * event.movementX / maxResolutionComp,
      y + 2 * Math.PI * event.movementY / maxResolutionComp
    ]);
    
  }, [gl.canvasRef]);

  const [mousePoint, setMousePoint] = useState<[number, number]>([0, 0]);
  // Function that converts y values from 0 to 1 into an angle relative to the midpoint of the image
  const yToTheta = useCallback((v: number) => -360* widthHeight[1]/widthHeight[0] * v + 180* widthHeight[1]/widthHeight[0] , [widthHeight]);

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

  // Create program to project and render image
  const compassProgram = useProgram(gl, CompassVert, CompassFrag);
  useSampler(compassProgram, 1, "compass", compassImage, {
    format: HTMLTextureFormat.ALPHA
  });
  useResolutionUniform(gl, compassProgram);
  useUniform(compassProgram, "mousePos", () => mousePos as vec, [mousePos]);
  useUniform(compassProgram, "compassAngle", () => (
    (isArray(compassAngle) ? [compassAngle[0] * Math.PI / 180] : [compassAngle * Math.PI / 180]) as vec
  ), [compassAngle]);

  // Called for the screenshot button, to fetch data to put in the file to save
  const getCanvasScreenshot = useCallback(() => {
    if (!gl.canvasRef.current)
      return [];

    const canvas = gl.canvasRef.current;
    gl.render(true);
    const dataURL = canvas.toDataURL("image/png");

    return convertToBlob(dataURL);
  }, [gl]);

  return (
    <div className="flex flex-col gap-2.5 flex-grow">
      ({mousePoint[0].toFixed(3)}, {mousePoint[1].toFixed(3)}) -{'>'} {yToTheta(mousePoint[1]).toFixed(2)} deg
      <AutosizedGLCanvas
        gl={gl}
        className="rounded p-3 flex-grow"
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
      <div className="flex flex-row gap-3 items-center">
        <Input placeholder="Heading" className="w-48" onValueChange={setTextCompassAngle} isInvalid={!textIsValid} value={textCompassAngle}></Input>
        <Slider value={compassAngle} onChange={setCompassAngle} size="lg" maxValue={360} minValue={0}
                step={0.01} className="flex-grow"></Slider>
      </div>
    </div>
  );
}

export default Perspective360CamCanvas;

