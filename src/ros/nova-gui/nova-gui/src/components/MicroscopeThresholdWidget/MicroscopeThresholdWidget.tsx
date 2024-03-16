import useGL from "../WebGLCanvas/hooks/useGL.tsx";
import {useProgram} from "../WebGLCanvas/hooks/useProgram.tsx";
import vert from "./gl/threshold.vert";
import frag from "./gl/threshold.frag";
import React, {useCallback, useEffect, useRef, useState} from "react";
import {Button, Card, CardBody, Checkbox, Input, Slider} from "@nextui-org/react";
import useSamplers, {GLSampler} from "../WebGLCanvas/hooks/useSamplers.tsx";
import useDict from "../WebGLCanvas/hooks/useDict.tsx";
import useWebcam from "../WebGLCanvas/hooks/useWebcam.tsx";
import useAttributes from "../WebGLCanvas/hooks/useAttributes.tsx";
import useCanvasSize from "../WebGLCanvas/hooks/useCanvasSize.tsx";
import useUniforms, {vec} from "../WebGLCanvas/hooks/useUniforms.tsx";
import CopyableInput from "../CopyableInput/CopyableInput.tsx";

const attributes = {
  aPosition: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  }
};

const onFloatChanged = (mutator: (x: string) => void) => (userInput: string) => {
  if (userInput.match(/((([1-9]([0-9]*))|0)((.([0-9]*))?))|(^$)/))
    mutator(userInput);
}

const MicroscopeThresholdWidget: React.FC = () => {
  const [threshold, setThreshold] = useState<number>(0.5);
  const [brightness, setBrightness] = useState<number | undefined>();
  const [showThreshold, setShowThreshold] = useState<boolean>(true);

  const [manualThreshold, setManualThreshold] = useState<string>("");
  const parsedManualThreshold = parseFloat(manualThreshold);
  const isManualThresholdValid = !isNaN(parsedManualThreshold);
  const finalThreshold = isManualThresholdValid ? (parsedManualThreshold / 100) : threshold;

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
    threshold: [finalThreshold]
  }), [threshold, manualThreshold])
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

  const toggleShowThreshold = useCallback(() => {
    setShowThreshold(!showThreshold);
  }, [showThreshold, setShowThreshold])

  /*
  <CardHeader className="pb-0 flex flex-row">
        <div className="grow">Microscope Thresholding</div>
        <Checkbox className="px-2" isSelected={showThreshold} onValueChange={setShowThreshold}></Checkbox>
      </CardHeader>
   */
  // className="flex flex-row gap-1.5"

  return (
    <Card>
      <CardBody className="flex flex-col gap-3">
        <div className="grid grid-cols-[1fr_auto] gap-3 items-center justify-items-center">
          <div className="grow justify-self-stretch text-left">Microscope Thresholding</div>
          <div className="mr-[-8px]">
            <Checkbox isSelected={showThreshold} onValueChange={setShowThreshold}/>
          </div>
          <div className="flex flex-col gap-3 grow relative overflow-hidden rounded-lg"
               onMouseDown={toggleShowThreshold}>
            <video ref={videoRef} className={showThreshold ? "-z-10" : "z-20"}/>
            <canvas className="absolute max-w-full max-h-full right-0 left-0 z-10 rounded-lg" ref={gl.canvasRef} width={width} height={height}/>
          </div>
          <Slider
            size="lg"
            step={1 / 255}
            maxValue={(256 / 255)}
            minValue={0}
            orientation="vertical"
            aria-label="Threshold"
            defaultValue={0.5}
            isDisabled={isManualThresholdValid}
            value={finalThreshold}
            classNames={{"track": "mx-0"}}
            onChange={(v) => {
              if (manualThreshold.length === 0)
                setThreshold(Array.isArray(v) ? v[0] : v);
            }}
          />
        </div>

        <div className="flex flex-row justify-center gap-3 items-end">
          <Button className="" onClick={getBrightness}>
            Read
          </Button>
          <CopyableInput className="grow basis-1"
                         size="md" labelPlacement="outside"
                         label={"Average Brightness"}
                         value={brightness === undefined ? "" : `${(100 * brightness).toFixed(4)} %`}
                         copyValue={brightness === undefined ? "" : (100 * brightness).toFixed(4)}
                         classNames={{
                           input: "font-mono",
                           inputWrapper: "data-[hover=true]:bg-default-100"
                         }}
                         placeholder={"##.#### %"}>
          </CopyableInput>
          <Input className="grow basis-1"
                 size="md"
                 labelPlacement="outside"
                 label={isManualThresholdValid ? "Threshold (manual)" : "Threshold"}
                 value={manualThreshold} onValueChange={onFloatChanged(setManualThreshold)}
                 classNames={{
                   input: "font-mono",
                 }}
                 placeholder={`${Math.min(threshold * 100, 100).toFixed(4)}`}
                 endContent={<span className="font-mono">%</span>}>
          </Input>


        </div>
      </CardBody>
    </Card>
  );
}

export default MicroscopeThresholdWidget;