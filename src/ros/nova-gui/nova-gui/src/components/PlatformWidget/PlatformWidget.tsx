import {Button, Card, CardBody, CardHeader, Switch} from "@nextui-org/react";
import {useEffect, useState} from "react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";
import { IRosNovaInterfacesStepperActionFeedback, IRosNovaInterfacesStepperActionGoal, IRosNovaInterfacesStepperActionGoalConst, IRosNovaInterfacesStepperActionResult } from "../../ros/rosTypes.ts";
import { useRosAction } from "../../hooks/ros/useRosAction.ts";
import { RosAction } from "../../ros/actions/RosAction.ts";
import toast from "react-hot-toast";


export interface PlatformWidgetProps {

}

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


const PlatformWidget: React.FC<PlatformWidgetProps> = () => {
  const [currentLocationIndex, setCurrentLocationIndex] = useState<number | null>(null);
  const [targetLocationIndex, setTargetLocationIndex] = useState<number>(0);
  const [targetPosition, setTargetPosition] = useState<number | null>(null);
  const [actionSent, setActionSent] = useState<boolean>(false);
  const { sendGoal, feedback, goalResponse, cancelGoal} = useRosAction(RosAction.SCIENCE_SAMPLE_TRAY);

  const stepperFeedback = feedback as IRosNovaInterfacesStepperActionFeedback;
  const stepperGoalResponse = goalResponse as IRosNovaInterfacesStepperActionResult;

  
  const goTo = () => {
    console.log("Applying Sample Tray Position");
    const goal : IRosNovaInterfacesStepperActionGoal = {
      "goal": SAMPLE_TRAY_LOCATIONS[targetLocationIndex].value,
      "action": IRosNovaInterfacesStepperActionGoalConst.GO_TO
    };

    console.log(goal);

    sendGoal(goal);
    setActionSent(true);

  };

  const zero = () => {
    console.log("Zeroing Stepper");
    const goal : IRosNovaInterfacesStepperActionGoal = {
      "goal": SAMPLE_TRAY_LOCATIONS[targetLocationIndex].value,
      "action": IRosNovaInterfacesStepperActionGoalConst.ZERO
    };

    console.log(goal);

    sendGoal(goal);
    setActionSent(true);

  }

  const cancel = () => {
    cancelGoal();
    setActionSent(false);
    setTargetPosition(null);
  }
  
  
  const pickerRow = (
    <div className="mt-3">
      <div className="font-bold">Select catcher to extend to</div>
      <div className="flex flex-row mt-1.5 gap-3">
        <SegmentedPicker onIndexChange={setTargetLocationIndex} selectedIndex={targetLocationIndex} isDisabled={actionSent}>
          {SAMPLE_TRAY_LOCATIONS.map((location, index) => (
            <div key={index}>{location.display}</div>
          ))}
        </SegmentedPicker>
        <Button color="primary" isDisabled={actionSent} onPress={() => goTo()}  >
          Go To
        </Button>
      </div>
    </div>
  );

  const buttonRow = (
    <div className="flex flex-row mt-3 gap-5 justify-center">
      <Button color="warning" onPress={() => zero()}>Zero Stepper</Button>
      <Button color="danger" onPress={() => cancel()}>Cancel Action</Button>
    </div>
  );

  const ethanolPumpRow = (
    <div className="my-3">
      <div className="font-bold">Ethanol Pump</div>
      <Switch className="mt-1.5 mx-1.5"></Switch>
    </div>
  );

  useEffect(() => {
    if (!actionSent){
      return;
    }
    console.log(`Goal Response: ${goalResponse}`);
    console.log(`Action Sent: ${actionSent}`)
    if (goalResponse) {
      if (stepperGoalResponse.success) {
        setCurrentLocationIndex(targetLocationIndex);
        
        
        toast.success(`Successfully moved to ${SAMPLE_TRAY_LOCATIONS[targetLocationIndex].display}`);
      }
      else {
        toast.error(`Failed to move to ${SAMPLE_TRAY_LOCATIONS[targetLocationIndex].display}`);
      }
      if (feedback){
        setTargetPosition(null);
      }
      setActionSent(false);
    }

  },[actionSent, goalResponse]);

  useEffect(() => {
    if (!actionSent){
      return;
    }
    if (targetPosition === null && feedback) {
      setTargetPosition(stepperFeedback.goal_position);
    }
  },[feedback]);


  return (
    <Card>
      <CardHeader className="pb-0">
      Platform
      </CardHeader>
      <CardBody>
        <div className="grid grid-cols-3">
          <div className="font-bold">Current Location:</div>
          <div className="">{currentLocationIndex ? SAMPLE_TRAY_LOCATIONS[currentLocationIndex].display : "Unknown (Please Zero)"}</div>
          <div className="">{feedback ? stepperFeedback.current_position: "-"}</div>
          <div className="font-bold">Target Location:</div>
          <div className="">{SAMPLE_TRAY_LOCATIONS[targetLocationIndex].display}</div>
          <div className="">{targetPosition ? targetPosition : "-"}</div>
        </div>
        {pickerRow}
        {buttonRow}
        {ethanolPumpRow}
      </CardBody>
    </Card>
  )

}

export default PlatformWidget;



