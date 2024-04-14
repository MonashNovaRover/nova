import {useCallback, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import Vert from "./test.vert";
import Frag from "./test.frag";
import LineVert from "./line.vert";
import LineFrag from "./line.frag";
import OverlayVert from "./overlay.vert";
import OverlayFrag from "./overlay.frag";
import BezFrag from "./bez.frag";
import BezVert from "./bez.vert";
import IdentityVert from "./identity.vert";
import IdentityFrag from "./identity.frag";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../../hooks/webgl/program/useProgram.ts";
import useSampler from "../../../hooks/webgl/program/sampler/useSampler.ts";
import useAttribute from "../../../hooks/webgl/program/attribute/useAttribute.ts";
import useWebcam from "../../../hooks/webgl/program/sampler/useWebcam.ts";
import useResolutionUniform from "../../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import AutosizedGLCanvas from "../../../components/AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import GLProgramDrawMode from "../../../hooks/webgl/program/GLProgramDrawMode.ts";
import useImageTexture from "../../../hooks/webgl/program/sampler/useImageTexture.ts";
import ImageSRC from "../../../assets/arm-image.png";
import SecondImageSRC from "../../../assets/rover-top-down-dark.png";
import useAnimationFrame from "../../../hooks/webgl/gl/useAnimationFrame.ts";
import useProgramRenderEffect from "../../../hooks/webgl/program/useProgramRenderEffect.ts";
import useUniform from "../../../hooks/webgl/program/uniform/useUniform.ts";
import GLTextureWrapMode from "../../../hooks/webgl/program/sampler/GLTextureWrapMode.ts";
import HTMLTextureFormat from "../../../hooks/webgl/program/sampler/HTMLTextureFormat.ts";

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
  const secondImage = useImageTexture(SecondImageSRC);

  // Main Program
  const program = useProgram(gl, Vert, Frag);
  useResolutionUniform(gl, program);
  useSampler(program, 0, "spec", videoRef.current, {
    wrapS: GLTextureWrapMode.REPEAT,
    wrapT: GLTextureWrapMode.CLAMP_TO_EDGE
  });
  useAttribute(program, "aPosition", () => [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ], []);

  // Bezier
  const bezLineProgram = useProgram(gl, Vert, LineFrag, {
    drawMode: GLProgramDrawMode.LINE_STRIP,
    vertexCount: 3,
  })
  useAttribute(bezLineProgram, "aPosition", () => [
    [0.2, 0.2], [0.8, 0.5], [0.4, 0.8]
  ], []);
  const bezProgram = useProgram(gl, BezVert, BezFrag, {
    drawMode: GLProgramDrawMode.TRIANGLES,
    vertexCount: 3
  });
  useAttribute(bezProgram, "aPosition", () => [
    [0.2, 0.2], [0.8, 0.5], [0.4, 0.8]
  ], []);

  useAttribute(bezProgram, "index", () => [
    [1,0,0], [0.5,1,0.5], [0,0,1]
  ], []);

  // Overlay an image
  const overlayImage = count % 2 === 0 ? image : secondImage;
  const overlayProgram = useProgram(gl, OverlayVert, OverlayFrag);
  // useSampler(overlayProgram, 1, "image", image);
  useSampler(overlayProgram, 0, "image", overlayImage, {
    wrapT: GLTextureWrapMode.MIRRORED_REPEAT,
    wrapS: GLTextureWrapMode.MIRRORED_REPEAT
  });
  useAttribute(overlayProgram, "aPosition", () => [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ], []);
  useResolutionUniform(gl, overlayProgram, "imageResolution", overlayImage)
  useResolutionUniform(gl, overlayProgram, "resolution");
  useUniform(overlayProgram, "time", () => [time], [time])

  // Draw lines on top of everything
  const lineProgram = useProgram(gl, LineVert, LineFrag, {
    drawMode: GLProgramDrawMode.LINE_LOOP,
    vertexCount: 4
  });
  useAttribute(lineProgram, "aLinePosition", () => [
    [-Math.cos(time), -Math.sin(time)], [-0.5, Math.sin(0.5 * time -3.14159/4)], [0.5, Math.sin(0.5 * time + 3.14159/4)], [Math.cos(1.87654321 * time), Math.sin(1.87654321 * time)],
  ], [time]);
  useProgramRenderEffect(lineProgram, (context) => {
    // This only works on chromium!
    context.lineWidth(2.5);
  }, [])

  const secondVideoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(secondVideoRef)

  // Add the "mirrored" webcam with another GL!
  const secondGL = useGL();
  const mirrorWebcamProgram = useProgram(secondGL, IdentityVert, IdentityFrag);
  useSampler(mirrorWebcamProgram, 0, "image", videoRef.current, {
    wrapT: GLTextureWrapMode.CLAMP_TO_EDGE,
    wrapS: GLTextureWrapMode.MIRRORED_REPEAT,
    format: count % 2 === 0 ? HTMLTextureFormat.RGB : HTMLTextureFormat.LUMINANCE,
  })
  useAttribute(mirrorWebcamProgram, "aPosition", () => [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ], []);
  useUniform(mirrorWebcamProgram, "count", () => [count], [count])

  return (
    <div
      className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-5 overflow-clip max-h-screen">
      <p>{count}</p>
      <AutosizedGLCanvas gl={gl} drawChildrenBelow={false} className="overflow-hidden resize">
        <div className={`relative block h-full`} style={{
          left: `${(50 - 50*Math.cos(time))}%`,
        }}><p>Count: {count}</p></div>
      </AutosizedGLCanvas>
      <video ref={videoRef}></video>
      <AutosizedGLCanvas gl={secondGL} sizeTarget={videoRef.current}>
        <i>Mirrored with WebGL hooks!</i>
      </AutosizedGLCanvas>
      <video ref={secondVideoRef}></video>
      <Button onPress={increment}>Increment Count</Button>
    </div>
  )
}