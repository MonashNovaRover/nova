/**
 * UV Vis Spectrometer component
 * Author: Bailey Chessum
 */
import React, {useCallback, useEffect, useState} from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Input,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
  useDisclosure
} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import UVVisSpecGraph from "./UVVisSpecGraph.tsx";
import {Settings} from "react-feather";
import useNumberField from "./useNumberField.ts";
import useGL from "../../hooks/webgl/gl/useGL.ts";
import {max, zip} from "lodash";
import useDownload from "../../hooks/useDownload.ts";
import RamanLocalStorageSaveButton from "../RamanSpec/RamanLocalStorageSaveButton.tsx";

export interface UVVisSpecProps {
  onSave?: (points: number[][], name: string) => void,
}

const UVVisSpec: React.FC<UVVisSpecProps> = (props) => {
  const bifrost = useBifrost({ topic: RosTopic.UV_VIS_SPEC });
  const luminance = useSelector((state: RootState) => state.uvVisSpecStore.luminance);

  const [blankLuminance, setBlankLuminance] = useState<number[]>([]);
  const saveBlankLuminance = () => {setBlankLuminance(luminance)};

  const blankLuminanceIsValid = luminance.length === blankLuminance.length;
  const beerLambertZip = !blankLuminanceIsValid ? luminance
    : zip(luminance, blankLuminance)
        .map(([final, initial]) => -Math.log10(final! / initial!));

  const [startWavelength, startWavelengthString, setStartWavelength] = useNumberField("UVVisSpec-startWavelength", 546.5);
  const [startColumn, startColumnString, setStartColumn] = useNumberField("UVVisSpec-startCol", 0.213);

  const [endWavelength, endWavelengthString, setEndWavelength] = useNumberField("UVVisSpec-endWavelength", 611.6);
  const [endColumn, endColumnString, setEndColumn] = useNumberField("UVVisSpec-endColumn", 0.343);

  const [mousePoint, setMousePoint] = useState<[number, number]>([0, 0]);

  const gradient = (endWavelength - startWavelength) / (endColumn - startColumn);
  const viewportStartWavelength = startWavelength - gradient * startColumn;
  const viewportEndWavelength = viewportStartWavelength + gradient;

  // Function that converts col values from 0 to 1 into a wavelength using calibration data
  const colToWavelength = useCallback((v: number) => gradient * v + viewportStartWavelength, [gradient, viewportStartWavelength]);

  // Called whenever the user presses the save button
  const onSave = useCallback((graphName: string) => {
    if (!props.onSave)
      return;

    const savedLuminance = beerLambertZip;

    const maxLuminance = Math.max(max(savedLuminance) ?? 441.67295593, 10);

    // [x, y] points to return
    const points = savedLuminance.map((lum, i) => (
      [colToWavelength((i) / (savedLuminance.length-1)), lum / maxLuminance]
    ))

    props.onSave(points, graphName);
  }, [colToWavelength, beerLambertZip, props])

  const download = useDownload("uv-vis-spec.csv", () => {
    const savedLuminance = beerLambertZip;
    const maxLuminance = Math.max(max(savedLuminance) ?? 441.67295593, 10);

    // [x, y] points to return
    const points = savedLuminance.map((lum, i) => (
      [colToWavelength((i) / (savedLuminance.length-1)), lum / maxLuminance]
    ));

    const lines = ["wavelength,intensity"];
    for (let i = 0; i < points.length; i++)
      lines.push(`${points[i][0]},${points[i][1]}`);

    return lines.join('\n');
  }, [beerLambertZip, colToWavelength], { type: "text/csv;charset=utf-8" })

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const gl = useGL();
  const onMouseMove = useCallback((event: React.MouseEvent<HTMLCanvasElement, MouseEvent>) => {
    if (!gl.canvasRef.current)
      return;

    const bounds = gl.canvasRef.current.getBoundingClientRect();
    const pixelsX = event.clientX - bounds.left;
    const pixelsY = event.clientY - bounds.top;

    const x = pixelsX / bounds.width;
    const y = pixelsY / bounds.height;

    setMousePoint([x, y]);
  }, [gl.canvasRef])


  const {isOpen: isSettingsOpen, onOpen: onSettingsOpen, onOpenChange: onSettingsOpenChange} = useDisclosure();

  const modal = (
    <Modal className="dark" isOpen={isSettingsOpen} onOpenChange={onSettingsOpenChange}>
      <ModalContent>
        {(onClose) => (
          <>
            <ModalHeader className="flex flex-col gap-1">Modal Title</ModalHeader>
            <ModalBody>
              <Input value={startColumnString} onValueChange={setStartColumn} label={"Start Column"}/>
              <Input value={endColumnString} onValueChange={setEndColumn} label={"End Column"}/>
              <Input value={startWavelengthString} onValueChange={setStartWavelength} label={"Start Wavelength"}/>
              <Input value={endWavelengthString} onValueChange={setEndWavelength} label={"End Wavelength"}/>
            </ModalBody>
            <ModalFooter>
              <Button color="danger" variant="light" onPress={onClose}>
                Close
              </Button>
            </ModalFooter>
          </>
        )}
      </ModalContent>
    </Modal>
  );

  // Construct the data into a format to be displayed by <DataChart>
  /*const apexDataOutput = [{
    name: "Webcam Slice",
    data: data
  }];*/

  const settingsDropdown = (
    <Button
      variant={"light"}
      isIconOnly
      className="m-0"
      onPress={onSettingsOpen}
      size="sm"
    >
      <Settings></Settings>
    </Button>
  )

  const blankButtons = (
    <div className="flex flex-row gap-3 items-end">

      <Button color={"primary"} size="sm" onPress={saveBlankLuminance}>
        Set Blank
      </Button>
      <Button color={"default"} size="sm" onPress={() => {setBlankLuminance([])}}>
        Clear Blank
      </Button>
    </div>
  );

  const chart = (
    <UVVisSpecGraph
      luminance={beerLambertZip}
      colEndPercent={0.95}
      colStartPercent={0.05}
      wavelengthLabelCount={11}
      percentageLabelCount={10}

      startWavelength={viewportStartWavelength}
      endWavelength={viewportEndWavelength}
      onMouseMove={onMouseMove}
      gl={gl}
    >

    </UVVisSpecGraph>
  )

  return (
    <Card>
      <CardHeader className="flex flex-row gap-2">
        <div className="grow">UV Vis Spec</div>
        <div className="font-mono px-3 opacity-75">
          ({mousePoint[0].toFixed(3)}, {mousePoint[1].toFixed(3)}) -{'>'} {colToWavelength(mousePoint[0]).toFixed(2)} nm 
        </div>
        {settingsDropdown}
      </CardHeader>
      <CardBody>
        {chart}
        <div className="flex flex-row gap-3">
          {blankButtons}
          <RamanLocalStorageSaveButton onSave={onSave} onCSVSave={download}></RamanLocalStorageSaveButton>
        </div>
      </CardBody>
      {modal}
    </Card>
    // peaks={getDefaultPeakFinder(2, 20)(data)}>
  )

}

export default UVVisSpec



