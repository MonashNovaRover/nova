import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"
import Vert from "./vert.ts" ;
import Frag from "./frag.ts";



const WebGL360CamWidget: React.FC<C> = () => {

  return (
    <WebGLCanvas
      vert={Vert}
      frag={Frag}
      width={800}
      height={600}
      positions={[1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -0.0]}
      vertexAttributes={{a_positions: [[1.0, 1.0], [-1.0, 1.0], [1.0, -1.0], [-1.0, -0.0]]}}
    />
  );
}


export default WebGL360CamWidget;