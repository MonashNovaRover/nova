import {Button, Card, CardBody, CardHeader, CardProps, Input, Progress} from "@nextui-org/react";
import React, { useEffectEvent, useState } from "react";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";
import { RosAction } from "../../../ros/actions/RosAction.ts";
import { IRosScienceInterfacesPumpsActionFeedback, IRosScienceInterfacesPumpsActionGoal, IRosScienceInterfacesPumpsActionResult } from "../../../ros/rosTypes.ts";
import { useRosAction } from "../../../hooks/ros/useRosAction.ts";
import toast from "react-hot-toast";
import {Database, MoreHorizontal, Search, Square} from "react-feather";

export interface PumpsWidgetProps extends CardProps {}

const PUMPS = [
  {
    display: "→ Shots",
    value: "fill_shots",
  },
  {
    display: "→ Inner Ring (P)",
    value: "fill_inner_p",
  },
  {
    display: "→ Inner Ring",
    value: "fill_inner",
  },
  {
    display: "→ Outer Ring (P)",
    value: "fill_outer_p",
  },
  {
    display: "→ Outer Ring",
    value: "fill_outer",
  },
  {
    display: "→ Electrochem (P)",
    value: "fill_electrochem",
  },
  {
    display: "→ Electrochem",
    value: "fill_electrochem_p",
  },
  {
    display: "→ Sulphuric Acid",
    value: "fill_sulphuric_acid",
  },
];


const PumpsWidget: React.FC<PumpsWidgetProps> = (props) => {
  const [selectedPumpIndex, setSelectedPumpIndex] = useState<number>(0);
 
  const [actionSent, setActionSent] = useState<boolean>(false);
  const [timeToRun, setTimeToRun] = useState<string>("");
  const { sendGoal, cancelGoal, feedback, goalResponse } = useRosAction(RosAction.PUMPS);

  const [showModal, setShowModal] = useState(false)

  const pumpsFeedback = feedback as IRosScienceInterfacesPumpsActionFeedback;
  const pumpsGoalResponse = goalResponse as IRosScienceInterfacesPumpsActionResult;

  // const picker = (
  //     <SegmentedPicker onIndexChange={setSelectedPumpIndex} selectedIndex={selectedPumpIndex} isDisabled={actionSent}>
  //       {PUMPS.map((pump, index) => (
  //         <div key={index}>{pump.display}</div>
  //       ))}
  //     </SegmentedPicker>
  // );
  //
  // const pickerRow = (
  //   <div className="mt-3">
  //     <div className="font-bold">Select Location</div>
  //     <div className="flex flex-row mt-1.5 gap-3 justify-center ">
  //       {picker}
  //     </div>
  //   </div>
  // );


  const timeField = (
      <Input
        className="col-span-2"
        label="Time To Run"
        placeholder="0.0s"
        value={timeToRun}
        onValueChange={setTimeToRun}
        isDisabled={actionSent}
        endContent={
          <div className="pointer-events-none flex items-center">
            <span className="text-default-400 text-small">s</span>
          </div>
        }
      />
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
    } as IRosScienceInterfacesPumpsActionGoal;

    console.log(`${goal.time_to_run}`);

    sendGoal(goal);
    setActionSent(true);
  }

  const cancel = () => {
    console.log("Cancel Action");
    cancelGoal();
    setActionSent(false);
  }

  useEffectEvent(() => {
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

  });

  const progressBar = (
    <div className="flex flex-row items-center justify-center">
      <Square className="w-20"/>
      <Progress
        color="secondary"
        value={actionSent && selectedPumpIndex === 0 && pumpsFeedback ? pumpsFeedback.time_running : 0}
        maxValue={actionSent && pumpsFeedback ? pumpsFeedback.time_to_run : 1}
      />
      <Database className="w-20"/>
      {/*<Progress*/}
      {/*  color="secondary"*/}
      {/*  value={actionSent && selectedPumpIndex !== 0 && pumpsFeedback ? pumpsFeedback.time_running : 0}*/}
      {/*  maxValue={actionSent && pumpsFeedback ? pumpsFeedback.time_to_run : 1}*/}
      {/*/>*/}
      {/*<Search className="w-20"/>*/}
    </div>
  )


  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row justify-between">
        Pumps
        <Button
          variant={"light"}
          isIconOnly
          onPress={() => {
            setShowModal(true)
          }}
        >
          <MoreHorizontal/>
        </Button>
      </CardHeader>
      <CardBody className="flex flex-col gap-3">

        {progressBar}

        <div className="flex flex-row justify-around">
          <div className="w-40 text-center">
            <span>{actionSent && selectedPumpIndex === 0 && pumpsFeedback ? pumpsFeedback.time_running.toFixed(2) + " / " : "0 / 0s"}</span>
            <span>{actionSent && selectedPumpIndex === 0 && pumpsFeedback ? pumpsFeedback.time_to_run.toFixed(2) + "s" : ""}</span>
          </div>

          {/*<div className="w-40 text-center">*/}
          {/*  <span>{actionSent && selectedPumpIndex !== 0 && pumpsFeedback ? pumpsFeedback.time_running.toFixed(2) + " / " : " 0 / 0s"}</span>*/}
          {/*  <span>{actionSent && selectedPumpIndex !== 0 && pumpsFeedback ? pumpsFeedback.time_to_run.toFixed(2) + "s" : ""}</span>*/}
          {/*</div>*/}

        </div>

        {/*{pickerRow}*/}
        <div className="grid grid-cols-9 gap-3">
          <Select label="Select Location" className="col-span-3">
            {PUMPS.map(pump => <SelectItem key={pump.value}>{pump.display}</SelectItem>)}
          </Select>
          {timeField}

          <div className="grid grid-cols-2 gap-3 justify-center items-center col-span-4">
            <Button color="primary" isDisabled={actionSent} onPress={() => sendAction()}>
              Run
            </Button>
            <Button color="danger" onPress={() => cancel()}>Cancel</Button>
          </div>
        </div>

      </CardBody>
      {/*<Modal*/}
      {/*  isOpen={!showModal}*/}
      {/*  onClose={() => setShowModal(false)}*/}
      {/*>*/}
      {/*  <ModalHeader>Pumps Presets</ModalHeader>*/}
      {/*  <ModalBody className="flex flex-col">*/}
      {/*    <Input></Input>*/}
      {/*    <Input></Input>*/}
      {/*    <Input></Input>*/}
      {/*    <Input></Input>*/}
      {/*    <Input></Input>*/}
      {/*    <Input></Input>*/}
      {/*  </ModalBody>*/}
      {/*</Modal>*/}
    </Card>
  )

}

export default PumpsWidget;



