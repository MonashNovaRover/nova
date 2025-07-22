import {useCallback, useRef, useState} from "react";
import {Button} from "@nextui-org/react";
import Vert from "./gl/test.vert";
import Frag from "./gl/test.frag";
import LineVert from "./gl/line.vert";
import LineFrag from "./gl/line.frag";
import OverlayVert from "./gl/overlay.vert";
import OverlayFrag from "./gl/overlay.frag";
import BezFrag from "./gl/bez.frag";
import BezVert from "./gl/bez.vert";
import IdentityVert from "./gl/identity.vert";
import IdentityFrag from "./gl/identity.frag";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../../hooks/webgl/program/useProgram.ts";
import useSampler from "../../../hooks/webgl/program/sampler/useSampler.ts";
import useAttribute from "../../../hooks/webgl/program/attribute/useAttribute.ts";
import useWebcam from "../../../hooks/webgl/program/sampler/useWebcam.ts";
import useResolutionUniform from "../../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import AutosizedGLCanvas from "../../../components/shared/components/AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import GLProgramDrawMode from "../../../hooks/webgl/program/GLProgramDrawMode.ts";
import useImageTexture from "../../../hooks/webgl/program/sampler/useImageTexture.ts";
import ImageSRC from "../../../assets/arm-image.png";
import SecondImageSRC from "../../../assets/rover-top-down-dark.png";
import MonkeysImageSRC from "../../../assets/equirectangular.png";
import NovaImageSRC from "../../../assets/nova-logo.png";
import useProgramRenderEffect from "../../../hooks/webgl/program/useProgramRenderEffect.ts";
import useUniform from "../../../hooks/webgl/program/uniform/useUniform.ts";
import GLWrapMode from "../../../hooks/webgl/program/sampler/GLWrapMode.ts";
import HTMLTextureFormat from "../../../hooks/webgl/program/sampler/HTMLTextureFormat.ts";
import useScreenQuadAttribute from "../../../hooks/webgl/program/attribute/useScreenQuadAttribute.ts";
import useTimeUniform from "../../../hooks/webgl/program/uniform/useTimeUniform.ts";
import useTimeAttribute from "../../../hooks/webgl/program/attribute/useTimeAttribute.ts";

/**
 * A webgl hooks stress test, that ensures that some everything is working smoothly.
 * @constructor
 */
export default function TestWebGLView() {
  const [count, setCount] = useState<number>(0)
  const increment = useCallback(() => {
    setCount(i => i + 1);
  }, [setCount]);

  const gl = useGL();

  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef)
  const image = useImageTexture(ImageSRC);
  const secondImage = useImageTexture(SecondImageSRC);

  // Main Program
  const program = useProgram(gl, Vert, Frag);
  useResolutionUniform(gl, program);
  useSampler(program, 0, "spec", videoRef.current, {
    wrapS: GLWrapMode.REPEAT,
    wrapT: GLWrapMode.CLAMP_TO_EDGE
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
  useSampler(overlayProgram, 0, "image", overlayImage, {
    wrapT: GLWrapMode.MIRRORED_REPEAT,
    wrapS: GLWrapMode.MIRRORED_REPEAT
  });
  useAttribute(overlayProgram, "aPosition", () => [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ], []);
  useResolutionUniform(gl, overlayProgram, "imageResolution", overlayImage)
  useResolutionUniform(gl, overlayProgram, "resolution");
  useTimeUniform(overlayProgram);

  // Draw lines on top of everything
  const lineProgram = useProgram(gl, LineVert, LineFrag, {
    drawMode: GLProgramDrawMode.LINE_LOOP,
    vertexCount: 4
  });
  useTimeAttribute(lineProgram, "aLinePosition", (milliseconds) => {
    const time = milliseconds / 1000;
    return [
      [-Math.cos(time), -Math.sin(time)], [-0.5, Math.sin(0.5 * time -3.14159/4)],
      [0.5, Math.sin(0.5 * time + 3.14159/4)], [Math.cos(1.87654321 * time), Math.sin(1.87654321 * time)],
    ];
  });

  useProgramRenderEffect(lineProgram, (context) => {
    // This only works on chromium!
    context.lineWidth(2.5);
  }, [])

  // Add the "mirrored" webcam with another GL!
  const secondGL = useGL();
  const mirrorWebcamProgram = useProgram(secondGL, IdentityVert, IdentityFrag);
  useSampler(mirrorWebcamProgram, 0, "image", videoRef.current, {
    wrapS: GLWrapMode.MIRRORED_REPEAT,
    wrapT: GLWrapMode.CLAMP_TO_EDGE,
    format: count % 2 === 0 ? HTMLTextureFormat.RGB : HTMLTextureFormat.LUMINANCE,
  })
  useScreenQuadAttribute(mirrorWebcamProgram);
  useUniform(mirrorWebcamProgram, "count", () => [count], [count])

  // Static image test
  const monkeysImage = useImageTexture(MonkeysImageSRC);
  const novaImage = useImageTexture(NovaImageSRC);
  const imageGLImage = [monkeysImage, novaImage, secondImage, image][count % 4];

  const imageGL = useGL();
  const imageProgram = useProgram(imageGL, IdentityVert, IdentityFrag);
  useSampler(imageProgram, 0, "image", imageGLImage, {
    wrapS: GLWrapMode.MIRRORED_REPEAT,
    format: count % 8 < 4 ? HTMLTextureFormat.RGBA : HTMLTextureFormat.LUMINANCE_ALPHA
  });
  useScreenQuadAttribute(imageProgram);
  useUniform(imageProgram, "count", [10])

  return (<div className="h-screen">
    <div className="grid w-full gap-3 p-3 auto-cols-fr max-h-full grid-cols-3 overflow-clip pb-48">
      <video ref={videoRef} className="rounded w-full"></video>

      <AutosizedGLCanvas gl={secondGL} sizeTarget={videoRef.current} className="rounded">
        <i>Mirrored with WebGL hooks!</i>
      </AutosizedGLCanvas>

      <div className="flex flex-col gap-3 align-middle justify-center items-center p-3">
        <p>{count}</p>
        <Button onPress={increment} color={count % 2 === 0 ? "primary" : "default"} size="lg">
          Increment Count
        </Button>
      </div>

      <AutosizedGLCanvas gl={gl} drawChildrenBelow={false}
                         className="col-span-2 resize max-h-full max-w-full min-h-6 min-w-24 rounded">
        <div className={`relative block h-full`} style={{
          left: `${(50 - 50 * Math.cos(5))}%`,
        }}><p>Count: {count}</p></div>
      </AutosizedGLCanvas>

      <AutosizedGLCanvas gl={imageGL} className="min-h-64 rounded"></AutosizedGLCanvas>
    </div>
  </div>)
}