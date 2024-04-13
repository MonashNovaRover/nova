import {useCallback, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import Vert from "./test.vert";
import Frag from "./test.frag";
import LineVert from "./line.vert";
import LineFrag from "./line.frag";
import OverlayFrag from "./overlay.frag";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import {useProgram} from "../../../hooks/webgl/program/useProgram.ts";
import useSampler from "../../../hooks/webgl/program/sampler/useSampler.ts";
import useAttribute from "../../../hooks/webgl/program/attribute/useAttribute.ts";
import useWebcam from "../../../hooks/webgl/program/sampler/useWebcam.ts";
import useResolutionUniform from "../../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import AutosizedGLCanvas from "../../../components/AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import GLProgramDrawMode from "../../../hooks/webgl/program/GLProgramDrawMode.ts";
import useImageTexture from "../../../hooks/webgl/program/sampler/useImageTexture.ts";
import ImageSRC from "../../../assets/arm-image.png";
import useAnimationFrame from "../../../hooks/webgl/gl/useAnimationFrame.ts";
import useProgramRenderEffect from "../../../hooks/webgl/program/useProgramRenderEffect.ts";

export default function TestWebGLView() {
  const [count, setCount] = useState<number>(0)
  const increment = useCallback(() => {
    setCount(i => i + 1);
  }, [setCount]);


  const [time, setTime] = useState<number>(0);
  useAnimationFrame(setTime);

  const gl = useGL();

  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef)

  const image = useImageTexture(ImageSRC);

  const program = useProgram(gl, Vert, Frag);



  useResolutionUniform(gl, program);
  useSampler(program, 0, "image", videoRef.current);
  useAttribute(program, "aPosition", () => [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ], []);

  // Overlay an image
  const overlayProgram = useProgram(gl, Vert, OverlayFrag);
  useSampler(overlayProgram, 1, "image", image);
  useAttribute(overlayProgram, "aPosition", () => [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ], []);

  // Draw lines ontop of everything
  const lineProgram = useProgram(gl, LineVert, LineFrag, {
    drawMode: GLProgramDrawMode.LINE_LOOP,
    numberOfVertices: 4
  });
  useAttribute(lineProgram, "aLinePosition", () => [
    [-Math.cos(time), -Math.sin(time)], [-0.5, Math.sin(0.5 * time -3.14159/4)], [0.5, Math.sin(0.5 * time + 3.14159/4)], [Math.cos(1.87654321 * time), Math.sin(1.87654321 * time)],
  ], [time]);
  useProgramRenderEffect(lineProgram, (context) => {
    context.lineWidth(2.5);
  }, [])

  return (
    <>
      <div
        className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-6 overflow-clip max-h-screen">
        <p>{count}</p>
        <AutosizedGLCanvas gl={gl} drawChildrenBelow={false}>
          <p>Hello</p>
        </AutosizedGLCanvas>
        <video ref={videoRef}></video>
        <Button onPress={increment}>Increment Count</Button>
      </div>
    </>
  )
}
