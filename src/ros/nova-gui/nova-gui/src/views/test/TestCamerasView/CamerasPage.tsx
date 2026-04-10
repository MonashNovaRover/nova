import {Button, Card, CardBody, CardHeader, Tooltip} from "@nextui-org/react";
import {Eye, EyeOff, Menu, Pause, Play, Square, X} from "react-feather";
import { useCameraStreamer } from "../../../components/cameras/CameraComponent/hooks/useCameraStreamer";
import { useState } from "react";
import {
  CameraControlPanelModal,
} from "../../../components/cameras/CameraComponent/components/CamerasControlPanelModal";
import SegmentedPicker from "../../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import { SaveAllCamerasModal } from "../../../components/navbar/TopBar/SaveAllCamerasModal";
import { CameraPresetDropdown } from "../../../components/cameras/CameraPresetDropdown";
import {arcCameraSetup, ARCCompModes, CameraView} from "../../shared/CamerasPage/CameraPageConstants.tsx";
import SerialMappedCameraComponent from "../../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import {TestCameraTable} from "./CameraTable.tsx";
import {BooleanChip} from "../../../components/cameras/CameraComponent/components/BooleanChip.tsx";

export interface CameraPageProps {
  toggleSidebar: () => void;
  views: CameraView[];
}

const CameraPage = (props: CameraPageProps) => {
  const { views, toggleSidebar } = props;
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const [isSaveModalOpen, setIsSaveModalOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const { refreshAvailabilities } = useCameraStreamer();

  const [allCamsOn, setAllCamsOn] = useState(false);

  const [selectedTab, setSelectedTab] = useState(0);

  return (
    <div className="p-3 flex flex-col gap-0">
      <div className="flex flex-row justify-between items-center gap-32 pl-1 mb-3">
        <div className="flex flex-row gap-3 items-center">
          <Button isIconOnly onPressStart={toggleSidebar} variant="ghost" color="primary">
            <Menu/>
          </Button>
          {!allCamsOn ? (
            <Button
              size="md"
              color="primary"
              className="w-28"
              onPress={() => setAllCamsOn(true)}
            >
              <Eye size="15px" fill="white" /> Show All
            </Button>
          ) : (
            <Button
              size="md"
              color="danger"
              onPress={() => setAllCamsOn(false)}
            >
              <EyeOff size="15px" fill="white" /> Hide All
            </Button>
          )}
        </div>

        <SegmentedPicker
          selectedIndex={selectedTab}
          onIndexChange={setSelectedTab}
          children={[
            views.map(v => v.viewTitle)
          ]}
          color="primary"
          className="pb-0"
          fullWidth
          variant="bordered"
        />

        <div className="flex flex-row gap-3">
          <Button
            size="md"
            color="primary"
            variant="ghost"
            className="w-36"
            onPress={() => setControlPanelOpen(true)}
          >
            Control Panel
          </Button>
          <CameraPresetDropdown onSavePress={() => setIsSaveModalOpen(true)} />
        </div>
      </div>

      {
        <div className="grid grid-cols-4 gap-3">
          {views[selectedTab].cameraSerials.map((serial, i) => (
            <SerialMappedCameraComponent
              cameraSerial={serial}
              key={i}
              autostart={allCamsOn}
            />
          ))}
        </div>
      }

      <CameraControlPanelModal
        showModal={controlPanelOpen}
        closeModal={closeControlPanel}
        refreshAvailabilies={refreshAvailabilities}
      />
      <SaveAllCamerasModal
        isOpen={isSaveModalOpen}
        onClose={() => setIsSaveModalOpen(false)}
      />
    </div>
  );
};

export const TestCameraPage = () => {

  const [showSidebar, setShowSidebar] = useState(true)

  const expandedSidebarWidth = "27vw";
  const sidebarWidth = showSidebar ? expandedSidebarWidth : "0px";


  return (
    <div
      className="flex flex-row w-full items-stretch"
      style={{ height: "calc(100vh - 4.05rem)" }}
    >
      <div
        className="transition-all duration-300 overflow-clip h-full"
        style={{
          maxWidth: sidebarWidth,
          width:sidebarWidth
        }}
      >
        <div
          className="top-0 bottom-0 left-0 right-0 h-full"
          style={{
            width: expandedSidebarWidth
          }}
        >
          { <Card radius="none" className="h-full" style={{
            width: expandedSidebarWidth
          }}>
              <CardHeader>
                <span className="grow font-bold">
                   Control Panel
                </span>
                <Button isIconOnly size="sm" variant="light" onPressStart={() => setShowSidebar(false)}>
                  <X size={20}/>
                </Button>
              </CardHeader>
              <CardBody className="flex flex-col gap-3 overflow-y-auto">
                <Tooltip
                  className="dark text-foreground"
                  content="Not Real Time"
                  closeDelay={100}
                >
                  <BooleanChip
                    boolean={true}
                    variant="dot"
                    trueText="Cameras Running"
                    falseText="Cameras Stopped"
                    size="lg"
                  />
                </Tooltip>
                <span>Streaming Controls</span>
                <div className="grid grid-cols-3 gap-2">
                  <Button color="primary" size="sm" startContent={<Play size={14}/>}>
                    Start all
                  </Button>
                  <Button color="warning" size="sm" startContent={<Pause size={14}/>}>
                    Pause all
                  </Button>
                  <Button color="danger" size="sm" startContent={<Square size={14}/>}>
                    Stop all
                  </Button>
                </div>

                <span>Camera Status</span>
                <div>
                  <TestCameraTable/>
                </div>

                <div className="grow"></div>

              </CardBody>
          </Card>}
        </div>
      </div>
      <div className="grow">
        <CameraPage
          views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]}
          toggleSidebar={() => setShowSidebar(!showSidebar)}
        />
      </div>
    </div>
  )
}
