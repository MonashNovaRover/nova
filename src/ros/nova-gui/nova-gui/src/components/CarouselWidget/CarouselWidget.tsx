/**
 * Carousel GUI component
 * Author: Bailey Chessum
 * Date Created: 12/5/2024
 */
import React, {Key, useEffect, useState} from "react";
import {
  Card,
  CardBody,
  CardHeader, 
  CardProps,
  Select,
  SelectItem,
  Switch
} from "@nextui-org/react";
import { RosAction } from "../../ros/actions/RosAction";
import StepperWidget from "../StepperWidget/StepperWidget";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../ros/services/rosService";
import { IRosStdSrvsSetBoolResponse } from "../../ros/rosTypes";
import toast from "react-hot-toast";

/**
 * Props for CarouselWidget
 */
export interface CarouselWidgetProps extends CardProps {
}

/**
 * Widget that holds controls for moving the carousel to different positions.
 * It assumes that cuvette names are in the form of some index starting from 0 to some N
 * @constructor
 */
const CarouselWidget: React.FC<CarouselWidgetProps> = (props) => {
  const [ selectedCuvette, setSelectedCuvette ] = useState<Key>();
  const [ selectedCuvetteIndex, setSelectedCuvetteIndex ] = useState<number>(0);
  const [ disableSelector, setDisableSelector ] = useState<boolean>(true);
  const [ offset, setOffset ] = useState<number>();
  const [ LED1, setLED1 ] = useState<boolean>(false);
  const [ LED2, setLED2 ] = useState<boolean>(false);

  const bifrostLed1 = useBifrost({service: RosService.UV_VIS_LED_1});
  const bifrostLed2 = useBifrost({service: RosService.UV_VIS_LED_2});

  // What values should be sent to the ROS node as positions to go to?
  const instruments : { [key: string] : number} ={
    "Camera": 1,
    "UV Vis Spec": 6,
  };

  const cuvetteCount = 20;
  const cuvettes = Array.from({ length: cuvetteCount }, (_, i) => {
    return {
      display: String(i + 1),
      value: String(i + 1),
    }});

  const changeCuvette = (e: React.ChangeEvent<HTMLSelectElement>) => {
    console.log(e.target.value);
    e.target.value ? setSelectedCuvette(e.target.value) : setSelectedCuvette("1");
  }

  
  // Picker for the cuvette
  const cuvettePicker = (
    <Select 
      label="Cuvette" 
      placeholder="Select a Cuvette" 
      className="col-span-2" 
      isDisabled={disableSelector}
      onChange={changeCuvette}>
      {cuvettes.map((cuvette) => (
        <SelectItem 
          key={cuvette.value} >
          {cuvette.display}
        </SelectItem>
      ))}
    </Select>
  );

  const changeInstrument = (e: React.ChangeEvent<HTMLSelectElement>) => {
    console.log(e.target.value);
    e.target.value ? setOffset(Number(e.target.value)) : setOffset(1);
  }

  const changeLED1 = (value: boolean) => {
    bifrostLed1.callService({data: value}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosStdSrvsSetBoolResponse;
        if (boolResponse?.success) {
          setLED1(value);
        }
        else{
          toast.error("Failed to switch LED 1")
        }
      }, 
      sendToRedux: true,
    });
  }

  const changeLED2 = (value: boolean) => {
    bifrostLed2.callService({data: value}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosStdSrvsSetBoolResponse;
        if (boolResponse?.success) {
          setLED2(value);
        }
        else{
          toast.error("Failed to switch LED 2")
        }
      },
      sendToRedux: true,
    });
  }

  const ledRow = (
    <div className="flex flex-row gap-5">
      <div className="my-3">
        <div className="font-bold">LED 1 (Top)</div>
        <Switch 
          className="mt-1.5 mx-1.5" 
          isSelected={LED1}
          onChange={() => changeLED1(!LED1)}
        />
      </div>
      <div className="my-3">
        <div className="font-bold">LED 2 (Bottom)</div>
        <Switch 
          className="mt-1.5 mx-1.5" 
          isSelected={LED2}
          onChange={() => changeLED2(!LED2)}
        />
      </div>
    </div>
  );


  // Picker for where to move the selected cuvette to
  const instrumentPicker = (
    <Select 
      label="Instrument" 
      placeholder="Select an instrument" 
      className="col-span-2" 
      isDisabled={disableSelector}
      onChange={changeInstrument}>
      {Object.keys(instruments).map((instrument) => (
        <SelectItem 
          key={instruments[instrument]} 
          endContent={
            <span className="text-foreground-500">({instruments[instrument]})</span>
          }>
          {instrument}
        </SelectItem>
      ))}
    </Select>
  );


  useEffect(() => {
    if (!selectedCuvette || !offset) {
      return;
    }
    if (Number(selectedCuvette) >= 0 && Number(selectedCuvette) <= cuvetteCount) {
      setSelectedCuvetteIndex((Number(selectedCuvette)-1 + (offset-1)) % cuvetteCount);
    }
    console.log(`SelectedCuvette ${selectedCuvette}, SelectedCuvetteIndex`)
  }, [selectedCuvette, offset]);

  // Construct the widget
  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row">
        <div className="flex-grow">Carousel</div>
      </CardHeader>
      <CardBody>
   
        <StepperWidget 
          rosActionType={RosAction.CAROUSEL_ACTION}
          locations={cuvettes}
          targetLocationIndex={selectedCuvetteIndex}
          setTargetLocationIndex={setSelectedCuvetteIndex}
          setDisableSelector={setDisableSelector}
          canSet>
          {cuvettePicker}
          {instrumentPicker}
        </StepperWidget>
        {ledRow}
      </CardBody>

    </Card>
  )
}

export default CarouselWidget;


