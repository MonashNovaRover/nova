/**
 * UV Vis Spectrometer component
 * Author: Bailey Chessum
 */
import React, {useCallback, useEffect, useRef, useState} from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Input,
  Switch,
} from "@nextui-org/react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import UVVisSpecGraph from "./UVVisSpecGraph.tsx";
import {Eye, EyeOff, Settings} from "react-feather";
import useNumberField from "./useNumberField.ts";
import useGL from "../../../hooks/webgl/gl/useGL.ts";
import {zip} from "lodash";
import useDownload from "../../../hooks/useDownload.ts";
import RamanLocalStorageSaveButton from "../RamanSpec/RamanLocalStorageSaveButton.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
//import {UVVisSpecStartStopButtons} from "./UVVisSpecStartStopButtons.tsx";
import {useCarouselPosition} from "../CarouselWidget/CarouselPositionContext.tsx";
import {getCuvetteName} from "../CarouselWidget/cuvetteName.ts";
import {shouldApplyBlank} from "./chemicalConfig.ts";

/**
 * Scale factor derived from max RGB luminance: sqrt(3 * 255^2) / 100 = 4.4167295593
 * Used to convert raw luminance to percentage scale (0-100%)
 */
export const LUMINANCE_SCALE_FACTOR = 4.4167295593;

/**
 * Converts raw luminance to percentage scale.
 * @param luminance - Raw luminance value
 * @returns Value in percentage scale (raw 441.67 → 100%)
 */
export function luminanceToPercent(luminance: number): number {
  return luminance / LUMINANCE_SCALE_FACTOR;
}

export interface UVVisSpecProps {
  onSave?: (points: number[][], name: string) => void,
  overwriteDuplicates?: boolean,
  onOverwriteToggle?: (value: boolean) => void,
}

const UVVisSpec: React.FC<UVVisSpecProps> = (props) => {
  const bifrost = useBifrost({ topic: RosTopic.UV_VIS_SPEC });
  const luminance = useSelector((state: RootState) => state.uvVisSpecStore.luminance);

  /**
   * Dynamic graph naming based on carousel position.
   *
   * The name is determined by which cuvette is currently positioned:
   * - When inner ring is at position 12: uses outer cuvette name
   * - When outer ring is at positions 22-24: uses inner cuvette name
   * - Otherwise: no auto-name (user must enter manually)
   *
   * If the carousel context is not available (e.g., component used
   * outside URCScienceView), this gracefully falls back to no auto-name.
   */
  const carouselPosition = useCarouselPosition();
  const suggestedName = carouselPosition
    ? getCuvetteName(carouselPosition.innerCuvette, carouselPosition.outerCuvette)
    : undefined;

  const [blankLuminance, setBlankLuminance] = useGenericStore<number[]>("uvVisBlankStore");
  const [showBlank, setShowBlank] = useState(true);
  const saveBlankLuminance = () => {setBlankLuminance(luminance)};

  // Auto-apply/remove blank when cuvette changes
  const prevSuggestedName = useRef<string | undefined>(undefined);
  if (prevSuggestedName.current !== suggestedName) {
    prevSuggestedName.current = suggestedName;
    if (suggestedName) {
      setShowBlank(shouldApplyBlank(suggestedName));
    }
  }

  const blankLuminanceIsValid = luminance.length === blankLuminance.length;
  const absorbanceData = blankLuminanceIsValid
    ? zip(luminance, blankLuminance)
        .map(([final, initial]) => -Math.log10(final! / initial!))
    : null;

  // Show absorbance if blank is valid and showBlank is enabled, otherwise show raw luminance
  const displayData = (absorbanceData && showBlank) ? absorbanceData : luminance;
  const isShowingAbsorbance = absorbanceData !== null && showBlank;

  const [peak1Wavelength, peak1WavelengthString, setPeak1Wavelength] = useNumberField("UVVisSpec-peak1Wavelength", 545);
  const [peak1X, peak1XString, setPeak1X] = useNumberField("UVVisSpec-peak1X", 0.523);

  const [peak2Wavelength, peak2WavelengthString, setPeak2Wavelength] = useNumberField("UVVisSpec-peak2Wavelength", 611);
  const [peak2X, peak2XString, setPeak2X] = useNumberField("UVVisSpec-peak2X", 0.720);

  const [mousePoint, setMousePoint] = useState<[number, number]>([0, 0]);

  const gradient = (peak2Wavelength - peak1Wavelength) / (peak2X - peak1X);
  const viewportStartWavelength = peak1Wavelength - gradient * peak1X;
  const viewportEndWavelength = viewportStartWavelength + gradient;

  // Function that converts col values from 0 to 1 into a wavelength using calibration data
  const colToWavelength = useCallback((v: number) => gradient * v + viewportStartWavelength, [gradient, viewportStartWavelength]);

  // Called whenever the user presses the save button
  const onSave = useCallback((graphName: string) => {
    if (!props.onSave)
      return;

    const savedData = displayData;

    // [x, y] points to return - raw values (not normalized)
    const points = savedData.map((val, i) => (
      [colToWavelength((i) / (savedData.length-1)), isShowingAbsorbance ? val : luminanceToPercent(val)]
    ))

    props.onSave(points, graphName);
  }, [colToWavelength, displayData, isShowingAbsorbance, props])

  const download = useDownload("uv-vis-spec.csv", () => {
    const savedData = displayData;

    // [x, y] points to return - raw values (not normalized)
    const points = savedData.map((val, i) => (
      [colToWavelength((i) / (savedData.length-1)), isShowingAbsorbance ? val : luminanceToPercent(val)]
    ));

    const header = isShowingAbsorbance ? "wavelength,absorbance" : "wavelength,intensity_percent";
    const lines = [header];
    for (let i = 0; i < points.length; i++)
      lines.push(`${points[i][0]},${points[i][1]}`);

    return lines.join('\n');
  }, [displayData, isShowingAbsorbance, colToWavelength], { type: "text/csv;charset=utf-8" })

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const gl = useGL();
  const onMouseMove = (event: React.MouseEvent<HTMLCanvasElement, MouseEvent>) => {
    if (!gl.canvasRef.current)
      return;

    const bounds = gl.canvasRef.current.getBoundingClientRect();
    const pixelsX = event.clientX - bounds.left;
    const pixelsY = event.clientY - bounds.top;

    const x = pixelsX / bounds.width;
    const y = pixelsY / bounds.height;

    setMousePoint([x, y]);
  }

  const [settingsOpen, setSettingsOpen] = useState(false)

  const settings = (
    <div className="grid grid-cols-4 gap-3 mb-2">
      <Input value={peak1XString} onValueChange={setPeak1X} label={"Peak 1 X"}/>
      <Input value={peak1WavelengthString} onValueChange={setPeak1Wavelength} label={"Peak 1 Wavelength"}/>
      <Input value={peak2XString} onValueChange={setPeak2X} label={"Peak 2 X"}/>
      <Input value={peak2WavelengthString} onValueChange={setPeak2Wavelength} label={"Peak 2 Wavelength"}/>
      {props.onOverwriteToggle !== undefined && (
        <Switch
          size="sm"
          isSelected={props.overwriteDuplicates}
          onValueChange={props.onOverwriteToggle}
        >
          Overwrite duplicates
        </Switch>
      )}
    </div>
  )

  const settingsDropdown = (
    <Button
      variant={"light"}
      isIconOnly
      className="m-0"
      onPress={() => setSettingsOpen(!settingsOpen)}
      size="sm"
    >
      <Settings></Settings>
    </Button>
  )

  const blankButtons = (
    <div className="flex flex-row gap-3 items-end">
      <Button color={"default"} size="sm" onPress={saveBlankLuminance}>
        Set Blank
      </Button>
      {blankLuminanceIsValid && (
        <Button
          color={showBlank ? "secondary" : "primary"}
          size="sm"
          startContent={!showBlank ? <Eye size={16}/> : <EyeOff size={16}/>}
          onPress={() => setShowBlank(!showBlank)}
        >
          {showBlank ? "Hide Blank" : "Use Blank"}
        </Button>
      )}
    </div>
  );

  const chart = (
    <UVVisSpecGraph
      luminance={displayData}
      colEndPercent={0.95}
      colStartPercent={0.05}
      wavelengthLabelCount={11}
      percentageLabelCount={10}

      startWavelength={viewportStartWavelength}
      endWavelength={viewportEndWavelength}
      onMouseMove={onMouseMove}
      gl={gl}

      yAxisLabel={isShowingAbsorbance ? "Absorbance" : "Intensity (%)"}
      yAxisFormatter={isShowingAbsorbance ? (val) => val.toFixed(2) : (val) => `${val.toFixed(0)}%`}
    />
  )

  return (
    <Card>
      <CardHeader className="flex flex-row gap-2">
        <div className="grow">UV Vis Spec</div>
        <div className="font-mono px-3 opacity-75">
          ({mousePoint[0].toFixed(3)}, {mousePoint[1].toFixed(3)}) -{'>'} {colToWavelength(mousePoint[0]).toFixed(2)} nm 
        </div>
        {/*<UVVisSpecStartStopButtons/>*/}
        {settingsDropdown}
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        {settingsOpen && settings}
        <div>
          {chart}
        </div>
        <div className="flex flex-row gap-3 items-end">
          {blankButtons}
          <RamanLocalStorageSaveButton key={suggestedName} onSave={onSave} onCSVSave={download} suggestedName={suggestedName} />
        </div>
      </CardBody>
    </Card>
  )

}

export default UVVisSpec



