import {Card, CardHeader, CardBody, Select, SelectItem, Kbd} from "@nextui-org/react";
import { cloneElement, useState } from "react";
import './DriveWidget.css';
import { driveModes } from "./DriveModeDisplayData";
import { DriveModeButton } from "./DriveModeButton";
import DriveWidget from "./DriveWidget";


const DriveWidgetDemo: React.FC = () => {

  const [driveModeIndex, setDriveModeIndex] = useState("0");

  const handleDriveModeSelectChange =
    (e: React.ChangeEvent<HTMLSelectElement>) => {
      setDriveModeIndex(e.target.value);
    };

  return (
    <div className="grid  w-full gap-3 p-3 auto-cols-fr
      s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">

      <Card className="w-full col-span-1 row-span-2">
        <CardHeader>Drive Mode Select<br/>Old style with dropdown 1</CardHeader>
        <CardBody className="px-0 flex justify-center flex-col content-center">
          <div className="flex gap-3 justify-center overflow-hidden overflow-visible">
            <div>
              <Select
                variant="underlined"
                aria-label="Drive mode"
                label="Drive mode"
                className="max-w-xs"
                selectedKeys={[driveModeIndex]}
                onChange={handleDriveModeSelectChange}
              >
                {driveModes.map((mode, index) => (
                  <SelectItem key={index} value={`${index}`}
                              startContent={cloneElement(mode.icon, {className:
                                  `${mode.icon.props.className} w-3 h-3`})}
                              aria-label={mode.name}
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
                    tooltipPlacement="top"
                    driveModeActive={driveModeIndex === `${index}`}
                    onPress={() => setDriveModeIndex(`${index}`)}
                    iconClassName="w-5 h-5"
                  />
                ))}
              </div>

              <Select
                labelPlacement="outside"
                aria-label="Drive mode"
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
                              aria-label={mode.name}
                  >
                    {mode.name + " Mode"}
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
                    key={index}
                    driveModeData={mode}
                    driveModeActive={driveModeIndex === `${index}`}
                    onPress={() => setDriveModeIndex(`${index}`)}
                    iconClassName="w-5 h-5"
                    tooltipPlacement="right"
                    hideTooltip
                    hideKeybind
                    className="grow w-1/4 justify-start"
                  >
                    <span className="ml-1">{mode.name}</span>
                    <div className="grow"></div>
                    <div className="DriveModeButtonIconContainer">
                      <Kbd className="mx-2">{mode.keybind}</Kbd>
                    </div>

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
                  <span className="">{mode.shortName ?? mode.name}</span>
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
                  key={index}
                  driveModeData={mode}
                  driveModeActive={driveModeIndex === `${index}`}
                  onPress={() => setDriveModeIndex(`${index}`)}
                  iconClassName="w-5 h-5"
                  tooltipPlacement="right"
                  hideTooltip hideKeybind
                  className="grow w-1/4 justify-start"
                  keybindPlacement="top-left"
                >
                  <span className="ml-0.5">{mode.shortName ?? mode.name}</span>
                  <div className="grow"></div>
                  <Kbd className="mx-1.5">{mode.keybind}</Kbd>

                </DriveModeButton>
              ))}
            </div>
          </div>
        </CardBody>
      </Card>
      
      <DriveWidget className="w-full col-span-2 row-span-2"
                   driveModeIndex={driveModeIndex}
                   handleDriveModeSelectChange={handleDriveModeSelectChange}
                   setDriveModeIndex={setDriveModeIndex}>

      </DriveWidget>

    </div>


  );
};

export default DriveWidgetDemo;
