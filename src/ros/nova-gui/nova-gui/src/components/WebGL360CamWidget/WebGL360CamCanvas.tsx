import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"

import thresholdFrag from "../MicroscopeThresholdWidget/gl/threshold.frag";
import thresholdVert from "../MicroscopeThresholdWidget/gl/threshold.vert";

import React, {memo, MouseEventHandler, MouseEvent, WheelEvent, useCallback, useState} from "react";
import useDict from "../WebGLCanvas/hooks/useDict.tsx";
import { vec, vec2} from "../WebGLCanvas/hooks/useUniforms.tsx";
import {GLSampler} from "../WebGLCanvas/hooks/useSamplers.tsx";

import EquirectangularTestImage from "../../assets/equirectangular.png";
import CompassImage from "../../assets/compass.png";

import useImageTexture from "../WebGLCanvas/hooks/useImageTexture.ts";
import useGL from "../WebGLCanvas/hooks/useGL.tsx";
// import useCanvasSize from "../WebGLCanvas/hooks/useCanvasSize.tsx";
import {Button} from "@nextui-org/react";

const DEG_TO_RAD = 0.0174532925199;

// The attributes needed to be defined outside of the function, otherwise the useEffect in useAttributes in WebGLCanvas
// would be called whenever the uniforms changed. This implies that each time the function is called it generated a new
// set of attributes?
const attributes = {
  aPosition: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  }
};

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


export interface WebGL360CamCanvasProps {
  className?: string,
  videoRef?: React.MutableRefObject<HTMLVideoElement | null>
}

const WebGL360CamCanvasNonMemo = (props: WebGL360CamCanvasProps) => {
  const videoRef = props.videoRef;
  const equirectangularTest = useImageTexture(EquirectangularTestImage);
  const compass = useImageTexture(CompassImage);

  const gl = useGL();
  // const [width, height] = useCanvasSize(gl);
  const [width, height] = [1200, 700];
  const resolution = [width, height];

  // Panorama mode switch
  const [usePanorama, setUsePanorama] = useState<boolean>(true);
  const [fov, setFov] = useState(90);

  // Mouse position attribute
  const [mousePos, setMousePos] = useState<vec2>([0, 0]);
  const onMouseMove = useCallback((event: MouseEvent<HTMLCanvasElement>) => {
    if (event.buttons === 1) {
      const maxResolutionComp = Math.max(width, height) / window.devicePixelRatio;

      setMousePos([
        mousePos[0] + fov * DEG_TO_RAD * event.movementX / maxResolutionComp,
        mousePos[1] + fov * DEG_TO_RAD * event.movementY / maxResolutionComp
      ]);
    }

  }, [mousePos, fov, width, height]);

  const onMouseDown = useCallback((event: MouseEvent<HTMLCanvasElement>) => {
    if (event.buttons === 2)
      setUsePanorama(!usePanorama);
  }, [usePanorama]);

  const onWheel = useCallback((e: WheelEvent<HTMLCanvasElement>) => {
    const newFov = fov + e.deltaY / 50;
    setFov(Math.max(Math.min(newFov, usePanorama ? 360 : 179), 0.01));
  }, [fov, usePanorama]);

  // We define uniforms that change with changes to the camera properties
  const uniforms = useDict<vec>(() => ({
    mousePos: mousePos,
    fov: [fov],
    resolution: resolution as vec2,
    threshold: [fov/180]
  }), [mousePos, fov, resolution]);

  const samplers = useDict<GLSampler>(() => ({
    image: [videoRef?.current ?? equirectangularTest, 0],
  }), [videoRef?.current, equirectangularTest, compass]);

  // Construct the canvas
  return (<div className="w-full h-50 min-h-unit-24 min-w-unit-24 resize-y overflow-hidden mb-5">
      <Button onClick={() => {
        const numElements = width * height;

        if (!gl.gl)
          return;

        // Redraw
        gl.gl.clear(gl.gl.COLOR_BUFFER_BIT);
        // TODO: allow the mode to be specified, rather than being hard coded as `gl.TRIANGLE_STRIP`
        gl.gl.drawArrays(gl.gl.TRIANGLE_STRIP, 0, 4);

        const output = new Uint8Array(numElements * 4);
        gl.gl?.readPixels(0, 0, width, height, gl.gl.RGBA, gl.gl.UNSIGNED_BYTE, output);

        const average = output.reduce((acc, value, index) => (index % 4) === 3 ? acc : acc + (value/(numElements * 3)), 0) / 255;

        console.log(output);
        console.log(`${(100 * (1 - average)).toFixed(4)}%`);
      }}>
        biuggiugersubegrsub
      </Button>

      <WebGLCanvas
      gl={gl}

      className={props.className}

      resolution={resolution as vec2}

      // Defines the vertex and fragment shaders. Shader programs are auto-compiled by the component on change.
      vert={thresholdVert}   // Defines the vertex shader.
      frag={thresholdFrag}   // Defines the fragment shader.

      // Defines the background color drawn first on each render, before `gl.drawArrays`
      clearColor={[0.0, 0.0, 0.0, 0.0]}

      // Defines values for vertex attributes, where the key should match the name of the attribute in the vertex shader.
      // This might look like: `in (vec[234]|float) <name>` in the vertex shader. e.g. `in vec4 position`.
      attributes={attributes}

      // The number of vertices to render when calling `gl.drawArrays`
      vertexCount={4}

      // Defines float for float vector values for uniforms, where the key should match the name of the uniform in the
      // vertex or fragment shader. This might look like: `uniform (vec[234]|float) <name>` in the shader.
      uniforms={uniforms}

      // Defines samplers for the fragment shader (textures). These are passed as a `HTMLImageElement` or
      // `HTMLVideoElement`, which is automatically converted into a texture.
      samplers={samplers}

      onMouseMove={onMouseMove}
      onWheel={onWheel}
      onMouseEnter={disableScroll}
      onMouseLeave={enableScroll}
      onMouseDown={onMouseDown as MouseEventHandler}

    />

    </div>

  ); // <video ref={webcam}/> // <video width={1920} height={1440} ref={webcam}/>
}


const WebGL360CamCanvas = memo(WebGL360CamCanvasNonMemo);
export default WebGL360CamCanvas;