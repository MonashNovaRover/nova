import {Button, Card, CardBody, CardHeader, Tooltip} from "@nextui-org/react";
import {Pause, Play, Square, X} from "react-feather";
import {BooleanChip} from "../CameraComponent/components/BooleanChip.tsx";
import {CamerasTable} from "./CamerasTable.tsx";

interface CameraSidebarProps {
  refreshAvailabilities: () => void;
  showSidebar: boolean;
  setShowSidebar: (_: boolean) => void
}

export const CameraSidebar = ({refreshAvailabilities, showSidebar, setShowSidebar}: CameraSidebarProps) => {
  const expandedSidebarWidth = "27vw";
  const sidebarWidth = showSidebar ? expandedSidebarWidth : "0px";

  return (
    <div
      className="transition-all duration-300 overflow-clip h-full"
      style={{
        maxWidth: sidebarWidth,
        width:sidebarWidth
      }}
    >
      <div
        className="top-0 bottom-0 left-0 right-0 h-full"
        style={{width: expandedSidebarWidth}}
      >
        <Card radius="none" className="h-full" style={{
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
            <CamerasTable refreshAvailabilies={refreshAvailabilities}/>

            <div className="grow"></div>

          </CardBody>
        </Card>
      </div>
    </div>
  )
}