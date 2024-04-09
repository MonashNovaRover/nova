import {useCallback, useEffect, useMemo, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import useGL from "../../../components/WebGLCanvas/hooks/gl/useGL.ts";
import useCanvasSize from "../../../components/WebGLCanvas/hooks/gl/useCanvasSize.ts";
import {useProgram} from "../../../components/WebGLCanvas/hooks/program/useProgram.ts";
import {IVertexAttribute} from "../../../components/WebGLCanvas/hooks/program/attribute/useAttributes.ts";
import Vert from "./test.vert";
import Frag from "./test.frag";
import useSampler from "../../../components/WebGLCanvas/hooks/program/sampler/useSampler.ts";
import useAttribute from "../../../components/WebGLCanvas/hooks/program/attribute/useAttribute.ts";
import useWebcam from "../../../components/WebGLCanvas/hooks/program/sampler/useWebcam.ts";

const aPositionAttribute = {
  numComponents: 2,
  data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
} as IVertexAttribute

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
          }, [data]);
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


  const aTexCoord = useMemo(() => ({
    numComponents: 2,
    data: [1.0 + count/10, 1.0, count/10, 1.0, 1.0 + count/10, 0.0, count/10, 0.0]
  }), [count])
  useAttribute(program, "aTexCoord", aTexCoord);

  useAttribute(program, "aPosition", aPositionAttribute);



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
