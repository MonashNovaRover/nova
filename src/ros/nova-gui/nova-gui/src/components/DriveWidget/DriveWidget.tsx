import { Card, CardHeader, CardBody, Divider, Progress, Select, SelectItem, Button } from "@nextui-org/react";
import { cloneElement, useState } from "react";
import './DriveWidget.css';
import { driveModes } from "./DriveModeDisplayData";
import { DriveModeButton } from "./DriveModeButton";
import {DriveProgress} from "./DriveProgress";


const DriveWidget: React.FC = () => {

  const [driveModeIndex, setDriveModeIndex] = useState("0");

  const handleDriveModeSelectChange = 
    (e: React.ChangeEvent<HTMLSelectElement>) => {
    setDriveModeIndex(e.target.value);
  };

  //  <div className="grid auto-cols-fr w-full gap-3 p-3 s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 xl:grid-cols-5 xxl:grid-cols-6">
  return (
    <div className="grid  w-full gap-3 p-3 auto-cols-fr
      s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">

      <Card className="w-full col-span-1 row-span-2">
        <CardHeader>Drive Mode Select<br/>Old style with dropdown 1</CardHeader>
        <CardBody className="px-0 flex justify-center flex-col content-center">
          <div className="flex gap-3 justify-center overflow-hidden">
            <div>
              <Select
                variant="underlined"
                label="Drive mode"
                className="max-w-xs"
                selectedKeys={[driveModeIndex]}
                onChange={handleDriveModeSelectChange}
              >
                {driveModes.map((mode, index) => (
                  <SelectItem key={index} value={`${index}`}
                    startContent={cloneElement(mode.icon, {className:
                      `${mode.icon.props.className} w-3 h-3`})}
                  >
                    {mode.name}
                  </SelectItem>
                ))}
              </Select>

              <div className="flex gap-3 items-center pt-3 justify-center">
                {driveModes.map((mode, index) => (
                  <DriveModeButton
                    key={index}
                    driveModeData={mode}
                    driveModeActive={driveModeIndex === `${index}`}
                    onPress={() => setDriveModeIndex(`${index}`)}
                    iconClassName="w-5 h-5"
                    tooltipPlacement="bottom"
                  />
                ))}
              </div>
            </div>
          </div>
        </CardBody>
      </Card>

      <Card className="w-full col-span-1 row-span-2">
        <CardHeader>Drive Mode Select<br/>Old style with dropdown 2</CardHeader>
        <CardBody className="px-0 flex justify-center flex-col content-center">
          <div className="flex gap-3 justify-center overflow-hidden">
            <div>
              <div className="flex gap-3 items-center pt-3 justify-center">
                {driveModes.map((mode, index) => (
                    <DriveModeButton
                        key={index}
                        driveModeData={mode}
                        driveModeActive={driveModeIndex === `${index}`}
                        onPress={() => setDriveModeIndex(`${index}`)}
                        iconClassName="w-5 h-5"
                    />
                ))}
              </div>

              <Select
                  labelPlacement="outside"
                  color="primary"
                  variant="faded"
                  className="max-w-xs mt-2"
                  selectedKeys={[driveModeIndex]}
                  onChange={handleDriveModeSelectChange}
              >
                {driveModes.map((mode, index) => (
                    <SelectItem key={index} value={`${index}`}
                                startContent={cloneElement(mode.icon, {className:
                                      `${mode.icon.props.className} w-3 h-3`})}
                    >
                      {mode.name}
                    </SelectItem>
                ))}
              </Select>
            </div>
          </div>
        </CardBody>
      </Card>



      <Card className="w-full col-span-1 row-span-2">
        <CardHeader>Drive Mode Select<br/>Vertical list (small buttons)</CardHeader>
        <CardBody>
          <div className="grid grid-flow-col gap-3 auto-cols-fr">
            <div>
              <div className="flex gap-3 flex-col">
                {driveModes.map((mode, index) => (
                  <div key={index} className="flex gap-3 flex-row items-center">
                    <DriveModeButton
                      driveModeData={mode}
                      driveModeActive={driveModeIndex === `${index}`}
                      onPress={() => setDriveModeIndex(`${index}`)}
                      iconClassName="w-5 h-5"
                      tooltipPlacement="right"
                      hideTooltip
                    />
                    <span className="">{mode.name}</span>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </CardBody>
      </Card>

      <Card className="w-full col-span-1 row-span-2">
        <CardHeader>Drive Mode Select<br/>Vertical list (large buttons)</CardHeader>
        <CardBody>
          <div className="grid grid-flow-col gap-3 auto-cols-fr">
            <div>
              <div className="grid grid-flow-row gap-3 auto-cols-fr">
                {driveModes.map((mode, index) => (

                    <DriveModeButton
                        driveModeData={mode}
                        driveModeActive={driveModeIndex === `${index}`}
                        onPress={() => setDriveModeIndex(`${index}`)}
                        iconClassName="w-5 h-5"
                        tooltipPlacement="right"
                        hideTooltip
                        className="grow w-1/4 justify-start"
                        fullWidth="true"
                    >
                      <span className="">{mode.name}</span>

                    </DriveModeButton>

                ))}
              </div>
            </div>
          </div>
        </CardBody>
      </Card>



      <Card className="w-full col-span-2">
        <CardHeader>Drive Mode Select - Horizontal List 1</CardHeader>
        <CardBody className="flex justify-center flex-col content-center">
          <div>
            <div className="flex gap-3 flex-row">
              {driveModes.map((mode, index) => (
                <div key={index} className="flex w-1/4 gap-3 flex-row items-center">
                  <DriveModeButton
                    driveModeData={mode}
                    driveModeActive={driveModeIndex === `${index}`}
                    onPress={() => setDriveModeIndex(`${index}`)}
                    iconClassName="w-5 h-5"
                    tooltipPlacement="right"
                    hideTooltip
                  />
                  <span className="">{mode.name}</span>
                </div>
              ))}
            </div>
          </div>
        </CardBody>
      </Card>

      <Card className="w-full col-span-2">
        <CardHeader>Drive Mode Select - Horizontal List 2</CardHeader>
        <CardBody className="flex justify-center flex-col content-center">
          <div>
            <div className="grid grid-flow-col gap-3 auto-cols-fr">
              {driveModes.map((mode, index) => (

                  <DriveModeButton
                      driveModeData={mode}
                      driveModeActive={driveModeIndex === `${index}`}
                      onPress={() => setDriveModeIndex(`${index}`)}
                      iconClassName="w-5 h-5"
                      tooltipPlacement="right"
                      hideTooltip
                      className="grow w-1/4 justify-start"
                      fullWidth="true"
                      keybindPlacement="top-left"
                  >
                    <span className="">{mode.name}</span>

                  </DriveModeButton>
              ))}
            </div>
          </div>
        </CardBody>
      </Card>

      <Card className="w-full col-span-2">
        <CardHeader>Progress w/ label overlay</CardHeader>
        <CardBody className="flex justify-center flex-col content-center gap-2" >
          <DriveProgress
            value={1+(+driveModeIndex)}
            maxValue={4}
            valueLabel={<span className="text-white">{`${100 * (1+(+driveModeIndex)) / 4}%`}</span>}
            size="lg" className=""></DriveProgress>
          <DriveProgress
            value={1+(+driveModeIndex)}
            maxValue={4}
            size="lg"
            label="Value:"
            valueLabel={<span className="text-white">{`${100 * (1+(+driveModeIndex)) / 4}%`}</span>}
            className="">
          </DriveProgress>
        </CardBody>
      </Card>

    </div>


  );
};

/*
<div className="ExpandingDriveModeButton">

                </div>

 */

export default DriveWidget;
