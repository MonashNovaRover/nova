import {useCallback, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import Vert from "./test.vert";
import Frag from "./test.frag";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import {useProgram} from "../../../hooks/webgl/program/useProgram.ts";
import useSampler from "../../../hooks/webgl/program/sampler/useSampler.ts";
import useAttribute from "../../../hooks/webgl/program/attribute/useAttribute.ts";
import useUniform from "../../../hooks/webgl/program/uniform/useUniform.ts";
import useWebcam from "../../../hooks/webgl/program/sampler/useWebcam.ts";

export default function TestWebGLView() {
  const [count, setCount] = useState<number>(0)
  const increment = useCallback(() => {
    setCount(i => i + 1);
  }, [setCount]);

  const gl = useGL();

  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef)

  //const resolution = useCanvasSize(gl, videoRef.current ?? undefined);

  const program = useProgram(gl, Vert, Frag);

  useSampler(program, 0, "image", videoRef.current);

  useAttribute(program, "aTexCoord", [
    [1, 1], [0, 1], [1, 0], [0, 0]
  ]);

  useAttribute(program, "aPosition", [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ]);

  useUniform(program, "offset", () => [
    count / 10,
    0
  ], [count]);

  return (
    <>
      <div
        className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-6 overflow-clip max-h-screen">
        <p>{count}</p>
        <div className="relative">
          <div className="absolute top-0 left-0 right-0 bottom-0">
            <canvas width={600} height={350} ref={gl.canvasRef}></canvas>
          </div>
        </div>
        <video ref={videoRef}></video>
        <Button onPress={increment}>Increment Count</Button>
      </div>
    </>
  )
}
