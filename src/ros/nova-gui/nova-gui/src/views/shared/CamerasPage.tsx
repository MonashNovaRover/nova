import { Button, Tab, Tabs } from "@nextui-org/react";
import { CameraComponent } from "../../components/CameraComponent/CameraComponent";
import { Play, Square } from "react-feather";

export const CameraPage = () => {
  return (
    <div>
      <div className="flex flex-row m-4 gap-4">
        <Button size="sm" color="primary">
          <Play size="15px" fill="white" /> Start All
        </Button>
        <Button size="sm" color="danger">
          <Square size="15px" fill="white" /> Stop All
        </Button>
      </div>
      <Tabs
        size="lg"
        color="primary"
        className=" p-4"
        fullWidth
        variant="bordered"
      >
        <Tab title="Extreme Delivery">
          <AdHocCamLauyout />
        </Tab>
        <Tab title="Autonomous">
          <AdHocCamLauyout />
        </Tab>
        <Tab title="Science">
          <AdHocCamLauyout />
        </Tab>
        <Tab title="Equipment Servicing">
          <AdHocCamLauyout />
        </Tab>
      </Tabs>
    </div>
  );
};

const AdHocCamLauyout = () => {
  return (
    <div className="grid grid-cols-3 gap-0">
      <CameraComponent
        cameraName="Arm 360 View Camera"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 2"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 3"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 4"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 5"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 6"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 7"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
      <CameraComponent
        cameraName="Camera 8"
        src="https://cdn.britannica.com/17/83817-050-67C814CD/Mount-Everest.jpg"
      />
    </div>
  );
};
