import {Button, Card, CardBody, CardHeader, CardProps, Input} from "@nextui-org/react";
import { useEffect, useState } from "react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";
import { RosAction } from "../../ros/actions/RosAction.ts";
import { IRosNovaInterfacesPumpsActionFeedback, IRosNovaInterfacesPumpsActionGoal, IRosNovaInterfacesPumpsActionResult } from "../../ros/rosTypes.ts";
import { useRosAction } from "../../hooks/ros/useRosAction.ts";
import toast from "react-hot-toast";


export interface PumpsWidgetProps extends CardProps {}

const PUMPS = [
  {
    display: "Fill Shots",
    value: "fill_shots",
  },
  {
    display: "Fill Cuvettes (Prime)",
    value: "fill_cuvettes_prime",
  },
  {
    display: "Fill Cuvettes",
    value: "fill_cuvettes",
  },
  {
    display: "Clean Sheath",
    value: "clean_sheath",
  },
];


const PumpsWidget: React.FC<PumpsWidgetProps> = (props) => {
  const [selectedPumpIndex, setSelectedPumpIndex] = useState<number>(0.0);
 
  const [actionSent, setActionSent] = useState<boolean>(false);
  const [timeToRun, setTimeToRun] = useState<string>("");
  const { sendGoal, cancelGoal, feedback, goalResponse } = useRosAction(RosAction.PUMPS);

  const pumpsFeedback = feedback as IRosNovaInterfacesPumpsActionFeedback;
  const pumpsGoalResponse = goalResponse as IRosNovaInterfacesPumpsActionResult;

  const picker = (
      <SegmentedPicker onIndexChange={setSelectedPumpIndex} selectedIndex={selectedPumpIndex} isDisabled={actionSent}>
        {PUMPS.map((pump, index) => (
          <div key={index}>{pump.display}</div>
        ))}
      </SegmentedPicker>
  );

  const pickerRow = (
    <div className="mt-3">
      <div className="font-bold">Select Location</div>
      <div className="flex flex-row mt-1.5 gap-3 justify-center ">
        {picker}
      </div>
    </div>
  );


  const timeField = (
    <div className="flex flex-row p-1 justify-center">
      <Input
        className="w-3/4"
        label="Time To Run Pump"
        placeholder="0.0s"
        value={timeToRun}
        onValueChange={setTimeToRun}
        isDisabled={actionSent}
      />
    </div>
  )

  const sendAction = () => {
    if (actionSent) {
      return;
    }

    const time = Number(timeToRun)

    const invalidTimeToRun = (isNaN(time) || time > 1000 || time < 0)

    const goal = {
      "pump": PUMPS[selectedPumpIndex].value,
      ...(!invalidTimeToRun && {"time_to_run":  time}),
    } as IRosNovaInterfacesPumpsActionGoal;

    console.log(`${goal.time_to_run}`);

    sendGoal(goal);
    setActionSent(true);
  }

  const cancel = () => {
    console.log("Cancel Action");
    cancelGoal();
    setActionSent(false);
  }

  useEffect(() => {
    if (!actionSent){
      return;
    }
    console.log(`Goal Response: ${goalResponse}`);
    console.log(`Action Sent: ${actionSent}`)
    if (goalResponse && feedback) {
      if (pumpsGoalResponse.success) {      
        toast.success(`Successfully ran pump: ${PUMPS[selectedPumpIndex].display} for ${pumpsFeedback.time_to_run}s`);
      }
      else {
        toast.error(`Failed to run pump: ${PUMPS[selectedPumpIndex].display} for ${pumpsFeedback.time_to_run}s`);
      }
      setActionSent(false);
    }

  }, [actionSent, goalResponse, pumpsGoalResponse, feedback, selectedPumpIndex, pumpsFeedback]);



  return (
    <Card {...props}>
      <CardHeader className="pb-0">
      Pumps
      </CardHeader>
      <CardBody>
          <div>
            <div className="grid grid-cols-2">
              <div className="font-bold">Pump Name:</div>
              <div className="">{actionSent ? PUMPS[selectedPumpIndex].display : `Pumps Off`}</div>
              <div className="font-bold">Time To Run:</div>
              <div className="">{feedback ? `${pumpsFeedback.time_to_run.toFixed(2)} s`: "-"}</div>
              <div className="font-bold">Time Running:</div>
              <div className="">{feedback ? `${pumpsFeedback.time_running.toFixed(2)} s` : "-"}</div>
            </div>
            <div className="flex flex-col gap-2">
              {pickerRow}
              {timeField}
              <div className="flex flex-row gap-3 justify-center">
                <Button color="primary" isDisabled={actionSent} onPress={() => sendAction()}>
                  Run
                </Button> 
                <Button color="danger" onPress={() => cancel()}>Cancel Action</Button> 
              </div>
            </div>
        </div>
     
      </CardBody>
    </Card>
  )

}

export default PumpsWidget;



