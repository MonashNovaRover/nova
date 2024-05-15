/**
 * UV Vis Spectrometer component
 * Author: Bailey Chessum
 */
import React, {useEffect} from "react";
import {Card, CardBody, CardHeader} from "@nextui-org/react";
// import {getDefaultPeakFinder} from "../SpectraDisplay/ChartAnalysis.ts";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import UVVisSpecGraph from "./UVVisSpecGraph.tsx";


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
      <CardHeader>
        UV Vis Spec
      </CardHeader>
      <CardBody>
        {chart}
      </CardBody>
    </Card>
    // peaks={getDefaultPeakFinder(2, 20)(data)}>
  )

}

export default UVVisSpec



