import { Card, CardHeader, CardBody, Divider, Progress, Select, SelectItem } from "@nextui-org/react";
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
    <Card className="m-4">
      <CardHeader>Drive</CardHeader>
      <CardBody>

        <div className="flex gap-3 items-center pt-2 justify-center">

          <Progress fg="1" fb="1px" className="" aria-label="progress"/>


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

          </div>
          
          <Progress fg="1" fb="1px" className="" aria-label="progress"/>
        </div>

        <Divider className="my-2"/>

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
      </CardBody>
    </Card>
  );
};
export default DriveWidget;
