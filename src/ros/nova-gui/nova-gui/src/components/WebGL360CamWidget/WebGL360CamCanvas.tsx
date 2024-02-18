import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"
import vert from "./gl/perspective.vert";
import frag from "./gl/perspective.frag";
import panoramaFrag from "./gl/panorama.frag";
import panoramaVert from "./gl/panorama.vert";

import React, {memo, useCallback, useState} from "react";
import useDict from "../WebGLCanvas/hooks/useDict.tsx";
import {vec2} from "../WebGLCanvas/hooks/useUniforms.tsx";
import {GLSampler} from "../WebGLCanvas/hooks/useSamplers.tsx";

import EquirectangularTestImage from "../../assets/equirectangular.png";

import useImageTexture from "../WebGLCanvas/hooks/useImageTexture.ts";




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

  // Mouse position attribute
  const [mousePos, setMousePos] = useState<vec2>([0, 0]);
  const onMouseMove = useCallback((event: MouseEvent) => {
    if (event.buttons === 1)
      setMousePos([mousePos[0] + event.movementX, mousePos[1] + event.movementY]);
  }, [mousePos]);

  // Panorama mode switch
  const [usePanorama, setUsePanorama] = useState<boolean>(false);
  const onMouseDown = useCallback((event: MouseEvent) => {
    if (event.buttons === 2) {
      setUsePanorama(!usePanorama);
      console.log("Panorama: ", !usePanorama);
    }
  }, [usePanorama]);

  const [fov, setFov] = useState(90);
  const onWheel = useCallback((e: WheelEvent) => {
    const newFov = fov + e.deltaY / 50;
    setFov(Math.max(Math.min(newFov, 179), 0.01));
  }, [fov]);

  // We define uniforms that change with changes to the camera properties
  const uniforms = useDict(() => ({
    mousePos: mousePos,
    fov: [fov]
  }), [mousePos, fov]);

  const samplers = useDict<GLSampler>(() => ({
    camera: videoRef?.current ?? equirectangularTest
  }), [videoRef?.current, equirectangularTest]);



  // Construct the canvas
  return (
    <WebGLCanvas

      autoSize
      className={props.className}

      // Defines the vertex and fragment shaders. Shader programs are auto-compiled by the component on change.
      vert={usePanorama ? panoramaVert : vert}   // Defines the vertex shader.
      frag={usePanorama ? panoramaFrag : frag}   // Defines the fragment shader.

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
      onMouseDown={onMouseDown}
    />
  ); // <video ref={webcam}/> // <video width={1920} height={1440} ref={webcam}/>
}


const WebGL360CamCanvas = memo(WebGL360CamCanvasNonMemo);
export default WebGL360CamCanvas;