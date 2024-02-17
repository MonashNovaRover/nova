import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"
import Vert from "./vert.ts" ;
import Frag from "./frag.ts";
import React, {memo, useState} from "react";
import useDict from "../WebGLCanvas/hooks/useDict.tsx";
import {GLUniforms} from "../WebGLCanvas/hooks/useUniforms.tsx";
import {GLSampler} from "../WebGLCanvas/hooks/useSamplers.tsx";

import RoverImage from "../../assets/rover-top-down-dark.png";
import NovaImage from "../../assets/nova-logo.png";
import useWebcam from "../WebGLCanvas/hooks/useWebcam.tsx";
import useAnimationFrame from "../WebGLCanvas/hooks/useAnimationFrame.ts";
import useImageTexture from "../WebGLCanvas/hooks/useImageTexture.ts";

// The attributes needed to be defined outside of the function, otherwise the useEffect in useAttributes in WebGLCanvas
// would be called whenever the uniforms changed. This implies that each time the function is called it generated a new
// set of attributes?
const attributes = {
  a_position: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  },
  a_texCoord: {
    numComponents: 2,
    data: [1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0]
  }
};

const WebGL360CamWidgetNonMemo: React.FC = () => {
  // To show uniforms working, increment a time value and pass it as a uniform
  const [time, setTime] = useState(0);
  useAnimationFrame(setTime);

  const webcam = useWebcam();
  const rover = useImageTexture(RoverImage);
  const logo = useImageTexture(NovaImage);

  // We define uniforms that change with the time variable.
  const uniforms = {
    time: [time]
  } as GLUniforms;

  const samplers = useDict<GLSampler>(() => ({
    rover: logo,
    webcam: webcam,
  }), [rover, webcam, logo]);

  // Construct the canvas
  return <><WebGLCanvas width={1200} height={800}

    // Defines the vertex and fragment shaders. Shader programs are auto-compiled by the component on change.
    vert={Vert}   // Defines the vertex shader.
    frag={Frag}   // Defines the fragment shader.

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
  />

  </>; //  <video ref={webcam}/>
}


const WebGL360CamWidget = memo(WebGL360CamWidgetNonMemo);
export default WebGL360CamWidget;