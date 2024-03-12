import useGL from "../WebGLCanvas/hooks/useGL.tsx";
import {useProgram} from "../WebGLCanvas/hooks/useProgram.tsx";
import vert from "./gl/threshold.vert";
import frag from "./gl/threshold.frag";
import {useEffect} from "react";

const MicroscopeThresholdWidget = () => {

  const gl = useGL();
  const program = useProgram(gl.gl, vert, frag);

  useEffect(() => {

  }, []);



  return (

    <canvas ref={gl.canvasRef}></canvas>
  )
}

export default MicroscopeThresholdWidget;