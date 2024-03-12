import useGL from "../WebGLCanvas/hooks/useGL.tsx";
import {useProgram} from "../WebGLCanvas/hooks/useProgram.tsx";
import vert from "./gl/threshold.vert";
import frag from "./gl/threshold.frag";
import React, {useCallback, useEffect, useRef, useState} from "react";
import {Button, Card, CardBody, CardHeader, Slider} from "@nextui-org/react";
import useSamplers, {GLSampler} from "../WebGLCanvas/hooks/useSamplers.tsx";
import useDict from "../WebGLCanvas/hooks/useDict.tsx";
import useWebcam from "../WebGLCanvas/hooks/useWebcam.tsx";
import useAttributes from "../WebGLCanvas/hooks/useAttributes.tsx";
import useCanvasSize from "../WebGLCanvas/hooks/useCanvasSize.tsx";
import useUniforms, {vec} from "../WebGLCanvas/hooks/useUniforms.tsx";
import CopyableOutput from "../CopyableOutput/CopyableOutput.tsx";

const attributes = {
  aPosition: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  }
};

const MicroscopeThresholdWidget = () => {
  const [threshold, setThreshold] = useState<number>(0.5);
  const [brightness, setBrightness] = useState<number | undefined>();

  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef);

  const gl = useGL();
  const program = useProgram(gl.gl, vert, frag);

  const [width, height] = useCanvasSize(gl, videoRef.current ?? undefined);

  const samplers = useDict<GLSampler>(() => ({
    image: [videoRef.current, 0]
  }), [videoRef.current])
  const frameID = useSamplers(gl.gl, program, samplers);

  useAttributes(gl.gl, program, attributes);

  const uniforms = useDict<vec>(() => ({
    threshold: [1-threshold]
  }), [threshold])
  useUniforms(gl.gl, program, uniforms);

  // Render the canvas whenever anything relevant changes
  useEffect(() => {
    if (!gl.gl || !program)
      return;

    // Redraw
    gl.gl.clear(gl.gl.COLOR_BUFFER_BIT);
    gl.gl.drawArrays(gl.gl.TRIANGLE_STRIP, 0, 4);
  }, [gl.gl, program, videoRef.current?.width, videoRef.current?.height, attributes, samplers, uniforms, threshold, frameID]);

  const getBrightness = useCallback(() => {
    const numElements = width * height;

    if (!gl.gl)
      return;

    // Redraw
    gl.gl.clear(gl.gl.COLOR_BUFFER_BIT);
    // TODO: allow the mode to be specified, rather than being hard coded as `gl.TRIANGLE_STRIP`
    gl.gl.drawArrays(gl.gl.TRIANGLE_STRIP, 0, 4);

    const output = new Uint8Array(numElements * 4);
    gl.gl?.readPixels(0, 0, width, height, gl.gl.RGBA, gl.gl.UNSIGNED_BYTE, output);

    const average = output.reduce((acc, value, index) => (index % 4) === 3 ? acc : acc + (value/(numElements * 3)), 0) / 255;

    console.log(output);
    console.log(`${(100 * (1 - average)).toFixed(4)}%`);

    setBrightness(1 - average);
  }, [gl, width, height])

  return (
    <Card>
      <CardHeader className="pb-0">Microscope Thresholding</CardHeader>
      <CardBody className="flex flex-col gap-3">
        <div className="flex flex-row gap-1.5">
          <div className="flex flex-col gap-3 grow relative overflow-hidden rounded-lg">
            <video ref={videoRef} className="-z-10"/>
            <canvas className="absolute max-w-full max-h-full right-0 left-0 z-10 rounded-lg" ref={gl.canvasRef} width={width} height={height}/>
          </div>
          <div>
            <Slider
              size="lg"
              step={1 / 255}
              maxValue={(256 / 255)}
              minValue={0}
              orientation="vertical"
              aria-label="Threshold"
              defaultValue={0.6}
              value={threshold}
              onChange={(v) => setThreshold(Array.isArray(v) ? v[0] : v)}
            />
          </div>
        </div>

        <div className="flex flex-row justify-center items-center gap-3">
          <Button className="" onClick={getBrightness}>
            Read
          </Button>
          <CopyableOutput className="grow">
            {brightness === undefined ? "" : `${brightness.toFixed(4)}%`}
          </CopyableOutput>
        </div>
      </CardBody>
    </Card>
  );
}

export default MicroscopeThresholdWidget;