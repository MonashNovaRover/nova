import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"
import Vert from "./vert.ts" ;
import Frag from "./frag.ts";
import React, {memo, useEffect, useState} from "react";



const WebGL360CamWidgetNonMemo: React.FC = () => {

  const [time, setTime] = useState(0);
  const fps = 144;

  useEffect(() => {
    setTimeout(() => {
      setTime(time + 1/fps);
    }, 1000 / fps);
  });

  return (
    <WebGLCanvas
      vert={Vert}
      frag={Frag}
      width={800}
      height={600}
      clearColor={[0.0, 0.0, 0.0, 0.0]}

      vertexAttributes={{
        a_position: {
          numComponents: 2,
          data: [[1.0, 1.0], [-1.0, 1.0], [1.0, -1.0], [-1.0, -1.0]].flat()
        }
      }}
      vertexCount={4}

      uniforms={{
        time: [time]
      }}
    />

  );
}


const WebGL360CamWidget = memo(WebGL360CamWidgetNonMemo);
export default WebGL360CamWidget;