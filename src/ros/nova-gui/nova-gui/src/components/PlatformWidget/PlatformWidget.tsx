import {Card, CardBody, CardHeader, CardProps, Switch} from "@nextui-org/react";
import { useEffect, useState } from "react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction.ts";
import StepperWidget from "./StepperWidget.tsx";
import { RosAction } from "../../ros/actions/RosAction.ts";
import { RosService } from "../../ros/services/rosService.ts";
import { IRosStdSrvsSetBoolResponse } from "../../ros/rosTypes.ts";


export interface PlatformWidgetProps extends CardProps {}

const SAMPLE_TRAY_LOCATIONS = [
  {
    display: "Sample Two",
    value: "sample_two",
  },
  {
    display: "Cleaning Sheath",
    value: "clean",
  },
  {
    display: "Auger",
    value: "auger",
  },
  {
    display: "Sample One",
    value: "sample_one",
  },
  {
    display: "Cache",
    value: "cache",
  },
];


const PlatformWidget: React.FC<PlatformWidgetProps> = (props) => {
  const [targetLocationIndex, setTargetLocationIndex] = useState<number>(0);
  const [disableSelector, setDisableSelector] = useState<boolean>(false);
  const [mixerOn, setMixerOn] = useState<boolean>(false);
  const bifrost = useBifrost({service: RosService.MIXERS});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);
  
  const picker = (
      <SegmentedPicker onIndexChange={setTargetLocationIndex} selectedIndex={targetLocationIndex} isDisabled={disableSelector}>
        {SAMPLE_TRAY_LOCATIONS.map((location, index) => (
          <div key={index}>{location.display}</div>
        ))}
      </SegmentedPicker>
  );


  const changeMixer = (value: boolean) => {
    bifrost.callService({data: value}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosStdSrvsSetBoolResponse;
        if (boolResponse?.success) {
          setMixerOn(value);
        }
      }
    });
  }


  const mixerRow = (
    <div className="my-3">
      <div className="font-bold">Mixers</div>
      <Switch 
        className="mt-1.5 mx-1.5" 
        isSelected={mixerOn}
        onChange={() => changeMixer(!mixerOn)}
      />
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
        {mixerRow}
      </CardBody>
    </Card>
  )

}

export default PlatformWidget;



