import React, {useState} from "react";
import {useGenericStore} from "../../../../hooks/useGenericStore.ts";
import {Button, Input, Modal, ModalBody, ModalContent, ModalHeader, Tooltip} from "@nextui-org/react";
import { AngleType } from "./Theta360CamWidget.tsx";
import { HelpCircle } from "react-feather";

export interface heightCalc360CamProps {
  angles: AngleType,
  setAngles:  React.Dispatch<React.SetStateAction<AngleType>>
}

  const HeightCalculator: React.FC<heightCalc360CamProps> = (props) => {

  // local stores for input fields
  const [inputDistance, setInputDistance] = useGenericStore<string>("theta360InputDistance");
  const [inputLow, setInputLow] = useState<string>("")
  const [inputHigh, setInputHigh] = useState<string>("")

  const [isModalOpen, setModalOpen] = useState<boolean>(false)
  
  // Calculate height of landmark
  const radThetaHigh = props.angles.high * Math.PI / 180;
  const radThetaLow = props.angles.low * Math.PI / 180;
  const landmarkHeight = Number(inputDistance) * (Math.tan(radThetaHigh) - Math.tan(radThetaLow));

  // set input elements when props change due to canvas input
  const [prevAngles, setPrevAngles] = useState(props.angles);
  if (props.angles.low !== prevAngles.low || props.angles.high !== prevAngles.high) {
    setPrevAngles(props.angles);
    setInputLow(props.angles.low.toFixed(2));
    setInputHigh(props.angles.high.toFixed(2));
  }

  // set angles when text inputs change - handled in event handlers in input elements
  const setInputValues = () => {
    props.setAngles({low: Number(inputLow), high: Number(inputHigh)})
    setInputLow(Number(inputLow).toFixed(2))
    setInputHigh(Number(inputHigh).toFixed(2))
  }
  // height is calculated on render, render triggered on state change

  // input fields for height calculation
  const distField = (
    <div className="flex flex-row p-1 justify-start">
      <Input
        className="w-3/4"
        label="Distance"
        placeholder="0.0m"
        value={inputDistance}
        onValueChange={setInputDistance}
      />
    </div>
  )
  const lowThetaField = (
    <div className="flex flex-row p-1 justify-start">
      <Tooltip
        className="text-tiny text-default-500 rounded-md"
        content="Press Enter to confirm"
        placement="right"
      >
      <Input
        className="w-3/4"
        label="Lower Angle"
        placeholder="0.0deg"
        value={inputLow}
        onValueChange={setInputLow}
        onKeyDown={(e) => {
          const val = (e.target as HTMLInputElement).value
          if (e.key === "Enter" && !isNaN(Number(val))) {
            setInputValues()
          }
        }}
      />
      </Tooltip>
    </div>
  )
  const highThetaField = (
    <div className="flex flex-row p-1 justify-start">
      <Tooltip
        className="text-tiny text-default-500 rounded-md"
        content="Press Enter to confirm"
        placement="right"
      >
      <Input
        className="w-3/4"
        label="Upper Angle"
        placeholder="0.0deg"
        value={inputHigh}
        onValueChange={setInputHigh}
        onKeyDown={(e) => {
          const val = (e.target as HTMLInputElement).value
          if (e.key === "Enter" && !isNaN(Number(val))) {
            setInputValues()
          }
        }}
      />
      </Tooltip>
    </div>
  )

  return (
    <div className="flex flex-row gap-1 justify-start my-5">
      <Modal isOpen={isModalOpen} onClose={()=>setModalOpen(false)} className="dark text-foreground" size="xl">
        <ModalContent>
          <ModalHeader>
            Height Calculator Usage
          </ModalHeader>
          <ModalBody className="pb-5 inline">
            Manual input: Fill boxes and press <b>enter</b> or press calculate <br/>
            Fast fill (low): <b>Shift</b> + <b>Left Click</b> on canvas to fill low angle <br/>
            Fast fill (high): <b>Shift</b> + <b>Left Click</b> on canvas to fill low angle <br/>
            Fast fill will automatically calculate the height. 
          </ModalBody>
        </ModalContent>
      </Modal>
      <div className="flex flex-col gap-1">
        {distField}
        <span className="ml-3 w-[8rem] text-[0.7rem]">
          Distance of landmark from camera in metres
        </span>
      </div>
      <div className="flex flex-col gap-1">
        {lowThetaField}
        <span className="ml-3 w-[8rem] text-[0.7rem]">
          Low angle of landmark from camera center
        </span>
      </div>
      <div className="flex flex-col gap-1">
        {highThetaField}
        <span className="ml-3 w-[8rem] text-[0.7rem]">
          High angle of landmark from camera center
        </span>
      </div>
      <div className="flex flex-col p-1 justify-start gap-1">
        Height of landmark:
        <b> {landmarkHeight.toPrecision(2)}m </b>
        <div className="flex flex-row justify-between items-center">
          <Button className="-ml-1" onPress={setInputValues}>Calculate!</Button>
          <HelpCircle size="2rem" color="pink" onClick={()=>setModalOpen(true)}/>
        </div>
      </div>
    </div>
  )
}

export default HeightCalculator;