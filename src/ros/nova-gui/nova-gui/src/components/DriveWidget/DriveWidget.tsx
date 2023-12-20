import {
  Card,
  CardHeader,
  CardBody,
  Divider,
  Progress,
  Select,
  SelectItem,
  Button,
  ButtonProps, CardProps
} from "@nextui-org/react";
import {cloneElement, ReactNode, useState} from "react";
import './DriveWidget.css';
import { DriveModeButton } from "./DriveModeButton";
import {DriveProgress} from "./DriveProgress";
import { driveModes } from "./DriveModeDisplayData";

// Properties for the DriveModeButton component.
export interface IDriveWidgetProps extends CardProps {
  driveModeIndex: string,
  handleDriveModeSelectChange: (e: React.ChangeEvent<HTMLSelectElement>) => void,
  setDriveModeIndex: (string) => void
}

export interface IDriveWidgetWheelData {
  wheelValue: number,
  pivotValue: number,
  name: ReactNode
}

const DriveWidgetWheelData: React.FC<IDriveWidgetWheelData> = (props: IDriveWidgetWheelData) => {
  return <Card shadow="sm" isBlurred>
    <CardBody className="grid grid-rows-1 grid-cols-4 content-center place-content-stretch">
      <div className="flex flex-col justify-center">
        <span className="align-middle">{props.name}</span>
      </div>
      <div className="flex flex-col gap-2 col-span-3">
        <DriveProgress size="lg" value={props.wheelValue} maxValue={1}>
          <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
            <span>WHEEL</span>
            <span>{`${(props.wheelValue * 100).toFixed(0)}%`}</span>
          </div>
        </DriveProgress>
        <DriveProgress size="lg" value={props.pivotValue} maxValue={1}>
          <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
            <span>PIVOT</span>
            <span>{`${(props.pivotValue * 100).toFixed(0)}%`}</span>
          </div>
        </DriveProgress>
      </div>
    </CardBody>
  </Card>
}


const DriveWidget: React.FC<IDriveWidgetProps> = (props: IDriveWidgetProps) => {

  const selector = (
    <CardBody className="grid DriveWidgetTopGrid gap-y-2.5 gap-x-5">
      <div className="flex flex-col justify-end items-center">
        <span>Average Velocity</span>
      </div>

      <div className="flex flex-col justify-end items-center">
        <span>{driveModes[props.driveModeIndex].fullName ?? driveModes[props.driveModeIndex].name} Mode</span>
      </div>

      <div className="flex flex-col justify-end items-center">
        <span>Speed Control</span>
      </div>

      <div>
        <DriveProgress size="lg" value={0.314} maxValue={1}>
          31.4%
        </DriveProgress>
      </div>

      <div className="flex gap-3 items-center justify-center">
        {driveModes.map((mode, index) => (
          <DriveModeButton
            key={index}
            driveModeData={mode}
            tooltipPlacement="bottom"
            driveModeActive={props.driveModeIndex === `${index}`}
            onPress={() => props.setDriveModeIndex(`${index}`)}
            iconClassName="w-5 h-5"
          />
        ))}
      </div>

      <div>
        <DriveProgress size="lg">
          0%
        </DriveProgress>
      </div>
    </CardBody>
  )

  return (<Card {...props} >
    <CardHeader className="text-h1">
      Drive
    </CardHeader>

    {selector}
    <Divider/>
    <CardBody className="flex flex-col gap-3">
      <div className="grid grid-cols-2 grid-rows-2 gap-2">
        <DriveWidgetWheelData wheelValue={0.2} pivotValue={0.5} name={<>Front<br/>Left</>}/>
        <DriveWidgetWheelData wheelValue={0.8} pivotValue={0.5} name={<>Front<br/>Right</>}/>
        <DriveWidgetWheelData wheelValue={0.2} pivotValue={0.7} name={<>Back<br/>Left</>}/>
        <DriveWidgetWheelData wheelValue={0.7} pivotValue={0.1} name={<>Back<br/>Right</>}/>
      </div>
    </CardBody>
  </Card>)
};

export default DriveWidget;
