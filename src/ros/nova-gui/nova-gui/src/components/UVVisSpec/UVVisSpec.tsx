/**
 * UV Vis Spectrometer component
 * Author: Bailey Chessum
 */
import React, {useEffect} from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownTrigger, Input, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader, useDisclosure
} from "@nextui-org/react";
// import {getDefaultPeakFinder} from "../SpectraDisplay/ChartAnalysis.ts";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import UVVisSpecGraph from "./UVVisSpecGraph.tsx";
import {MoreHorizontal} from "react-feather";
import useNumberField from "./useNumberField.ts";


const UVVisSpec: React.FC = () => {
  const bifrost = useBifrost({ topic: RosTopic.UV_VIS_SPEC });
  const luminance = useSelector((state: RootState) => state.uvVisSpecStore.luminance);

  const [startWavelength, startWavelengthString, setStartWavelength] = useNumberField("UVVisSpec-startWavelength", 600);
  const [startColumn, startColumnString, setStartColumn] = useNumberField("UVVisSpec-startWavelength", 0.25);

  const [endWavelength, endWavelengthString, setEndWavelength] = useNumberField("UVVisSpec-startWavelength", 1500);
  const [endColumn, endColumnString, setEndColumn] = useNumberField("UVVisSpec-endColumn", 0.75);

  const gradient = (endWavelength - startWavelength) / (endColumn - startColumn);
  const viewportStartWavelength = startWavelength - gradient * startColumn;
  const viewportEndWavelength = viewportStartWavelength + gradient;

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const {isOpen: isSettingsOpen, onOpen: onSettingsOpen, onOpenChange: onSettingsOpenChange} = useDisclosure();

  const modal = (
    <Modal className="dark" isOpen={isSettingsOpen} onOpenChange={onSettingsOpenChange}>
      <ModalContent>
        {(onClose) => (
          <>
            <ModalHeader className="flex flex-col gap-1">Modal Title</ModalHeader>
            <ModalBody>
              <Input value={startColumnString} onValueChange={setStartColumn}/>
              <Input value={endColumnString} onValueChange={setEndColumn}/>
              <Input value={startWavelengthString} onValueChange={setStartWavelength}/>
              <Input value={endWavelengthString} onValueChange={setEndWavelength}/>
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
    <Dropdown className="m-0">
      <DropdownTrigger>
        <Button
          variant={"light"}
          isIconOnly
          className="m-0"
        >
          <MoreHorizontal></MoreHorizontal>
        </Button>
      </DropdownTrigger>
      <DropdownMenu aria-label="Static Actions">
        <DropdownItem key="advanced" onPress={onSettingsOpen}>
          Settings
        </DropdownItem>
      </DropdownMenu>
    </Dropdown>
  )

  const chart = (
    <UVVisSpecGraph
      luminance={luminance}
      colEndPercent={0.95}
      colStartPercent={0.05}
      wavelengthLabelCount={20}
      percentageLabelCount={10}

      startWavelength={viewportStartWavelength}
      endWavelength={viewportEndWavelength}
    >

    </UVVisSpecGraph>
  )

  return (
    <Card>
      <CardHeader className="flex flex-rowo">
        <div className="grow">UV Vis Spec</div>
        {settingsDropdown}
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



