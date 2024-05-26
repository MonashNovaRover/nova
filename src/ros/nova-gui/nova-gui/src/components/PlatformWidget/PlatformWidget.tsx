import {Card, CardBody, CardHeader, CardProps, Switch} from "@nextui-org/react";
import { useState } from "react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";

import StepperWidget from "./StepperWidget.tsx";
import { RosAction } from "../../ros/actions/RosAction.ts";


export interface PlatformWidgetProps extends CardProps {}

const SAMPLE_TRAY_LOCATIONS = [
  {
    display: "Auger",
    value: "auger",
  },
  {
    display: "Sample One",
    value: "sample_one",
  },
  {
    display: "Sample Two",
    value: "sample_two",
  },
  {
    display: "Cache",
    value: "cache",
  },
  {
    display: "Cleaning Sheath",
    value: "clean",
  },
];


const PlatformWidget: React.FC<PlatformWidgetProps> = (props) => {
  const [targetLocationIndex, setTargetLocationIndex] = useState<number>(0);
  const [disableSelector, setDisableSelector] = useState<boolean>(false);
  
  const picker = (
      <SegmentedPicker onIndexChange={setTargetLocationIndex} selectedIndex={targetLocationIndex} isDisabled={disableSelector}>
        {SAMPLE_TRAY_LOCATIONS.map((location, index) => (
          <div key={index}>{location.display}</div>
        ))}
      </SegmentedPicker>
  );


  const ethanolPumpRow = (
    <div className="my-3">
      <div className="font-bold">Ethanol Pump</div>
      <Switch className="mt-1.5 mx-1.5"></Switch>
    </div>
  );


  return (
    <Card {...props}>
      <CardHeader className="pb-0">
      Platform
      </CardHeader>
      <CardBody>
        <StepperWidget 
          rosActionType={RosAction.SAMPLE_TRAY}
          locations={SAMPLE_TRAY_LOCATIONS} 
          targetLocationIndex={targetLocationIndex} 
          setTargetLocationIndex={setTargetLocationIndex}
          setDisableSelector={setDisableSelector} 
          canZero> 
          {picker}
        </StepperWidget>
        {ethanolPumpRow}
      </CardBody>
    </Card>
  )

}

export default PlatformWidget;



