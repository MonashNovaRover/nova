import useGL from "../WebGLCanvas/hooks/useGL.tsx";
import {useProgram} from "../WebGLCanvas/hooks/useProgram.tsx";
import vert from "./gl/threshold.vert";
import frag from "./gl/threshold.frag";
import React, {useCallback, useEffect, useRef, useState} from "react";
import {
  Button,
  Card,
  CardBody,
  Checkbox,
  Input,
  Slider,
  Table, TableBody, TableCell,
  TableColumn,
  TableHeader,
  TableRow
} from "@nextui-org/react";
import useSamplers, {GLSampler} from "../WebGLCanvas/hooks/useSamplers.tsx";
import useDict from "../WebGLCanvas/hooks/useDict.tsx";
import useAttributes from "../WebGLCanvas/hooks/useAttributes.tsx";
import useCanvasSize from "../WebGLCanvas/hooks/useCanvasSize.tsx";
import useUniforms, {vec} from "../WebGLCanvas/hooks/useUniforms.tsx";
import CopyableInput from "../CopyableInput/CopyableInput.tsx";
import {CameraComponentProps} from "../CameraComponent/CameraComponent.tsx";
import CameraSessionStartStopButton from "../CameraComponent/components/CameraSessionStartStopButton.tsx";
import {useLocalStorage} from "../nir-probe/hooks/useLocalStorage.ts";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import {RosService} from "../../ros/services/rosService.ts";
import {useCameraStream} from "../CameraComponent/hooks/useCameraStream.ts";
import {useCameraStreamer} from "../CameraComponent/hooks/useCameraStreamer.ts";

const attributes = {
  aPosition: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  }
};

export interface ThresholdingFileEntry {
  threshold: number,
  brightness: number
}

export interface ThresholdingFile {
  entries: ThresholdingFileEntry[]
}
const EMPTY_THRESHOLDING_FILE = {
  entries: []
} as ThresholdingFile;

const onFloatChanged = (mutator: (x: string) => void) => (userInput: string) => {
  if (userInput.match(/((([1-9]([0-9]*))|0)((.([0-9]*))?))|(^$)/))
    mutator(userInput);
}

const MicroscopeThresholdWidget: React.FC<CameraComponentProps> = (props) => {
  const { cameraSerial, autostart: allCamerasStarted } = props;

  const [threshold, setThreshold] = useState<number>(0.5);
  const [brightness, setBrightness] = useState<number | undefined>();
  const [showThreshold, setShowThreshold] = useState<boolean>(true);

  const [manualThreshold, setManualThreshold] = useState<string>("");
  const parsedManualThreshold = parseFloat(manualThreshold);
  const isManualThresholdValid = !isNaN(parsedManualThreshold);
  const finalThreshold = isManualThresholdValid ? (parsedManualThreshold / 100) : threshold;

  const videoRef = useRef<HTMLVideoElement | null>(null);
  useCameraStreamer();

  const {
    streamingState,
    sendSessionStartMessage,
    closeSession,
  } = useCameraStream(cameraSerial, videoRef, allCamerasStarted); // useWebcam(videoRef);

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
  }, [gl.gl, program, videoRef.current?.width, videoRef.current?.height, samplers, uniforms, threshold, frameID]);

  const toggleShowThreshold = useCallback(() => {
    setShowThreshold(!showThreshold);
  }, [showThreshold, setShowThreshold]);

  // Calibration things


  const concentrationFunction = useCallback((brightness: number) => {
    const densityRatio = 4.45 / 1.4;
    return (densityRatio * brightness) / (brightness * (densityRatio - 1) + 1);
  }, []);

  const [filename, setFilename] = useState<string>("");

  const [file, setFile] = useLocalStorage<ThresholdingFile>(`thresholding-${filename}`,
    EMPTY_THRESHOLDING_FILE, [filename]);

  // Prepends a row to the file
  const prependFileEntry = useCallback((newEntry : ThresholdingFileEntry) => {
    const newFile = {
      entries: [newEntry, ...file.entries]
    };

    setFile(newFile);
  }, [file, setFile]);

  // Deletes an entry from the file
  const deleteFileEntry = useCallback((index: number) => {
    const newFile = {
      entries: file.entries.filter((_, i) => i !== index)
    }

    setFile(newFile);
  }, [file, setFile]);


  const getBrightness = useCallback(() => {
    const numElements = width * height;

    if (!gl.gl)
      return;

    // Redraw
    gl.gl.clear(gl.gl.COLOR_BUFFER_BIT);
    gl.gl.drawArrays(gl.gl.TRIANGLE_STRIP, 0, 4);

    const output = new Uint8Array(numElements * 4);
    gl.gl?.readPixels(0, 0, width, height, gl.gl.RGBA, gl.gl.UNSIGNED_BYTE, output);

    const average = output.reduce((acc, value, index) =>
      (index % 4) === 3 ? acc : acc + (value/(numElements * 3)), 0) / 255;

    setBrightness(1 - average);

    const fileEntry: ThresholdingFileEntry = {
      threshold: finalThreshold,
      brightness: 1 - average
    };
    prependFileEntry(fileEntry);
  }, [gl, width, height, prependFileEntry, finalThreshold]);

  // Microscope servo controls
  const topicBifrost = useBifrost({ topic: RosTopic.MICROSCOPE_SERVO });

  const microscopeServoState = useSelector(
    (state: RootState) => state.microscopeServoStore
  );

  // Accessing the Store using useSelector hook
  const microscopeServoService = useSelector((state: RootState) =>
    state.microscopeServiceStore
  );

  // Invoking Bifrost and pointing it towards SET_SERVO
  const serviceBifrost = useBifrost({ service: RosService.MOVE_MICROSCOPE_SERVO });

  const setZoomFocus = (angle: number) => serviceBifrost.callService({ angle: angle });
  const [zoom, setZoom] = useState(45);

  // Wrap with useEffect hook to only run it once
  useEffect(() => {
    // call bifrost.syncWithTopic() to initiate Realtime Updates
    topicBifrost.syncWithTopic();
  }, [topicBifrost]);

  useEffect(() => {
    if (microscopeServoState.angle !== zoom) {
      setZoomFocus(zoom);
    }
  }, [zoom]);

  const readingRows = file.entries.map(({threshold, brightness}, index) => (
    <TableRow>
      <TableCell className="font-mono">{(100 * threshold).toFixed(4)} %</TableCell>
      <TableCell className="font-mono">{(100 * brightness).toFixed(4)} %</TableCell>
      <TableCell className="font-mono">{(100 * concentrationFunction(brightness)).toFixed(4)} %</TableCell>
      <TableCell>
        <Button variant="light" fullWidth color="danger" onClick={() => deleteFileEntry(index)}>
          Delete
        </Button>
      </TableCell>
    </TableRow>
  ));

  const averageThreshold = file.entries
    .reduce((acc, v) => v.threshold + acc, 0) / Math.max(1, file.entries.length);
  const averageBrightness = file.entries
    .reduce((acc, v) => v.brightness + acc, 0) / Math.max(1, file.entries.length);
  const standardDeviation = Math.sqrt(file.entries.reduce((acc, v) => {
    const diff = v.brightness - averageBrightness;
    return acc + (diff * diff);
  }, 0) / Math.max(1, file.entries.length));

  const t_up = averageBrightness + 2.131 * (standardDeviation / Math.sqrt(file.entries.length));
  const t_down = averageBrightness - 2.131 * (standardDeviation / Math.sqrt(file.entries.length));

  const averageHeaderRow = (
    <TableRow className="relative h-6">
      <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400">
        Site Average
      </TableCell>
      <TableCell>{""}</TableCell>
      <TableCell>{""}</TableCell>
      <TableCell>{""}</TableCell>
    </TableRow>
  )

  const readingHeaderRow = (
    <TableRow className="relative h-3">
      <TableCell className="absolute text-small uppercase tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400">
        Site Readings
      </TableCell>
      <TableCell>{""}</TableCell>
      <TableCell>{""}</TableCell>
      <TableCell>{""}</TableCell>
    </TableRow>
  );

  const averageRow = (
    <TableRow>
      <TableCell className="font-mono">{(100 * averageThreshold).toFixed(4)} %</TableCell>
      <TableCell className="font-mono">{(100 * averageBrightness).toFixed(4)} %</TableCell>
      <TableCell className="font-mono">{(100 * concentrationFunction(averageBrightness)).toFixed(4)} %</TableCell>
      <TableCell>{""}</TableCell>
    </TableRow>
  );

  const noReadingHeader = (
    <TableRow className="relative h-6">
      <TableCell className="absolute text-small tracking-wider text-nowrap left-0 right-64 w-full top-0 h-1 text-foreground-400">
        No readings recorded. Press "Read" to take a reading.
      </TableCell>
      <TableCell>{""}</TableCell>
      <TableCell>{""}</TableCell>
      <TableCell>{""}</TableCell>
    </TableRow>
  );

  const tableRows = file.entries.length > 0 ?
    [averageHeaderRow, averageRow, readingHeaderRow, ...readingRows ] :
    [noReadingHeader];

  return (
    <div className="flex flex-col gap-1.5">
      <Card>
        <CardBody className="flex flex-col gap-3 overflow-hidden">
          <div className="flex flex-row">
            <div className="grow justify-self-stretch text-left col-span-3">Microscope Thresholding</div>
            <CameraSessionStartStopButton streamingState={streamingState}
                                          sendSessionStartMessage={sendSessionStartMessage}
                                          closeSession={closeSession}/>
          </div>
          <div className="grid grid-cols-[auto_1fr_1fr_auto] gap-3 items-center justify-items-center">
            <div className=
                   "flex flex-col gap-3 grow relative overflow-hidden rounded-lg col-span-3 w-full cursor-pointer"
                 onMouseDown={toggleShowThreshold}>
              <video controls={false}
                     aria-label="threshold input video"
                     autoPlay
                     loop
                     muted
                     playsInline
                     ref={videoRef}
                     className={showThreshold ? "-z-10" : "z-20"}/>
              <canvas className="absolute max-w-full max-h-full right-0 left-0 z-10 rounded-lg"
                      aria-label="thresheld output"
                      ref={gl.canvasRef}
                      width={width}
                      height={height}/>
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
            
            <Button className="place-self-end" onPress={getBrightness}>
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
            <div className="mr-[-8px] pr-[-8px] place-self-start justify-self-center">
              <Checkbox isSelected={showThreshold} onValueChange={setShowThreshold} aria-label="thresholding enabled"/>
            </div>
          </div>
        </CardBody>
      </Card>

      <div className="flex flex-col bg-default-100 rounded-xl rounded-b-2xl overflow-hidden">
        <SiteSelectWidget onValueChanged={setFilename} hideCard/>
        <Table aria-label="Thresholding readings"
               className={"shadow-lg z-10"} classNames={{
          wrapper: "rounded-t-none "
        }}>
          <TableHeader>
            <TableColumn key="threshold" className="text-center">Threshold</TableColumn>
            <TableColumn key="brightness">Average Brightness</TableColumn>
            <TableColumn key="concentration">Concentration</TableColumn>
            <TableColumn key="action" className="text-center">Action</TableColumn>
          </TableHeader>
          <TableBody>
            {...tableRows}
          </TableBody>

        </Table>
      </div>
    </div>
  );
}

export default MicroscopeThresholdWidget;