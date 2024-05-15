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
  DropdownTrigger
} from "@nextui-org/react";
// import {getDefaultPeakFinder} from "../SpectraDisplay/ChartAnalysis.ts";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import UVVisSpecGraph from "./UVVisSpecGraph.tsx";
import {MoreHorizontal} from "react-feather";


const UVVisSpec: React.FC = () => {

  const bifrost = useBifrost({ topic: RosTopic.UV_VIS_SPEC });
  const luminance = useSelector((state: RootState) => state.uvVisSpecStore.luminance);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);



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
        <DropdownItem key="advanced" onPress={() => {}}>
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
      wavelengthLabelCount={5}
      startWavelength={600}
      endWavelength={1500}
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
    </Card>
    // peaks={getDefaultPeakFinder(2, 20)(data)}>
  )

}

export default UVVisSpec



