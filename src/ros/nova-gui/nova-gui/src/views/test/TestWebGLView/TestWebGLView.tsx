import {useCallback, useEffect, useMemo, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import useGL from "../../../components/WebGLCanvas/hooks/gl/useGL.tsx";
import useCanvasSize from "../../../components/WebGLCanvas/hooks/useCanvasSize.tsx";
import {useProgram} from "../../../components/WebGLCanvas/hooks/program/useProgram.tsx";
import useAttributes, {IVertexAttribute} from "../../../components/WebGLCanvas/hooks/useAttributes.tsx";
import Vert from "./test.vert";
import Frag from "./test.frag";
import useImageTexture from "../../../components/WebGLCanvas/hooks/useImageTexture.ts";
import ImageSRC from "../../../assets/nova-logo.png";
import useSampler from "../../../components/WebGLCanvas/hooks/useSampler.ts";
import useEffectQueue from "../../../components/WebGLCanvas/hooks/effectQueue/useEffectQueue.ts";
import useEffectQueueEffect from "../../../components/WebGLCanvas/hooks/effectQueue/useEffectQueueEffect.ts";
import {vec2} from "../../../components/WebGLCanvas/hooks/useUniforms.tsx";
import useAttribute from "../../../components/WebGLCanvas/hooks/program/useAttribute.ts";
import useWebcam from "../../../components/WebGLCanvas/hooks/useWebcam.tsx";

const attributes = {
  aPosition: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  }
}

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
  const webcam = useWebcam(videoRef);
  useSampler(program, 0, "image", videoRef.current);


  const aTexCoord = useMemo(() => ({
    numComponents: 2,
    data: [1.0 + count/10, 1.0, count/10, 1.0, 1.0 + count/10, 0.0, count/10, 0.0]
  }), [count])
  useAttribute(program, "aTexCoord", aTexCoord);

  useAttribute(program, "aPosition", aPositionAttribute);

  useEffect(() => {
    let frameID = 0;

    const callback = () => {

      if (gl.context)
        gl.renderQueue.render(gl.context);

      frameID = requestAnimationFrame(callback);
    }
    callback();

    return () => {
      cancelAnimationFrame(frameID)
    }
  }, [program]);

  // useUniform(program, "offset", useMemo(() => [count / 10, 0], [count]));



  return (
    <>
      <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-6 overflow-clip max-h-screen">
        <p>{count}</p>
        <div className="relative">
          <video ref={videoRef}></video>
          <div className="absolute top-0 left-0 right-0 bottom-0">
            <canvas width={resolution[0]} height={resolution[1]} ref={gl.canvasRef}></canvas>
          </div>
        </div>
        {button}

      </div>
    </>
  )
}
