import {Button, Card, CardBody, CardFooter, CardHeader, Input, Tooltip} from "@nextui-org/react";
import {Minus, Pause, Play, Plus, Square, X} from "react-feather";
import {BooleanChip} from "../CameraComponent/components/BooleanChip.tsx";
import {CamerasTable} from "./CamerasTable.tsx";

interface CameraSidebarProps {
  refreshAvailabilities: () => void;
  showSidebar: boolean;
  setShowSidebar: (_: boolean) => void
  gridSize: number
  setGridSize: (_: number) => void
}

export const CameraSidebar = (
  {refreshAvailabilities, showSidebar, setShowSidebar, gridSize, setGridSize}
  : CameraSidebarProps) => {
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

          <CardFooter className="flex flex-row gap-3 items-center">
            <span className="shrink-0">Grid Size:</span>
            <Input
              className="flex-1 min-w-0"
              type="number"
              value={gridSize.toString()}
              onValueChange={val => setGridSize(parseInt(val))}
            />
            <Button isIconOnly className="shrink-0" onPressStart={() => setGridSize(gridSize -1)}><Minus/></Button>
            <Button isIconOnly className="shrink-0" onPressStart={() => setGridSize(gridSize +1)}><Plus/></Button>
          </CardFooter>
        </Card>
      </div>
    </div>
  )
}