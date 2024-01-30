import WebGLCanvas from "../WebGLCanvas/WebGLCanvas"
import Vert from "./vert.txt";
import Frag from "./frag.txt";



const WebGL360CamWidget: React.FC = () => {

  return (
    <WebGLCanvas
      vert={Vert}
      frag={Frag}
    />
  );
}


export default WebGL360CamWidget;