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
import CopyableInput from "../../shared/components/CopyableInput/CopyableInput.tsx";
import {CameraComponentProps} from "../../cameras/CameraComponent/CameraComponent.tsx";
import CameraSessionStartStopButton from "../../cameras/CameraComponent/components/CameraSessionStartStopButton.tsx";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useCameraStream} from "../../cameras/CameraComponent/hooks/useCameraStream.ts";
import {useCameraStreamer} from "../../cameras/CameraComponent/hooks/useCameraStreamer.ts";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../../hooks/webgl/program/useProgram.ts";
import useSampler from "../../../hooks/webgl/program/sampler/useSampler.ts";
import useUniform from "../../../hooks/webgl/program/uniform/useUniform.ts";
import useAttribute from "../../../hooks/webgl/program/attribute/useAttribute.ts";
import AutosizedGLCanvas from "../../shared/components/AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteData, SiteDataState} from "../../../redux/models/genericStores/SiteDataState.ts";

export interface ThresholdingFileEntry {
  threshold: number,
  brightness: number
}

const onFloatChanged = (mutator: (x: string) => void) => (userInput: string) => {
  if (userInput.match(/((([1-9]([0-9]*))|0)((.([0-9]*))?))|(^$)/))
    mutator(userInput);
}

const MicroscopeThresholdWidget: React.FC<CameraComponentProps> = (props) => {
  const { cameraSerial, autostart  } = props;

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
  } = useCameraStream(cameraSerial, videoRef, autostart);

  const gl = useGL();
  const program = useProgram(gl, vert, frag);

  useAttribute(program, "aPosition", [
    [1, 1], [-1, 1], [1, -1], [-1, -1]
  ]);
  useSampler(program, 0, "image", videoRef.current);
  useUniform(program, "threshold", () => [finalThreshold], [finalThreshold]);

  const toggleShowThreshold = useCallback(() => {
    setShowThreshold(!showThreshold);
  }, [showThreshold, setShowThreshold]);

  // Calibration things
  const concentrationFunction = useCallback((brightness: number) => {
    const densityRatio = 4.45 / 1.4;
    return (densityRatio * brightness) / (brightness * (densityRatio - 1) + 1);
  }, []);

  // current site and related site data
  const [currentSite, _] = useGenericStore<Site>("currentSite");
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");

  // thresholding entries at the current site
  const thresholdingEntries = siteData[currentSite].thresholdingEntries;

  // sets the new thresholding entries to the current site
  const setEntries = useCallback((newEntries: ThresholdingFileEntry[]) => {
    setSiteData({
      ...siteData,
      [currentSite]: {
        ...siteData[currentSite],
        thresholdingEntries: newEntries
      } as SiteData
    } as SiteDataState)
  }, [currentSite, siteData, setSiteData])

  // Prepends a row to the file
  const prependFileEntry = useCallback((newEntry : ThresholdingFileEntry) => {
    setEntries([newEntry, ...thresholdingEntries]);
  }, [thresholdingEntries, setEntries]);

  // Deletes an entry from the file
  const deleteFileEntry = useCallback((index: number) => {
    setEntries(thresholdingEntries.filter((_, i) => i !== index));
  }, [thresholdingEntries, setEntries]);

  const getBrightness = useCallback(() => {
    if (!gl.canvasRef.current)
      return;

    const width = gl.canvasRef.current.width;
    const height = gl.canvasRef.current.height;

    const numElements = width * height;

    if (!gl.context)
      return;

    // Redraw
    gl.render(true);

    const output = new Uint8Array(numElements * 4);
    gl.context?.readPixels(0, 0, width, height, gl.context.RGBA, gl.context.UNSIGNED_BYTE, output);

    const average = output.reduce((acc, value, index) =>
      (index % 4) === 3 ? acc : acc + (value/(numElements * 3)), 0) / 255;

    setBrightness(1 - average);

    const fileEntry: ThresholdingFileEntry = {
      threshold: finalThreshold,
      brightness: 1 - average
    };
    prependFileEntry(fileEntry);
  }, [gl, prependFileEntry, finalThreshold]);

  // Microscope servo controls
  const topicBifrost = useBifrost({ topic: RosTopic.MICROSCOPE_SERVO });

  // Wrap with useEffect hook to only run it once
  useEffect(() => {
    // call bifrost.syncWithTopic() to initiate Realtime Updates
    topicBifrost.syncWithTopic();
  }, [topicBifrost]);

  const readingRows = thresholdingEntries.map(({threshold, brightness}, index) => (
    <TableRow>
      <TableCell className="font-mono">{(100 * threshold).toFixed(4)} %</TableCell>
      <TableCell className="font-mono">{(100 * brightness).toFixed(4)} %</TableCell>
      <TableCell className="font-mono">{(100 * concentrationFunction(brightness)).toFixed(4)} %</TableCell>
      <TableCell>
        <Button variant="light" fullWidth color="danger" onPress={() => deleteFileEntry(index)}>
          Delete
        </Button>
      </TableCell>
    </TableRow>
  ));

  const averageThreshold = thresholdingEntries
    .reduce((acc, v) => v.threshold + acc, 0) / Math.max(1, thresholdingEntries.length);
  const averageBrightness = thresholdingEntries
    .reduce((acc, v) => v.brightness + acc, 0) / Math.max(1, thresholdingEntries.length);

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

  const tableRows = thresholdingEntries.length > 0 ?
    [averageHeaderRow, averageRow, readingHeaderRow, ...readingRows ] :
    [noReadingHeader];

  return (
    <div className="flex flex-col gap-1.5">
      <Card className="z-0" shadow="none">
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

              <AutosizedGLCanvas gl={gl} sizeTarget={videoRef.current} drawChildrenBelow={showThreshold}>
                <video controls={false}
                       aria-label="threshold input video"
                       autoPlay
                       loop
                       muted
                       playsInline
                       ref={videoRef}/>
              </AutosizedGLCanvas>
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
        <SiteSelectWidget/>
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