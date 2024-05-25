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
// import {getDefaultPeakFinder} from "../SpectraDisplay/ChartAnalysis.ts";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import UVVisSpecGraph from "./UVVisSpecGraph.tsx";
import {Settings} from "react-feather";
import useNumberField from "./useNumberField.ts";
import useGL from "../../hooks/webgl/gl/useGL.ts";

export interface UVVisSpecProps {
  onSave?: (x: number[], y: number[]) => void,
}


const UVVisSpec: React.FC<UVVisSpecProps> = (props) => {
  const bifrost = useBifrost({ topic: RosTopic.UV_VIS_SPEC });
  const luminance = useSelector((state: RootState) => state.uvVisSpecStore.luminance);

  const [startWavelength, startWavelengthString, setStartWavelength] = useNumberField("UVVisSpec-startWavelength", 546.5);
  const [startColumn, startColumnString, setStartColumn] = useNumberField("UVVisSpec-startCol", 0.213);

  const [endWavelength, endWavelengthString, setEndWavelength] = useNumberField("UVVisSpec-endWavelength", 611.6);
  const [endColumn, endColumnString, setEndColumn] = useNumberField("UVVisSpec-endColumn", 0.343);

  const [mousePoint, setMousePoint] = useState<[number, number]>([0, 0]);

  const gradient = (endWavelength - startWavelength) / (endColumn - startColumn);
  const viewportStartWavelength = startWavelength - gradient * startColumn;
  const viewportEndWavelength = viewportStartWavelength + gradient;

  // Called whenever the user presses the save button
  const onSave = useCallback(() => {
    if (!props.onSave)
      return;

    // Call the callback function with appropriate data
    const x = Array.from({ length: luminance.length }, (_, i) => (i) / (luminance.length-1))
    props.onSave(x, luminance);
  }, [luminance, props])

  const gl = useGL();

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

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

  const colToWavelength = useCallback((v: number) => gradient * v + viewportStartWavelength, [gradient, viewportStartWavelength]);

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

  const chart = (
    <UVVisSpecGraph
      luminance={luminance}
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
        <Button color="success" onPress={onSave} size="sm">Save</Button>
      </CardHeader>
      <CardBody>
        {chart}
      </CardBody>
      {modal}
    </Card>
    // peaks={getDefaultPeakFinder(2, 20)(data)}>
  )

}

export default UVVisSpec



