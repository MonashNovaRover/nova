import {Button} from "@nextui-org/react";
import {useEffect, useState} from "react";
import { IRosNovaInterfacesStepperActionFeedback, IRosNovaInterfacesStepperActionGoal, IRosNovaInterfacesStepperActionGoalConst, IRosNovaInterfacesStepperActionResult } from "../../ros/rosTypes.ts";
import { useRosAction } from "../../hooks/ros/useRosAction.ts";
import { RosAction } from "../../ros/actions/RosAction.ts";
import toast from "react-hot-toast";

type Location = {
  display: string;
  value: string;
}

export interface StepperWidgetProps {
  rosActionType: RosAction;
  locations: Location[];
  targetLocationIndex: number;
  setTargetLocationIndex: React.Dispatch<React.SetStateAction<number>>;
  children: React.ReactNode;
  canZero?: boolean;
  canSet?: boolean;
  setDisableSelector: React.Dispatch<React.SetStateAction<boolean>>;
}


const StepperWidget: React.FC<StepperWidgetProps> = (props) => {
  const { locations, targetLocationIndex, setTargetLocationIndex, children, rosActionType, setDisableSelector, canSet=false, canZero=false } = props;
  const [currentLocationIndex, setCurrentLocationIndex] = useState<number | null>(null);
  const [targetPosition, setTargetPosition] = useState<number | null>(null);
  const [actionSent, setActionSent] = useState<boolean>(false);
  const { sendGoal, feedback, goalResponse, cancelGoal} = useRosAction(rosActionType);

  const stepperFeedback = feedback as IRosNovaInterfacesStepperActionFeedback;
  const stepperGoalResponse = goalResponse as IRosNovaInterfacesStepperActionResult;

  const sendAction = (action: IRosNovaInterfacesStepperActionGoalConst) => {
    if (actionSent) {
      return;
    }
    const zero = action === IRosNovaInterfacesStepperActionGoalConst.ZERO;
    const goal : IRosNovaInterfacesStepperActionGoal = {
      "goal": zero ? "zero" : locations[targetLocationIndex].value,
      "action": action
    };

    console.log(goal);

    sendGoal(goal);
    setActionSent(true);
  }

  
  const goTo = () => {
    console.log(`Stepper Go To: ${locations[targetLocationIndex].display}`);
    sendAction(IRosNovaInterfacesStepperActionGoalConst.GO_TO);
  };

    
  const set = () => {
    console.log(`Stepper Set: ${locations[targetLocationIndex].display}`);
    sendAction(IRosNovaInterfacesStepperActionGoalConst.SET);

  };

  const zero = () => {
    console.log("Zero Stepper");
    sendAction(IRosNovaInterfacesStepperActionGoalConst.ZERO);
  }

  const cancel = () => {
    console.log("Cancel Action");
    cancelGoal();
    setActionSent(false);
    setTargetPosition(null);
  }

  const next = () => {
    console.log("Next");
    setTargetLocationIndex((targetLocationIndex + 1) % locations.length);
    goTo();
  }

  const prev = () => {
    console.log("Prev");
    setTargetLocationIndex((targetLocationIndex - 1) % locations.length);
    goTo();
  }

  
  
  const pickerRow = (
    <div className="mt-3">
      <div className="font-bold">Select Location</div>
      <div className="flex flex-row mt-1.5 gap-3 justify-center ">
        {children}
        <Button color="primary" isDisabled={actionSent} onPress={() => goTo()}>
          Go To
        </Button>
      </div>
    </div>
  );

  const buttonRow = (
    <div className="flex flex-col mt-3 gap-5 justify-center">
      <div className="flex flex-row mt-3 gap-5 justify-center">
        {canZero && <Button color="warning" isDisabled={actionSent} onPress={() => zero()}>Zero Stepper</Button>}
        {canSet && <Button color="warning" onPress={() => set()}>Set Stepper</Button>}
        <Button color="primary" isDisabled={actionSent} onPress={() => prev()}>Prev</Button>
        <Button color="primary" isDisabled={actionSent} onPress={() => next()}>Next</Button>
        <Button color="danger" onPress={() => cancel()}>Cancel Action</Button>
     
      </div>
        <div className="flex flex-row gap-5 justify-center">
        
      </div>
    </div>
  );


  useEffect(() => {
    setDisableSelector(actionSent);
    if (!actionSent){
      return;
    }
    console.log(`Goal Response: ${goalResponse}`);
    console.log(`Action Sent: ${actionSent}`)
    if (goalResponse) {
      if (stepperGoalResponse.success) {
        setCurrentLocationIndex(targetLocationIndex);
        
        
        toast.success(`Successfully moved to ${locations[targetLocationIndex].display}`);
      }
      else {
        toast.error(`Failed to move to ${locations[targetLocationIndex].display}`);
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

  useEffect(() => {
    console.log(`Locations ${locations.toString()}`);
    console.log(`Target Location Index: ${targetLocationIndex}`);
    console.log(`Target Location: ${targetLocationIndex !== null ? locations[targetLocationIndex].display : "Unknown"}`);
  }, [currentLocationIndex]);


  return (
    <div>
        <div className="grid grid-cols-3">
          <div className="font-bold">Current Location:</div>
          <div className="">{currentLocationIndex ? locations[currentLocationIndex].display : `Unknown (Please ${ canZero ? "Zero" : "Set"})`}</div>
          <div className="">{feedback ? stepperFeedback.current_position: "-"}</div>
          <div className="font-bold">Target Location:</div>
          <div className="">{locations[targetLocationIndex].display}</div>
          <div className="">{targetPosition ? targetPosition : "-"}</div>
        </div>
        {pickerRow}
        {buttonRow}
    </div>

  )

}

export default StepperWidget;



