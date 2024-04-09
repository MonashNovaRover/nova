import {useCallback, useEffect, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import Vert from "./test.vert";
import Frag from "./test.frag";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import useCanvasSize from "../../../hooks/webgl/gl/useCanvasSize.ts";
import {useProgram} from "../../../hooks/webgl/program/useProgram.ts";
import useWebcam from "../../../hooks/webgl/program/sampler/useWebcam.ts";
import useSampler from "../../../hooks/webgl/program/sampler/useSampler.ts";
import useAttribute from "../../../hooks/webgl/program/attribute/useAttribute.ts";

export default function TestWebGLView() {
  const fluentHooks = {
    usePrint: (...data: unknown[]) => {
      useEffect(() => {
        console.log(...data);
      }, [data]);

      return {
        usePrint: () => {
          useEffect(() => {
            console.log(...data);
          }, []);
        }
      }
    },
    useCounter: () : [number, ()=>void] => {
      const [count, setCount] = useState<number>(0);

      return [count, useCallback(() => setCount(i => i + 1), [setCount])];
    }
  }

  const [count, increment] = fluentHooks.useCounter();

  // fluentHooks.usePrint("current count:", count).usePrint();

  const button = <Button onPress={increment}>Increment Count</Button>;

  const gl = useGL();
  const resolution = useCanvasSize(gl);

  const program = useProgram(gl, Vert, Frag);

  //const image = useImageTexture(ImageSRC);
  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef);
  useSampler(program, 0, "image", videoRef.current);


  useAttribute(program, "aTexCoord", () => [
    [1, 1], [0, 1], [1, 0], [0, 0]
  ], [count]);

  useAttribute(program, "aPosition", [[1, 1], [-1, 1], [1, -1], [-1, -1]]);



  // useUniform(program, "offset", useMemo(() => [count / 10, 0], [count]));

  return (
    <>
      <div
        className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-6 overflow-clip max-h-screen">
        <p>{count}</p>
        <div className="relative">
          <div className="absolute top-0 left-0 right-0 bottom-0">
            <canvas width={resolution[0]} height={resolution[1]} ref={gl.canvasRef}></canvas>
          </div>
        </div>
        <video ref={videoRef}></video>
        {button}

      </div>
    </>
  )
}
