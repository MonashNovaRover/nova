import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"
import Vert from "./vert.ts" ;
import Frag from "./frag.ts";
import React, {memo, useEffect, useRef, useState} from "react";
import {loadImageFromURL} from "../WebGLCanvas/loadTexture.ts";
import {CameraComponentProps} from "../CameraComponent/CameraComponent.tsx";
import {useCameraStream} from "../CameraComponent/hooks/useCameraStream.ts";

const TICK_INTERVAL_MS = 6.94444444444;


const WebGL360CamWidgetNonMemo: React.FC<CameraComponentProps> = (props: CameraComponentProps) => {





  // Example for sampler: you could use cameras 2
  /*
  const { cameraSerial } = props;
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const { streamingState, sendSessionStartMessage, isCameraOnline } =
    useCameraStream(cameraSerial, videoRef);
  */



  // To show uniforms working, increment a time value and pass it as a uniform
  const [time, setTime] = useState(0);
  useEffect(() => {
    setTimeout(() => {
      setTime(time + 0.001 * TICK_INTERVAL_MS);
    }, TICK_INTERVAL_MS)
  });

  // Construct the canvas
  return <WebGLCanvas width={800} height={600}

    // Defines the vertex and fragment shaders. Shader programs are auto-compiled by the component on change.
    vert={Vert}   // Defines the vertex shader.
    frag={Frag}   // Defines the fragment shader.

    // Defines the background color
    clearColor={[0.0, 0.0, 0.0, 0.0]}

    // Defines values for vertex attributes, where the key should match the name of the attribute in the vertex shader.
    // This might look like: `in (vec[234]|float) <name>` in the vertex shader. e.g. `in vec4 position`.
    vertexAttributes={{
      a_position: {
        numComponents: 2,
        data: [[1.0, 1.0], [-1.0, 1.0], [1.0, -1.0], [-1.0, -1.0]].flat()
      },
      a_texCoord: {
        numComponents: 2,
        data: [ 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0]
      }
    }}

    // The number of vertices to render when calling `gl.drawArrays`
    vertexCount={4}

    // Defines float for float vector values for uniforms, where the key should match the name of the uniform in the
    // vertex or fragment shader. This might look like: `uniform (vec[234]|float) <name>` in the shader.
    uniforms={{
      time: [time]
    }}

    // Defines samplers for the fragment shader (textures). These are passed as a `HTMLImageElement` or
    // `HTMLVideoElement`, which is automatically converted into a texture.
    //
    // samplers={{
    //   test_image: loadImageFromURL("../../assets/rover-top-down-dark.png"),
    //   "360cam": videoRef.current
    // }}
  />;
}


const WebGL360CamWidget = memo(WebGL360CamWidgetNonMemo);
export default WebGL360CamWidget;