import { Card, CardHeader, CardBody, Divider, Progress, Select, SelectItem, Button } from "@nextui-org/react";
import { cloneElement, useState } from "react";
import './DriveWidget.css';
import { driveModes } from "./DriveModeDisplayData";
import { DriveModeButton } from "./DriveModeButton";


const DriveWidget: React.FC = () => {

  const [driveModeIndex, setDriveModeIndex] = useState("0");

  const handleDriveModeSelectChange = 
    (e: React.ChangeEvent<HTMLSelectElement>) => {
    setDriveModeIndex(e.target.value);
  };

  return (
    <div className="flex flex-row">
      <Card className="m-4 w-1/3">
        <CardHeader>Drive Mode Select - Old style with dropdown</CardHeader>
        <CardBody>
          <div className="flex gap-3 items-center pt-2 justify-center">

            <div className="grow"/>

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
                    tooltopPlacement="bottom"
                  />
                ))}
              </div>
            </div>
            
            <div className="grow"/>

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
                color="default"
                variant="bordered"
                className="max-w-xs mt-2 text-white" 
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

            <div className="grow"/>
          </div>
        </CardBody>
      </Card>
      <Card className="m-4 w-1/3">
        <CardHeader>Drive Mode Select - Vertical list</CardHeader>
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
                      tooltopPlacement="right"
                      hideTooltip
                    />
                    <span className="">{mode.name}</span>
                  </div>
                ))}
              </div>
            </div>
            <div>
              <div className="grid grid-flow-row gap-3 auto-cols-fr">
                {driveModes.map((mode, index) => (
                  
                    <DriveModeButton
                      driveModeData={mode}
                      driveModeActive={driveModeIndex === `${index}`}
                      onPress={() => setDriveModeIndex(`${index}`)}
                      iconClassName="w-5 h-5"
                      tooltopPlacement="right"
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
      <Card className="m-4 w-1/3">
        <CardHeader>Drive Mode Select - Horizontal List</CardHeader>
        <CardBody>
          <div>
            <div className="flex gap-3 flex-row">
              {driveModes.map((mode, index) => (
                <div key={index} className="flex w-1/4 gap-3 flex-row items-center">
                  <DriveModeButton
                    driveModeData={mode}
                    driveModeActive={driveModeIndex === `${index}`}
                    onPress={() => setDriveModeIndex(`${index}`)}
                    iconClassName="w-5 h-5"
                    tooltopPlacement="right"
                    hideTooltip
                  />
                  <span className="">{mode.name}</span>
                </div>
              ))}
            </div>
          </div>

          <Divider className="my-5"></Divider>

          <div>
            <div className="grid grid-flow-col  gap-3 auto-cols-fr">
              {driveModes.map((mode, index) => (
                
                  <DriveModeButton
                    driveModeData={mode}
                    driveModeActive={driveModeIndex === `${index}`}
                    onPress={() => setDriveModeIndex(`${index}`)}
                    iconClassName="w-5 h-5"
                    tooltopPlacement="right"
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
    </div>
  );
};
export default DriveWidget;
